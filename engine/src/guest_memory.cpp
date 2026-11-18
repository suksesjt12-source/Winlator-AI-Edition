#include "aie/guest_memory.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace aie::engine {
namespace {

constexpr std::uint8_t KNOWN_PERMISSIONS =
    GuestMemoryRead | GuestMemoryWrite | GuestMemoryExecute;

void set_error(std::string* error, const char* message) {
    if (error != nullptr) *error = message;
}

bool range_fits(std::uint64_t base, std::size_t size) {
    return size != 0 &&
           size <= std::numeric_limits<std::uint64_t>::max() - base;
}

} // namespace

GuestMemory::GuestMemory(std::size_t maximum_mapped_bytes)
    : maximum_mapped_bytes_(maximum_mapped_bytes) {}

bool GuestMemory::map(std::uint64_t base, std::size_t size,
                      std::uint8_t permissions, std::string* error) {
    if ((base % GUEST_PAGE_SIZE) != 0 ||
        (size % GUEST_PAGE_SIZE) != 0 ||
        !range_fits(base, size)) {
        set_error(error, "Guest mappings require a non-empty, page-aligned, non-overflowing range.");
        return false;
    }
    if ((permissions & ~KNOWN_PERMISSIONS) != 0) {
        set_error(error, "Guest mapping contains unknown permission bits.");
        return false;
    }

    const std::size_t currently_mapped = mapped_bytes();
    if (size > maximum_mapped_bytes_ ||
        currently_mapped > maximum_mapped_bytes_ - size) {
        set_error(error, "Guest memory limit would be exceeded.");
        return false;
    }

    const std::uint64_t end = base + static_cast<std::uint64_t>(size);
    for (const auto& region : regions_) {
        const std::uint64_t region_end =
            region.base + static_cast<std::uint64_t>(region.size);
        if (base < region_end && region.base < end) {
            set_error(error, "Guest mapping overlaps an existing region.");
            return false;
        }
    }

    Region region;
    region.base = base;
    region.size = size;
    try {
        region.page_permissions.assign(
            size / static_cast<std::size_t>(GUEST_PAGE_SIZE), permissions);
        region.bytes.assign(size, 0);
        regions_.push_back(std::move(region));
    } catch (const std::bad_alloc&) {
        set_error(error, "Unable to allocate guest memory backing storage.");
        return false;
    }

    std::sort(regions_.begin(), regions_.end(),
              [](const Region& left, const Region& right) {
                  return left.base < right.base;
              });
    return true;
}

bool GuestMemory::unmap(std::uint64_t base, std::size_t size,
                        std::string* error) {
    const auto found = std::find_if(regions_.begin(), regions_.end(),
        [base, size](const Region& region) {
            return region.base == base && region.size == size;
        });
    if (found == regions_.end()) {
        set_error(error, "Unmap requires an exact existing guest region.");
        return false;
    }
    regions_.erase(found);
    return true;
}

bool GuestMemory::protect(std::uint64_t base, std::size_t size,
                          std::uint8_t permissions, std::string* error) {
    if ((base % GUEST_PAGE_SIZE) != 0 ||
        (size % GUEST_PAGE_SIZE) != 0 || !range_fits(base, size)) {
        set_error(error, "Guest protection requires a non-empty page-aligned range.");
        return false;
    }
    if ((permissions & ~KNOWN_PERMISSIONS) != 0) {
        set_error(error, "Guest protection contains unknown permission bits.");
        return false;
    }

    Region* region = find_region(base, size);
    if (region == nullptr) {
        set_error(error, "Protection range is not contained in one existing guest mapping.");
        return false;
    }

    const std::size_t first_page =
        static_cast<std::size_t>((base - region->base) / GUEST_PAGE_SIZE);
    const std::size_t page_count =
        size / static_cast<std::size_t>(GUEST_PAGE_SIZE);
    if (first_page > region->page_permissions.size() ||
        page_count > region->page_permissions.size() - first_page) {
        set_error(error, "Guest protection range is outside the mapping.");
        return false;
    }

    std::fill(region->page_permissions.begin() + first_page,
              region->page_permissions.begin() + first_page + page_count,
              permissions);
    return true;
}

GuestMemory::Region* GuestMemory::find_region(std::uint64_t address,
                                               std::size_t size) {
    if (size == 0 || size > std::numeric_limits<std::uint64_t>::max() - address)
        return nullptr;
    const std::uint64_t end = address + static_cast<std::uint64_t>(size);
    for (auto& region : regions_) {
        const std::uint64_t region_end =
            region.base + static_cast<std::uint64_t>(region.size);
        if (address >= region.base && end <= region_end) return &region;
    }
    return nullptr;
}

const GuestMemory::Region* GuestMemory::find_region(
    std::uint64_t address, std::size_t size) const {
    if (size == 0 || size > std::numeric_limits<std::uint64_t>::max() - address)
        return nullptr;
    const std::uint64_t end = address + static_cast<std::uint64_t>(size);
    for (const auto& region : regions_) {
        const std::uint64_t region_end =
            region.base + static_cast<std::uint64_t>(region.size);
        if (address >= region.base && end <= region_end) return &region;
    }
    return nullptr;
}

bool GuestMemory::has_permission(const Region& region, std::uint64_t address,
                                 std::size_t size,
                                 std::uint8_t permission) const {
    if (size == 0) return false;
    const std::size_t first_byte =
        static_cast<std::size_t>(address - region.base);
    const std::size_t last_byte = first_byte + size - 1U;
    const std::size_t first_page =
        first_byte / static_cast<std::size_t>(GUEST_PAGE_SIZE);
    const std::size_t last_page =
        last_byte / static_cast<std::size_t>(GUEST_PAGE_SIZE);
    for (std::size_t page = first_page; page <= last_page; ++page) {
        if ((region.page_permissions[page] & permission) == 0) return false;
    }
    return true;
}

bool GuestMemory::read(std::uint64_t address, void* destination,
                       std::size_t size) const {
    if (destination == nullptr || size == 0) return false;
    const Region* region = find_region(address, size);
    if (region == nullptr ||
        !has_permission(*region, address, size, GuestMemoryRead)) return false;
    const std::size_t offset = static_cast<std::size_t>(address - region->base);
    std::memcpy(destination, region->bytes.data() + offset, size);
    return true;
}

bool GuestMemory::fetch(std::uint64_t address, void* destination,
                        std::size_t size) const {
    if (destination == nullptr || size == 0) return false;
    const Region* region = find_region(address, size);
    if (region == nullptr ||
        !has_permission(*region, address, size, GuestMemoryExecute)) return false;
    const std::size_t offset = static_cast<std::size_t>(address - region->base);
    std::memcpy(destination, region->bytes.data() + offset, size);
    return true;
}

bool GuestMemory::write(std::uint64_t address, const void* source,
                        std::size_t size) {
    if (source == nullptr || size == 0) return false;
    Region* region = find_region(address, size);
    if (region == nullptr ||
        !has_permission(*region, address, size, GuestMemoryWrite)) return false;
    const std::size_t offset = static_cast<std::size_t>(address - region->base);
    std::memcpy(region->bytes.data() + offset, source, size);
    return true;
}

std::size_t GuestMemory::mapped_bytes() const {
    std::size_t total = 0;
    for (const auto& region : regions_) total += region.size;
    return total;
}

} // namespace aie::engine
