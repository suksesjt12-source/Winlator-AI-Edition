#include "aie/pe_image.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

static void put16(std::vector<std::uint8_t>& b, std::size_t p, std::uint16_t v) {
    b[p] = static_cast<std::uint8_t>(v);
    b[p + 1] = static_cast<std::uint8_t>(v >> 8);
}
static void put32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b[p + i] = static_cast<std::uint8_t>(v >> (i * 8));
}
static void put64(std::vector<std::uint8_t>& b, std::size_t p, std::uint64_t v) {
    put32(b, p, static_cast<std::uint32_t>(v));
    put32(b, p + 4, static_cast<std::uint32_t>(v >> 32));
}

static std::vector<std::uint8_t> make_pe64() {
    std::vector<std::uint8_t> b(512, 0);
    b[0] = 'M'; b[1] = 'Z';
    put32(b, 0x3c, 0x80);
    b[0x80] = 'P'; b[0x81] = 'E';
    put16(b, 0x84, 0x8664);
    put16(b, 0x86, 1);
    put16(b, 0x94, 112);
    const std::size_t opt = 0x98;
    put16(b, opt, 0x20b);
    put32(b, opt + 16, 0x1000);
    put64(b, opt + 24, 0x140000000ULL);
    const std::size_t sec = opt + 112;
    const char name[] = ".text";
    for (int i = 0; i < 5; ++i) b[sec + i] = static_cast<std::uint8_t>(name[i]);
    put32(b, sec + 8, 0x20);
    put32(b, sec + 12, 0x1000);
    put32(b, sec + 16, 0x20);
    put32(b, sec + 20, 0x180);
    put32(b, sec + 36, 0x60000020);
    return b;
}

int main() {
    const auto valid = aie::engine::inspect_pe_image(make_pe64());
    assert(valid.ok);
    assert(valid.image.architecture == aie::engine::PeArchitecture::X86_64);
    assert(valid.image.entry_point_rva == 0x1000);
    assert(valid.image.preferred_image_base == 0x140000000ULL);
    assert(valid.image.sections.size() == 1);
    assert(valid.image.sections[0].name == ".text");

    const std::vector<std::uint8_t> invalid{0, 1, 2, 3};
    assert(!aie::engine::inspect_pe_image(invalid).ok);

    auto malformed = make_pe64();
    put32(malformed, 0x3c, 0xfffffff0);
    assert(!aie::engine::inspect_pe_image(malformed).ok);

    std::cout << "PE image parser tests passed\n";
    return 0;
}
