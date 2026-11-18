#include "aie/guest_memory.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    using namespace aie::engine;

    GuestMemory memory(2U * static_cast<std::size_t>(GUEST_PAGE_SIZE));
    std::string error;

    assert(memory.map(0x10000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
                      GuestMemoryRead | GuestMemoryWrite, &error));
    assert(memory.mapped_bytes() == GUEST_PAGE_SIZE);

    // Mappings must be page-aligned, non-overlapping and within the configured cap.
    assert(!memory.map(0x10001U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
                       GuestMemoryRead, &error));
    assert(!memory.map(0x10000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
                       GuestMemoryRead, &error));
    assert(!memory.map(0x20000U, 2U * static_cast<std::size_t>(GUEST_PAGE_SIZE),
                       GuestMemoryRead, &error));

    // Newly mapped memory is zero-filled and supports bounded read/write.
    std::array<std::uint8_t, 8> bytes{};
    assert(memory.read(0x10020U, bytes.data(), bytes.size()));
    for (std::uint8_t value : bytes) assert(value == 0);

    const std::array<std::uint8_t, 8> expected{
        0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
    };
    assert(memory.write(0x10020U, expected.data(), expected.size()));
    bytes.fill(0);
    assert(memory.read(0x10020U, bytes.data(), bytes.size()));
    assert(bytes == expected);

    // Accesses crossing an unmapped boundary or targeting an unmapped page fail.
    assert(!memory.read(0x10ffcU, bytes.data(), bytes.size()));
    assert(!memory.write(0x30000U, expected.data(), expected.size()));

    // Protection changes are enforced by both read and write operations.
    assert(memory.protect(0x10000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
                          GuestMemoryRead, &error));
    assert(!memory.write(0x10020U, expected.data(), expected.size()));
    assert(memory.read(0x10020U, bytes.data(), bytes.size()));

    assert(memory.protect(0x10000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
                          GuestMemoryNone, &error));
    assert(!memory.read(0x10020U, bytes.data(), bytes.size()));

    // Execute permission is distinct from ordinary read permission.
    assert(memory.map(0x20000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
                      GuestMemoryExecute, &error));
    std::uint8_t instruction = 0;
    assert(memory.fetch(0x20000U, &instruction, 1));
    assert(!memory.read(0x20000U, &instruction, 1));
    assert(!memory.write(0x20000U, &instruction, 1));

    // Only exact mapped regions can be unmapped.
    assert(!memory.unmap(0x10000U, 128U, &error));
    assert(memory.unmap(0x10000U, static_cast<std::size_t>(GUEST_PAGE_SIZE), &error));
    assert(memory.unmap(0x20000U, static_cast<std::size_t>(GUEST_PAGE_SIZE), &error));
    assert(memory.mapped_bytes() == 0);

    // Per-page permissions can differ inside one contiguous mapping.
    GuestMemory paged_memory(2U * static_cast<std::size_t>(GUEST_PAGE_SIZE));
    assert(paged_memory.map(0x30000U, 2U * static_cast<std::size_t>(GUEST_PAGE_SIZE),
                            GuestMemoryRead | GuestMemoryWrite, &error));
    assert(paged_memory.protect(
        0x31000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
        GuestMemoryRead, &error));
    const std::uint8_t marker = 0x5a;
    assert(paged_memory.write(0x30000U, &marker, 1));
    assert(!paged_memory.write(0x31000U, &marker, 1));
    assert(paged_memory.read(0x31000U, &instruction, 1));
    assert(paged_memory.protect(
        0x31000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
        GuestMemoryExecute, &error));
    assert(paged_memory.fetch(0x31000U, &instruction, 1));
    assert(!paged_memory.read(0x31000U, &instruction, 1));
    assert(!paged_memory.write(0x31000U, &marker, 1));

    std::cout << "guest memory tests passed\n";
    return 0;
}
