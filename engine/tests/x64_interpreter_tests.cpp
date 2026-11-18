#include "aie/x64_interpreter.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    // mov rax,5; add rax,3; sub rax,1; ret
    const std::vector<std::uint8_t> program{
        0x48, 0xb8, 0x05, 0, 0, 0, 0, 0, 0, 0,
        0x48, 0x05, 0x03, 0, 0, 0,
        0x48, 0x2d, 0x01, 0, 0, 0,
        0xc3
    };
    const auto result = aie::engine::run_x64_subset(program.data(), program.size());
    assert(result.returned());
    assert(result.state.registers[0] == 7);
    assert(result.instructions_executed == 4);

    // A 32-bit register write must clear the upper half of the 64-bit register.
    const std::vector<std::uint8_t> mov32{0xb8, 0xff, 0xff, 0xff, 0xff, 0xc3};
    aie::engine::X64CpuState initial;
    initial.registers[0] = 0xffffffffffffffffULL;
    const auto zero_extend =
        aie::engine::run_x64_subset(mov32.data(), mov32.size(), initial);
    assert(zero_extend.returned());
    assert(zero_extend.state.registers[0] == 0xffffffffULL);

    const std::vector<std::uint8_t> truncated{0x48, 0xb8, 0x01, 0x02};
    const auto bad = aie::engine::run_x64_subset(truncated.data(), truncated.size());
    assert(bad.reason == aie::engine::X64StopReason::TruncatedInstruction);

    const std::vector<std::uint8_t> unsupported{0xcc};
    const auto unknown =
        aie::engine::run_x64_subset(unsupported.data(), unsupported.size());
    assert(unknown.reason == aie::engine::X64StopReason::UnsupportedInstruction);

    const std::vector<std::uint8_t> no_return{0x90};
    const auto limited =
        aie::engine::run_x64_subset(no_return.data(), no_return.size(), {}, 1);
    assert(limited.reason == aie::engine::X64StopReason::StepLimitReached);

    std::cout << "x86-64 interpreter subset tests passed\n";
    return 0;
}
