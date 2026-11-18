#include "aie/pe_loader.hpp"
#include "aie/x64_interpreter.hpp"

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
    std::vector<std::uint8_t> b(0x2c0, 0);
    b[0] = 'M'; b[1] = 'Z';
    put32(b, 0x3c, 0x80);
    b[0x80] = 'P'; b[0x81] = 'E';
    put16(b, 0x84, 0x8664);
    put16(b, 0x86, 3);
    put16(b, 0x94, 240);
    const std::size_t opt = 0x98;
    put16(b, opt, 0x20b);
    put32(b, opt + 16, 0x1000);
    put64(b, opt + 24, 0x140000000ULL);
    put32(b, opt + 32, 0x1000);
    put32(b, opt + 36, 0x200);
    put32(b, opt + 56, 0x4000);
    put32(b, opt + 60, 0x200);
    put32(b, opt + 108, 16);
    put32(b, opt + 112 + 8, 0x3000);
    put32(b, opt + 112 + 12, 40);
    put32(b, opt + 112 + 5 * 8, 0x2000);
    put32(b, opt + 112 + 5 * 8 + 4, 12);

    const std::size_t sec = opt + 240;
    const char text_name[] = ".text";
    for (int i = 0; i < 5; ++i) b[sec + i] = static_cast<std::uint8_t>(text_name[i]);
    put32(b, sec + 8, 0x20);
    put32(b, sec + 12, 0x1000);
    put32(b, sec + 16, 0x20);
    put32(b, sec + 20, 0x200);
    put32(b, sec + 36, 0x60000020);

    const std::size_t reloc = sec + 40;
    const char reloc_name[] = ".reloc";
    for (int i = 0; i < 6; ++i) b[reloc + i] = static_cast<std::uint8_t>(reloc_name[i]);
    put32(b, reloc + 8, 0x20);
    put32(b, reloc + 12, 0x2000);
    put32(b, reloc + 16, 0x20);
    put32(b, reloc + 20, 0x220);
    put32(b, reloc + 36, 0x42000040);

    const std::size_t idata = sec + 80;
    const char idata_name[] = ".idata";
    for (int i = 0; i < 6; ++i) b[idata + i] = static_cast<std::uint8_t>(idata_name[i]);
    put32(b, idata + 8, 0x80);
    put32(b, idata + 12, 0x3000);
    put32(b, idata + 16, 0x80);
    put32(b, idata + 20, 0x240);
    put32(b, idata + 36, 0xc0000040);

    // A tiny x64 test sequence at the image entry point: MOV EAX,42; RET.
    b[0x200] = 0xb8;
    b[0x201] = 42;
    b[0x205] = 0xc3;
    put64(b, 0x208, 0x140001234ULL);
    put32(b, 0x220, 0x1000);
    put32(b, 0x224, 12);
    put16(b, 0x228, static_cast<std::uint16_t>((10U << 12) | 8U));
    put16(b, 0x22a, 0);

    // Two import descriptors occupy 0x3000-0x3028. Tables follow them.
    put32(b, 0x240, 0x3038); // OriginalFirstThunk
    put32(b, 0x24c, 0x3028); // Name
    put32(b, 0x250, 0x3048); // FirstThunk
    const char module[] = "KERNEL32.dll";
    for (std::size_t i = 0; i < sizeof(module); ++i) b[0x268 + i] = module[i];
    put64(b, 0x278, 0x3058);
    put64(b, 0x280, 0);
    put64(b, 0x288, 0);
    put16(b, 0x298, 0);
    const char function_name[] = "Beep";
    for (std::size_t i = 0; i < sizeof(function_name); ++i) b[0x29a + i] = function_name[i];
    return b;
}

int main() {
    const auto file = make_pe64();
    const std::uint64_t preferred = 0x140000000ULL;
    const std::uint64_t actual = preferred + 0x100000ULL;

    const auto missing_resolver = aie::engine::map_pe_image(file, actual);
    assert(!missing_resolver.ok);

    const auto loaded = aie::engine::map_pe_image(
        file, actual,
        [](const std::string& module, const aie::engine::ImportedSymbol& symbol)
            -> std::optional<std::uint64_t> {
            if (module == "KERNEL32.dll" && !symbol.by_ordinal && symbol.name == "Beep")
                return 0x7fff12345678ULL;
            return std::nullopt;
        });

    assert(loaded.ok);
    assert(loaded.image.memory.size() == 0x4000);
    assert(loaded.image.entry_point() == actual + 0x1000);
    // Original 0x140001234 plus the 0x100000 load delta = 0x140101234.
    assert(loaded.image.memory[0x1008] == 0x34);
    assert(loaded.image.memory[0x1009] == 0x12);
    assert(loaded.image.memory[0x100a] == 0x10);
    assert(loaded.image.memory[0x100b] == 0x40);
    // Execute only the deliberately supported instruction subset from the mapped entry bytes.
    const auto entry_test = aie::engine::run_x64_subset(
        loaded.image.memory.data() + 0x1000, 0x20);
    assert(entry_test.returned());
    assert(entry_test.state.registers[0] == 42);

    // The resolver's function address is written to the IAT.
    assert(loaded.image.memory[0x3048] == 0x78);
    assert(loaded.image.memory[0x3049] == 0x56);
    assert(loaded.image.memory[0x304a] == 0x34);
    assert(loaded.image.memory[0x304b] == 0x12);

    const auto no_rebase = aie::engine::map_pe_image(file, preferred,
        [](const std::string&, const aie::engine::ImportedSymbol&)
            -> std::optional<std::uint64_t> { return 0x1122334455667788ULL; });
    assert(no_rebase.ok);
    assert(no_rebase.image.memory[0x100a] == 0);

    std::cout << "PE image loader tests passed\n";
    return 0;
}
