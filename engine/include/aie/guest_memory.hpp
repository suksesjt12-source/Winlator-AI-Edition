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

// A bounded, page-aligned guest virtual-memory map. This is an engine-owned
// byte store for tests and early execution work, not the Android process map.
class GuestMemory {
public:
    explicit GuestMemory(std::size_t maximum_mapped_bytes = 64U * 1024U * 1024U);

    bool map(std::uint64_t base, std::size_t size, std::uint8_t permissions,
             std::string* error = nullptr);
    bool unmap(std::uint64_t base, std::size_t size,
               std::string* error = nullptr);
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
        std::uint8_t permissions{};
        std::vector<std::uint8_t> bytes;
    };

    Region* find_region(std::uint64_t address, std::size_t size);
    const Region* find_region(std::uint64_t address, std::size_t size) const;

    std::size_t maximum_mapped_bytes_;
    std::vector<Region> regions_;
};

} // namespace aie::engine
