#include "aie/pe_loader.hpp"
#include "aie/guest_memory.hpp"
#include "aie/x64_interpreter.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace aie::engine {
namespace {

bool range_ok(std::size_t offset, std::size_t length, std::size_t total) {
    return offset <= total && length <= total - offset;
}

std::uint16_t read16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}
std::uint32_t read32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}
std::uint64_t read64(const std::uint8_t* p) {
    return static_cast<std::uint64_t>(read32(p)) |
           (static_cast<std::uint64_t>(read32(p + 4)) << 32);
}
void write32(std::uint8_t* p, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<std::uint8_t>(value >> (i * 8));
}
void write64(std::uint8_t* p, std::uint64_t value) {
    write32(p, static_cast<std::uint32_t>(value));
    write32(p + 4, static_cast<std::uint32_t>(value >> 32));
}

PeLoadResult fail(const char* message) {
    PeLoadResult result;
    result.error = message;
    return result;
}

bool image_range(const std::vector<std::uint8_t>& image,
                 std::uint64_t rva, std::size_t length) {
    return rva <= image.size() &&
           length <= image.size() - static_cast<std::size_t>(rva);
}

bool read_string(const std::vector<std::uint8_t>& image,
                 std::uint32_t rva, std::string& out) {
    if (!image_range(image, rva, 1)) return false;
    out.clear();
    const std::size_t start = rva;
    const std::size_t limit = std::min(image.size(), start + 4096U);
    for (std::size_t i = start; i < limit; ++i) {
        const char c = static_cast<char>(image[i]);
        if (c == '\0') return true;
        out.push_back(c);
    }
    return false;
}

bool add_delta(std::uint64_t value, std::uint64_t actual_base,
               std::uint64_t preferred_base, std::uint64_t& adjusted) {
    if (actual_base >= preferred_base) {
        const std::uint64_t delta = actual_base - preferred_base;
        if (value > std::numeric_limits<std::uint64_t>::max() - delta) return false;
        adjusted = value + delta;
    } else {
        const std::uint64_t delta = preferred_base - actual_base;
        if (value < delta) return false;
        adjusted = value - delta;
    }
    return true;
}

bool apply_relocations(LoadedPeImage& loaded, std::string& error) {
    const auto directory = loaded.info.relocation_directory;
    const bool relocated = loaded.load_base != loaded.info.preferred_image_base;
    if (directory.rva == 0 || directory.size == 0) {
        if (relocated) {
            error = "Image requires rebasing but has no base-relocation directory.";
            return false;
        }
        return true;
    }
    if (!image_range(loaded.memory, directory.rva, directory.size)) {
        error = "Base-relocation directory is outside the mapped image.";
        return false;
    }
    std::size_t cursor = directory.rva;
    const std::size_t end = cursor + directory.size;
    while (cursor < end) {
        if (!range_ok(cursor, 8, end)) {
            error = "Truncated base-relocation block.";
            return false;
        }
        const std::uint32_t page_rva = read32(loaded.memory.data() + cursor);
        const std::uint32_t block_size = read32(loaded.memory.data() + cursor + 4);
        if (page_rva == 0 && block_size == 0) break;
        if (block_size < 8 || (block_size & 1U) != 0 ||
            !range_ok(cursor, block_size, end)) {
            error = "Invalid base-relocation block size.";
            return false;
        }

        const std::size_t entries = (block_size - 8U) / 2U;
        for (std::size_t i = 0; i < entries; ++i) {
            const std::uint16_t item =
                read16(loaded.memory.data() + cursor + 8U + i * 2U);
            const std::uint16_t type = item >> 12;
            const std::uint16_t offset = item & 0x0fffU;
            if (type == 0) continue; // IMAGE_REL_BASED_ABSOLUTE padding.

            const std::uint64_t patch_rva =
                static_cast<std::uint64_t>(page_rva) + offset;
            const bool is_64 = loaded.info.format == PeFormat::PE32Plus;
            const std::size_t width = is_64 ? 8U : 4U;
            if ((is_64 && type != 10) || (!is_64 && type != 3)) {
                error = "Unsupported base-relocation type for this PE architecture.";
                return false;
            }
            if (!image_range(loaded.memory, patch_rva, width)) {
                error = "Base-relocation target is outside the mapped image.";
                return false;
            }

            if (!relocated) continue;

            std::uint64_t original = is_64
                ? read64(loaded.memory.data() + patch_rva)
                : read32(loaded.memory.data() + patch_rva);
            std::uint64_t adjusted = 0;
            if (!add_delta(original, loaded.load_base,
                           loaded.info.preferred_image_base, adjusted)) {
                error = "Base-relocation arithmetic overflow.";
                return false;
            }
            if (!is_64 && adjusted > std::numeric_limits<std::uint32_t>::max()) {
                error = "32-bit relocation result exceeds the PE address width.";
                return false;
            }
            if (is_64) write64(loaded.memory.data() + patch_rva, adjusted);
            else write32(loaded.memory.data() + patch_rva,
                        static_cast<std::uint32_t>(adjusted));
        }
        cursor += block_size;
    }
    return true;
}

bool resolve_imports(LoadedPeImage& loaded, const ImportResolver& resolver,
                     std::string& error) {
    const auto directory = loaded.info.import_directory;
    if (directory.rva == 0 && directory.size == 0) return true;
    if (directory.rva == 0 || directory.size < 20 ||
        !image_range(loaded.memory, directory.rva, directory.size)) {
        error = "Import directory is invalid or outside the mapped image.";
        return false;
    }
    if (!resolver) {
        error = "Image imports require an explicit import resolver.";
        return false;
    }

    const bool is_64 = loaded.info.format == PeFormat::PE32Plus;
    const std::size_t width = is_64 ? 8U : 4U;
    const std::uint64_t ordinal_flag =
        is_64 ? 0x8000000000000000ULL : 0x80000000ULL;
    const std::size_t end = static_cast<std::size_t>(directory.rva) + directory.size;
    bool descriptor_terminator_found = false;

    for (std::size_t desc = directory.rva;
         range_ok(desc, 20, end); desc += 20) {
        const std::uint8_t* d = loaded.memory.data() + desc;
        const std::uint32_t original_thunk = read32(d);
        const std::uint32_t name_rva = read32(d + 12);
        const std::uint32_t first_thunk = read32(d + 16);
        if (original_thunk == 0 && read32(d + 4) == 0 &&
            read32(d + 8) == 0 && name_rva == 0 && first_thunk == 0) {
            descriptor_terminator_found = true;
            break;
        }
        if (name_rva == 0 || first_thunk == 0) {
            error = "Import descriptor is missing its module name or IAT.";
            return false;
        }

        std::string module;
        if (!read_string(loaded.memory, name_rva, module) || module.empty()) {
            error = "Import descriptor contains an invalid module name.";
            return false;
        }

        const std::uint32_t lookup_rva =
            original_thunk != 0 ? original_thunk : first_thunk;
        bool thunk_terminator_found = false;
        for (std::size_t index = 0; ; ++index) {
            const std::uint64_t lookup = static_cast<std::uint64_t>(lookup_rva) +
                                         index * width;
            const std::uint64_t iat = static_cast<std::uint64_t>(first_thunk) +
                                      index * width;
            if (!image_range(loaded.memory, lookup, width) ||
                !image_range(loaded.memory, iat, width)) {
                error = "Import thunk table is outside the mapped image.";
                return false;
            }

            const std::uint64_t thunk = is_64
                ? read64(loaded.memory.data() + lookup)
                : read32(loaded.memory.data() + lookup);
            if (thunk == 0) {
                thunk_terminator_found = true;
                break;
            }

            ImportedSymbol symbol;
            if ((thunk & ordinal_flag) != 0) {
                symbol.by_ordinal = true;
                symbol.ordinal = static_cast<std::uint16_t>(thunk & 0xffffU);
            } else {
                if (thunk > std::numeric_limits<std::uint32_t>::max() ||
                    !image_range(loaded.memory, thunk, 3)) {
                    error = "Import-by-name entry is outside the mapped image.";
                    return false;
                }
                const std::uint32_t function_name_rva =
                    static_cast<std::uint32_t>(thunk) + 2U;
                if (!read_string(loaded.memory, function_name_rva, symbol.name) ||
                    symbol.name.empty()) {
                    error = "Import-by-name string is invalid.";
                    return false;
                }
            }

            const auto address = resolver(module, symbol);
            if (!address.has_value()) {
                error = "Import resolver did not provide an address for a requested symbol.";
                return false;
            }
            if (!is_64 && *address > std::numeric_limits<std::uint32_t>::max()) {
                error = "Resolved import address cannot fit in a PE32 IAT entry.";
                return false;
            }
            if (is_64) write64(loaded.memory.data() + iat, *address);
            else write32(loaded.memory.data() + iat,
                        static_cast<std::uint32_t>(*address));
        }
        if (!thunk_terminator_found) {
            error = "Import thunk table has no terminator.";
            return false;
        }
    }

    if (!descriptor_terminator_found) {
        error = "Import descriptor array has no null terminator.";
        return false;
    }
    return true;
}

} // namespace

PeLoadResult map_pe_image(const std::vector<std::uint8_t>& file,
                          std::uint64_t requested_base,
                          const ImportResolver& resolver,
                          std::size_t maximum_image_size) {
    const auto parsed = inspect_pe_image(file);
    if (!parsed.ok) return fail(parsed.error.c_str());

    const PeImageInfo& info = parsed.image;
    if (info.size_of_image == 0 || info.size_of_headers == 0 ||
        info.size_of_image > maximum_image_size ||
        info.size_of_headers > info.size_of_image ||
        info.size_of_headers > file.size()) {
        return fail("PE image/header sizes are invalid or exceed the configured limit.");
    }
    if (info.entry_point_rva >= info.size_of_image) {
        return fail("PE entry-point RVA is outside the image.");
    }

    const std::uint64_t load_base =
        requested_base == 0 ? info.preferred_image_base : requested_base;
    if (info.format == PeFormat::PE32 &&
        (load_base > std::numeric_limits<std::uint32_t>::max() ||
         info.size_of_image > 0x100000000ULL - load_base)) {
        return fail("Requested PE32 load address is outside the 32-bit address space.");
    }
    if (load_base > std::numeric_limits<std::uint64_t>::max() - info.size_of_image) {
        return fail("Requested image address range overflows.");
    }

    LoadedPeImage loaded;
    loaded.info = info;
    loaded.load_base = load_base;
    loaded.memory.assign(info.size_of_image, 0);
    std::memcpy(loaded.memory.data(), file.data(), info.size_of_headers);

    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;
    ranges.reserve(info.sections.size());
    for (const auto& section : info.sections) {
        const std::uint64_t mapped_size =
            std::max(section.virtual_size, section.raw_size);
        if (mapped_size == 0) continue;
        const std::uint64_t begin = section.virtual_address;
        const std::uint64_t finish = begin + mapped_size;
        if (finish < begin || finish > info.size_of_image ||
            begin < info.size_of_headers) {
            return fail("PE section virtual range is invalid.");
        }
        for (const auto& existing : ranges) {
            if (begin < existing.second && existing.first < finish)
                return fail("PE sections have overlapping virtual ranges.");
        }
        ranges.emplace_back(begin, finish);

        if (section.raw_size != 0) {
            if (!range_ok(section.raw_offset, section.raw_size, file.size()) ||
                !image_range(loaded.memory, section.virtual_address, section.raw_size)) {
                return fail("PE section raw data cannot be mapped safely.");
            }
            std::memcpy(loaded.memory.data() + section.virtual_address,
                        file.data() + section.raw_offset, section.raw_size);
        }
    }

    std::string error;
    if (!apply_relocations(loaded, error)) return fail(error.c_str());
    if (!resolve_imports(loaded, resolver, error)) return fail(error.c_str());

    PeLoadResult result;
    result.ok = true;
    result.image = std::move(loaded);
    return result;
}



GuestPeLoadResult map_pe_image_to_guest_memory(
    const std::vector<std::uint8_t>& file, GuestMemory& guest_memory,
    std::uint64_t requested_base, const ImportResolver& resolver,
    std::size_t maximum_image_size) {
    GuestPeLoadResult result;
    auto loaded = map_pe_image(file, requested_base, resolver, maximum_image_size);
    if (!loaded.ok) {
        result.error = loaded.error;
        return result;
    }

    const std::uint64_t load_base = loaded.image.load_base;
    if ((load_base % GUEST_PAGE_SIZE) != 0) {
        result.error = "PE load base must be aligned to the guest page size.";
        return result;
    }

    const std::size_t image_size = loaded.image.memory.size();
    const std::size_t page_size = static_cast<std::size_t>(GUEST_PAGE_SIZE);
    if (image_size == 0 ||
        image_size > std::numeric_limits<std::size_t>::max() - (page_size - 1U)) {
        result.error = "PE image size cannot be rounded safely to guest pages.";
        return result;
    }
    const std::size_t mapped_size =
        ((image_size + page_size - 1U) / page_size) * page_size;
    const std::size_t page_count = mapped_size / page_size;

    std::string memory_error;
    if (!guest_memory.map(load_base, mapped_size,
                          GuestMemoryRead | GuestMemoryWrite, &memory_error)) {
        result.error = "Could not reserve PE guest memory: " + memory_error;
        return result;
    }

    const auto rollback = [&guest_memory, load_base, mapped_size]() {
        std::string ignored;
        guest_memory.unmap(load_base, mapped_size, &ignored);
    };

    if (!guest_memory.write(load_base, loaded.image.memory.data(), image_size)) {
        rollback();
        result.error = "Could not copy the relocated PE image into guest memory.";
        return result;
    }

    // Remove all permissions first, then enable only pages owned by headers
    // or sections. This leaves alignment gaps inaccessible by default.
    if (!guest_memory.protect(load_base, mapped_size, GuestMemoryNone, &memory_error)) {
        rollback();
        result.error = "Could not apply initial guest-memory protections: " + memory_error;
        return result;
    }

    std::vector<std::uint8_t> page_permissions(page_count, GuestMemoryNone);
    const std::size_t header_pages =
        (static_cast<std::size_t>(loaded.image.info.size_of_headers) +
         page_size - 1U) / page_size;
    if (header_pages > page_count) {
        rollback();
        result.error = "PE headers exceed the rounded guest mapping.";
        return result;
    }
    for (std::size_t page = 0; page < header_pages; ++page)
        page_permissions[page] |= GuestMemoryRead;

    constexpr std::uint32_t SECTION_MEM_EXECUTE = 0x20000000U;
    constexpr std::uint32_t SECTION_MEM_READ = 0x40000000U;
    constexpr std::uint32_t SECTION_MEM_WRITE = 0x80000000U;

    for (const auto& section : loaded.image.info.sections) {
        const std::uint64_t extent =
            std::max(section.virtual_size, section.raw_size);
        if (extent == 0) continue;

        const std::uint64_t begin = section.virtual_address;
        const std::uint64_t end = begin + extent;
        if (end < begin || end > image_size) {
            rollback();
            result.error = "PE section exceeds the loaded image while applying page permissions.";
            return result;
        }

        std::uint8_t permissions = GuestMemoryNone;
        if ((section.characteristics & SECTION_MEM_READ) != 0)
            permissions |= GuestMemoryRead;
        if ((section.characteristics & SECTION_MEM_WRITE) != 0)
            permissions |= GuestMemoryWrite;
        if ((section.characteristics & SECTION_MEM_EXECUTE) != 0)
            permissions |= GuestMemoryExecute;

        const std::size_t first_page =
            static_cast<std::size_t>(begin / page_size);
        const std::size_t last_page =
            static_cast<std::size_t>((end + page_size - 1U) / page_size);
        if (last_page > page_count) {
            rollback();
            result.error = "PE section page permissions exceed the guest mapping.";
            return result;
        }
        for (std::size_t page = first_page; page < last_page; ++page) {
            page_permissions[page] |= permissions;
            if ((page_permissions[page] & (GuestMemoryWrite | GuestMemoryExecute)) ==
                (GuestMemoryWrite | GuestMemoryExecute)) {
                rollback();
                result.error = "PE section layout would create a writable and executable guest page.";
                return result;
            }
        }
    }

    // Apply contiguous runs of identical permissions to reduce protection calls.
    std::size_t first_page = 0;
    while (first_page < page_count) {
        const std::uint8_t permissions = page_permissions[first_page];
        std::size_t end_page = first_page + 1U;
        while (end_page < page_count &&
               page_permissions[end_page] == permissions) {
            ++end_page;
        }
        const std::uint64_t range_base =
            load_base + static_cast<std::uint64_t>(first_page * page_size);
        const std::size_t range_size = (end_page - first_page) * page_size;
        if (!guest_memory.protect(range_base, range_size, permissions, &memory_error)) {
            rollback();
            result.error = "Could not apply PE section page permissions: " + memory_error;
            return result;
        }
        first_page = end_page;
    }

    result.ok = true;
    result.info = std::move(loaded.image.info);
    result.load_base = load_base;
    result.entry_point = load_base + result.info.entry_point_rva;
    result.mapped_size = mapped_size;
    return result;
}

X64RunResult run_pe_entry_subset(const GuestPeLoadResult& image,
                                 GuestMemory& guest_memory,
                                 X64CpuState initial_state,
                                 std::size_t maximum_steps) {
    if (!image.ok) {
        X64RunResult failed;
        failed.reason = X64StopReason::InvalidInstructionPointer;
        failed.error = "PE image is not mapped successfully.";
        return failed;
    }
    if (image.info.architecture != PeArchitecture::X86_64) {
        X64RunResult failed;
        failed.reason = X64StopReason::UnsupportedInstruction;
        failed.error = "The current PE entry runner supports x86-64 images only.";
        return failed;
    }

    for (const auto& section : image.info.sections) {
        const std::uint64_t extent =
            std::max(section.virtual_size, section.raw_size);
        const std::uint64_t begin = section.virtual_address;
        const std::uint64_t end = begin + extent;
        if ((section.characteristics & 0x20000000U) == 0 ||
            image.info.entry_point_rva < begin ||
            image.info.entry_point_rva >= end || extent == 0) {
            continue;
        }
        if (extent > std::numeric_limits<std::size_t>::max()) {
            X64RunResult failed;
            failed.reason = X64StopReason::InvalidInstructionPointer;
            failed.error = "PE entry section is too large for the current interpreter.";
            return failed;
        }

        std::vector<std::uint8_t> code(static_cast<std::size_t>(extent));
        if (!guest_memory.fetch(image.load_base + begin, code.data(), code.size())) {
            X64RunResult failed;
            failed.reason = X64StopReason::MemoryFault;
            failed.error = "PE entry section is not mapped with execute permission.";
            return failed;
        }
        initial_state.rip = image.info.entry_point_rva - section.virtual_address;
        return run_x64_subset(code.data(), code.size(), initial_state,
                              maximum_steps, &guest_memory);
    }

    X64RunResult failed;
    failed.reason = X64StopReason::InvalidInstructionPointer;
    failed.error = "PE entry point does not belong to an executable section.";
    return failed;
}


} // namespace aie::engine
