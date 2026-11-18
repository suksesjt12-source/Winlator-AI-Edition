#include "aie/pe_image.hpp"

#include <cstring>
#include <utility>

namespace aie::engine {
namespace {

bool range_ok(std::size_t offset, std::size_t length, std::size_t total) {
    return offset <= total && length <= total - offset;
}

std::uint16_t u16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}

std::uint32_t u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

std::uint64_t u64(const std::uint8_t* p) {
    return static_cast<std::uint64_t>(u32(p)) |
           (static_cast<std::uint64_t>(u32(p + 4)) << 32);
}

PeParseResult fail(const char* message) {
    PeParseResult result;
    result.error = message;
    return result;
}

} // namespace

PeParseResult inspect_pe_image(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size < 64) return fail("File is too small for a DOS header.");
    if (data[0] != 'M' || data[1] != 'Z') return fail("Missing MZ signature.");

    const std::uint32_t pe_offset = u32(data + 0x3c);
    if (!range_ok(pe_offset, 24, size)) return fail("PE header is outside the file.");
    if (std::memcmp(data + pe_offset, "PE\0\0", 4) != 0)
        return fail("Missing PE signature.");

    const std::uint8_t* coff = data + pe_offset + 4;
    const std::uint16_t machine = u16(coff);
    const std::uint16_t section_count = u16(coff + 2);
    const std::uint16_t optional_size = u16(coff + 16);
    const std::size_t optional_offset = static_cast<std::size_t>(pe_offset) + 24;

    if (section_count == 0 || section_count > 96)
        return fail("Invalid PE section count.");
    if (optional_size < 2 || !range_ok(optional_offset, optional_size, size))
        return fail("Optional header is truncated.");

    const std::uint8_t* optional = data + optional_offset;
    const std::uint16_t magic = u16(optional);
    PeImageInfo info;
    info.machine = machine;
    info.section_count = section_count;

    std::size_t directory_offset = 0;
    std::size_t directory_count_offset = 0;
    if (magic == 0x10b) {
        if (optional_size < 96) return fail("PE32 optional header is truncated.");
        info.format = PeFormat::PE32;
        info.entry_point_rva = u32(optional + 16);
        info.preferred_image_base = u32(optional + 28);
        directory_count_offset = 92;
        directory_offset = 96;
        info.architecture = machine == 0x014c ? PeArchitecture::X86
                                               : PeArchitecture::Unsupported;
    } else if (magic == 0x20b) {
        if (optional_size < 112) return fail("PE32+ optional header is truncated.");
        info.format = PeFormat::PE32Plus;
        info.entry_point_rva = u32(optional + 16);
        info.preferred_image_base = u64(optional + 24);
        directory_count_offset = 108;
        directory_offset = 112;
        info.architecture = machine == 0x8664 ? PeArchitecture::X86_64
                                               : PeArchitecture::Unsupported;
    } else {
        return fail("Unsupported PE optional-header format.");
    }

    if (info.architecture == PeArchitecture::Unsupported)
        return fail("PE machine type and optional-header format are unsupported.");

    info.section_alignment = u32(optional + 32);
    info.file_alignment = u32(optional + 36);
    info.size_of_image = u32(optional + 56);
    info.size_of_headers = u32(optional + 60);

    const std::uint32_t directory_count = u32(optional + directory_count_offset);
    if (directory_count > 1) {
        if (!range_ok(directory_offset + 8, 8, optional_size))
            return fail("PE import data directory is truncated.");
        info.import_directory.rva = u32(optional + directory_offset + 8);
        info.import_directory.size = u32(optional + directory_offset + 12);
    }
    if (directory_count > 5) {
        if (!range_ok(directory_offset + 5 * 8, 8, optional_size))
            return fail("PE relocation data directory is truncated.");
        info.relocation_directory.rva = u32(optional + directory_offset + 5 * 8);
        info.relocation_directory.size = u32(optional + directory_offset + 5 * 8 + 4);
    }

    const std::size_t section_table = optional_offset + optional_size;
    const std::size_t table_bytes = static_cast<std::size_t>(section_count) * 40;
    if (!range_ok(section_table, table_bytes, size))
        return fail("PE section table is truncated.");

    info.sections.reserve(section_count);
    for (std::uint16_t i = 0; i < section_count; ++i) {
        const std::uint8_t* s = data + section_table + static_cast<std::size_t>(i) * 40;
        std::size_t name_length = 0;
        while (name_length < 8 && s[name_length] != 0) ++name_length;
        PeSection section;
        section.name.assign(reinterpret_cast<const char*>(s), name_length);
        section.virtual_size = u32(s + 8);
        section.virtual_address = u32(s + 12);
        section.raw_size = u32(s + 16);
        section.raw_offset = u32(s + 20);
        section.characteristics = u32(s + 36);

        if (section.raw_size != 0 &&
            !range_ok(section.raw_offset, section.raw_size, size))
            return fail("A PE section points outside the file.");
        info.sections.push_back(std::move(section));
    }

    PeParseResult result;
    result.ok = true;
    result.image = std::move(info);
    return result;
}

PeParseResult inspect_pe_image(const std::vector<std::uint8_t>& data) {
    return inspect_pe_image(data.data(), data.size());
}

} // namespace aie::engine
