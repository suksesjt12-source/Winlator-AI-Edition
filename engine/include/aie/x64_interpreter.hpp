#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace aie::engine {

constexpr std::size_t X64_STACK_SIZE = 4096;

struct X64CpuState {
    // Register order: RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8-R15.
    std::array<std::uint64_t, 16> registers{};
    std::uint64_t rip{};
    std::uint64_t rflags{0x2};

    // Private, bounded stack for this interpreter sandbox. RSP is an offset
    // into this array; a zero initial RSP is initialized to X64_STACK_SIZE.
    std::array<std::uint8_t, X64_STACK_SIZE> stack_memory{};
};

enum class X64StopReason {
    Returned,
    UnsupportedInstruction,
    TruncatedInstruction,
    InvalidInstructionPointer,
    StackFault,
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
// Supports NOP, RET, MOV immediate/register, XOR register, ADD/SUB/CMP RAX
// immediate, short JMP/JE/JNE, register/immediate PUSH and register POP,
// and near relative CALL. CALL/RET use the bounded sandbox stack above.
// General memory operands, Windows APIs, process memory and a full ABI are
// intentionally unsupported. This is not a Windows runtime.
X64RunResult run_x64_subset(const std::uint8_t* code, std::size_t code_size,
                            X64CpuState initial_state = {},
                            std::size_t maximum_steps = 100000);

} // namespace aie::engine
