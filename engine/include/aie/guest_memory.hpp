#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace aie::engine {

constexpr std::uint64_t GUEST_PAGE_SIZE = 4096;

enum GuestMemoryPermission : std::uint8_t {
    GuestMemoryNone = 0,
    GuestMemoryRead = 1U << 0,
    GuestMemoryWrite = 1U << 1,
    GuestMemoryExecute = 1U << 2
};

// Engine-owned, bounded guest virtual memory. Permissions are tracked per
// 4 KiB page; this is not an Android process map or a Windows VM subsystem.
class GuestMemory {
public:
    explicit GuestMemory(std::size_t maximum_mapped_bytes = 64U * 1024U * 1024U);

    bool map(std::uint64_t base, std::size_t size, std::uint8_t permissions,
             std::string* error = nullptr);
    bool unmap(std::uint64_t base, std::size_t size,
               std::string* error = nullptr);
    // Protects a page-aligned subrange of one existing mapping.
    bool protect(std::uint64_t base, std::size_t size, std::uint8_t permissions,
                 std::string* error = nullptr);

    bool read(std::uint64_t address, void* destination, std::size_t size) const;
    bool fetch(std::uint64_t address, void* destination, std::size_t size) const;
    bool write(std::uint64_t address, const void* source, std::size_t size);

    std::size_t mapped_bytes() const;
    std::size_t maximum_mapped_bytes() const { return maximum_mapped_bytes_; }

private:
    struct Region {
        std::uint64_t base{};
        std::size_t size{};
        std::vector<std::uint8_t> page_permissions;
        std::vector<std::uint8_t> bytes;
    };

    Region* find_region(std::uint64_t address, std::size_t size);
    const Region* find_region(std::uint64_t address, std::size_t size) const;
    bool has_permission(const Region& region, std::uint64_t address,
                        std::size_t size, std::uint8_t permission) const;

    std::size_t maximum_mapped_bytes_;
    std::vector<Region> regions_;
};

} // namespace aie::engine
