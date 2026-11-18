#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace aie::engine {

struct X64CpuState {
    // Register order: RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8-R15.
    std::array<std::uint64_t, 16> registers{};
    std::uint64_t rip{};
    std::uint64_t rflags{0x2};
};

enum class X64StopReason {
    Returned,
    UnsupportedInstruction,
    TruncatedInstruction,
    InvalidInstructionPointer,
    StepLimitReached
};

struct X64RunResult {
    X64CpuState state;
    X64StopReason reason{X64StopReason::UnsupportedInstruction};
    std::size_t instructions_executed{};
    std::string error;

    bool returned() const { return reason == X64StopReason::Returned; }
};

// Executes a deliberately small, testable x86-64 subset from a byte buffer.
// Supports a small subset: NOP, RET, MOV immediate/register, XOR register,
// ADD/SUB/CMP RAX immediate, and short JMP/JE/JNE. ModRM memory operands are
// intentionally unsupported. This is not a Windows runtime.
X64RunResult run_x64_subset(const std::uint8_t* code, std::size_t code_size,
                            X64CpuState initial_state = {},
                            std::size_t maximum_steps = 100000);

} // namespace aie::engine
