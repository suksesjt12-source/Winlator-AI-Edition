#include "aie/x64_interpreter.hpp"

#include <cstdint>
#include <limits>

namespace aie::engine {
namespace {

constexpr std::uint64_t FLAG_CF = 1ULL << 0;
constexpr std::uint64_t FLAG_PF = 1ULL << 2;
constexpr std::uint64_t FLAG_AF = 1ULL << 4;
constexpr std::uint64_t FLAG_ZF = 1ULL << 6;
constexpr std::uint64_t FLAG_SF = 1ULL << 7;
constexpr std::uint64_t FLAG_OF = 1ULL << 11;
constexpr std::uint64_t ARITHMETIC_FLAGS =
    FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF | FLAG_OF;

std::uint32_t read32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}
std::uint64_t read64(const std::uint8_t* p) {
    return static_cast<std::uint64_t>(read32(p)) |
           (static_cast<std::uint64_t>(read32(p + 4)) << 32);
}
bool even_parity(std::uint8_t value) {
    unsigned ones = 0;
    for (unsigned i = 0; i < 8; ++i) ones += (value >> i) & 1U;
    return (ones & 1U) == 0;
}
void set_arithmetic_flags(X64CpuState& state, std::uint64_t result,
                          std::uint64_t lhs, std::uint64_t rhs,
                          unsigned width, bool subtract) {
    const std::uint64_t mask = width == 64
        ? std::numeric_limits<std::uint64_t>::max() : 0xffffffffULL;
    const std::uint64_t sign = width == 64 ? (1ULL << 63) : (1ULL << 31);
    result &= mask;
    lhs &= mask;
    rhs &= mask;

    bool carry = false;
    bool overflow = false;
    if (subtract) {
        carry = lhs < rhs;
        overflow = (((lhs ^ rhs) & (lhs ^ result) & sign) != 0);
    } else {
        carry = result < lhs;
        overflow = (((~(lhs ^ rhs)) & (lhs ^ result) & sign) != 0);
    }
    const bool auxiliary = ((lhs ^ rhs ^ result) & 0x10U) != 0;

    state.rflags &= ~ARITHMETIC_FLAGS;
    state.rflags |= 0x2;
    if (carry) state.rflags |= FLAG_CF;
    if (even_parity(static_cast<std::uint8_t>(result))) state.rflags |= FLAG_PF;
    if (auxiliary) state.rflags |= FLAG_AF;
    if (result == 0) state.rflags |= FLAG_ZF;
    if ((result & sign) != 0) state.rflags |= FLAG_SF;
    if (overflow) state.rflags |= FLAG_OF;
}

X64RunResult stop(X64CpuState state, X64StopReason reason,
                  std::size_t count, const char* message) {
    X64RunResult result;
    result.state = state;
    result.reason = reason;
    result.instructions_executed = count;
    result.error = message;
    return result;
}

} // namespace

X64RunResult run_x64_subset(const std::uint8_t* code, std::size_t code_size,
                            X64CpuState state, std::size_t maximum_steps) {
    if (code == nullptr || state.rip >= code_size)
        return stop(state, X64StopReason::InvalidInstructionPointer, 0,
                    "Instruction pointer is outside the code buffer.");

    std::size_t executed = 0;
    while (executed < maximum_steps) {
        if (state.rip >= code_size)
            return stop(state, X64StopReason::InvalidInstructionPointer, executed,
                        "Instruction pointer moved outside the code buffer.");

        std::size_t cursor = static_cast<std::size_t>(state.rip);
        std::uint8_t rex = 0;
        if (code[cursor] >= 0x40 && code[cursor] <= 0x4f) {
            rex = code[cursor++];
            if (cursor >= code_size)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "REX prefix has no opcode.");
        }

        const std::uint8_t opcode = code[cursor++];
        if (opcode == 0x90 && rex == 0) {
            state.rip = cursor;
            ++executed;
            continue;
        }
        if (opcode == 0xc3 && rex == 0) {
            state.rip = cursor;
            ++executed;
            return stop(state, X64StopReason::Returned, executed, "");
        }

        if (opcode >= 0xb8 && opcode <= 0xbf) {
            const bool wide = (rex & 0x08U) != 0;
            const std::size_t immediate_size = wide ? 8U : 4U;
            if (cursor > code_size || immediate_size > code_size - cursor)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "MOV immediate is truncated.");
            const std::size_t reg = (opcode - 0xb8U) + ((rex & 0x01U) ? 8U : 0U);
            const std::uint64_t value = wide ? read64(code + cursor)
                                             : read32(code + cursor);
            state.registers[reg] = value; // A 32-bit write zero-extends to 64 bits.
            state.rip = cursor + immediate_size;
            ++executed;
            continue;
        }

        if (opcode == 0x05 || opcode == 0x2d) {
            if (cursor > code_size || 4U > code_size - cursor)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "Arithmetic immediate is truncated.");
            const bool wide = (rex & 0x08U) != 0;
            const unsigned width = wide ? 64U : 32U;
            const std::uint64_t lhs = state.registers[0];
            const std::uint32_t immediate = read32(code + cursor);
            const std::uint64_t rhs = wide
                ? static_cast<std::uint64_t>(static_cast<std::int64_t>(
                      static_cast<std::int32_t>(immediate)))
                : immediate;
            const bool subtract = opcode == 0x2d;
            const std::uint64_t raw = subtract ? lhs - rhs : lhs + rhs;
            const std::uint64_t result = width == 64 ? raw : (raw & 0xffffffffULL);
            state.registers[0] = result;
            set_arithmetic_flags(state, result, lhs, rhs, width, subtract);
            state.rip = cursor + 4U;
            ++executed;
            continue;
        }

        return stop(state, X64StopReason::UnsupportedInstruction, executed,
                    "Opcode is not implemented by the current x86-64 interpreter subset.");
    }

    return stop(state, X64StopReason::StepLimitReached, executed,
                "Interpreter instruction limit reached before RET.");
}

} // namespace aie::engine
