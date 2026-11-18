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

void set_logic_flags(X64CpuState& state, std::uint64_t result, unsigned width) {
    const std::uint64_t mask = width == 64
        ? std::numeric_limits<std::uint64_t>::max() : 0xffffffffULL;
    const std::uint64_t sign = width == 64 ? (1ULL << 63) : (1ULL << 31);
    result &= mask;
    state.rflags &= ~ARITHMETIC_FLAGS;
    state.rflags |= 0x2;
    if (even_parity(static_cast<std::uint8_t>(result))) state.rflags |= FLAG_PF;
    if (result == 0) state.rflags |= FLAG_ZF;
    if ((result & sign) != 0) state.rflags |= FLAG_SF;
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


bool push64(X64CpuState& state, std::uint64_t value) {
    std::uint64_t& rsp = state.registers[4];
    if (rsp > X64_STACK_SIZE || rsp < 8U) return false;
    rsp -= 8U;
    for (unsigned i = 0; i < 8; ++i)
        state.stack_memory[static_cast<std::size_t>(rsp) + i] =
            static_cast<std::uint8_t>(value >> (i * 8U));
    return true;
}

bool pop64(X64CpuState& state, std::uint64_t& value) {
    std::uint64_t& rsp = state.registers[4];
    if (rsp > X64_STACK_SIZE - 8U) return false;
    value = read64(state.stack_memory.data() + rsp);
    rsp += 8U;
    return true;
}

} // namespace

X64RunResult run_x64_subset(const std::uint8_t* code, std::size_t code_size,
                            X64CpuState state, std::size_t maximum_steps) {
    if (code == nullptr || state.rip >= code_size)
        return stop(state, X64StopReason::InvalidInstructionPointer, 0,
                    "Instruction pointer is outside the code buffer.");

    if (state.registers[4] == 0)
        state.registers[4] = X64_STACK_SIZE;

    std::size_t executed = 0;
    std::size_t call_depth = 0;
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
        if (opcode == 0x90) {
            state.rip = cursor;
            ++executed;
            continue;
        }
        if (opcode == 0xc3) {
            if (call_depth == 0)
                return stop(state, X64StopReason::Returned, executed + 1U, "");
            std::uint64_t return_address = 0;
            if (!pop64(state, return_address))
                return stop(state, X64StopReason::StackFault, executed,
                            "RET could not read a return address from the sandbox stack.");
            if (return_address >= code_size)
                return stop(state, X64StopReason::InvalidInstructionPointer, executed,
                            "RET target is outside the code buffer.");
            --call_depth;
            state.rip = return_address;
            ++executed;
            continue;
        }

        if (opcode >= 0x50 && opcode <= 0x57) { // PUSH r64
            const std::size_t reg = (opcode - 0x50U) + ((rex & 0x01U) ? 8U : 0U);
            const std::uint64_t value = state.registers[reg];
            if (!push64(state, value))
                return stop(state, X64StopReason::StackFault, executed,
                            "PUSH exceeded the bounded sandbox stack.");
            state.rip = cursor;
            ++executed;
            continue;
        }

        if (opcode >= 0x58 && opcode <= 0x5f) { // POP r64
            const std::size_t reg = (opcode - 0x58U) + ((rex & 0x01U) ? 8U : 0U);
            std::uint64_t value = 0;
            if (!pop64(state, value))
                return stop(state, X64StopReason::StackFault, executed,
                            "POP attempted to read beyond the bounded sandbox stack.");
            state.registers[reg] = value;
            state.rip = cursor;
            ++executed;
            continue;
        }

        if (opcode == 0x6a || opcode == 0x68) { // PUSH imm8 / imm32
            const std::size_t immediate_size = opcode == 0x6a ? 1U : 4U;
            if (cursor > code_size || immediate_size > code_size - cursor)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "PUSH immediate is truncated.");
            const std::int64_t signed_value = opcode == 0x6a
                ? static_cast<std::int8_t>(code[cursor])
                : static_cast<std::int32_t>(read32(code + cursor));
            if (!push64(state, static_cast<std::uint64_t>(signed_value)))
                return stop(state, X64StopReason::StackFault, executed,
                            "PUSH immediate exceeded the bounded sandbox stack.");
            state.rip = cursor + immediate_size;
            ++executed;
            continue;
        }

        if (opcode == 0xe8 && rex == 0) { // CALL rel32
            if (cursor > code_size || 4U > code_size - cursor)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "CALL displacement is truncated.");
            const std::int32_t displacement =
                static_cast<std::int32_t>(read32(code + cursor));
            const std::size_t next = cursor + 4U;
            const std::int64_t destination =
                static_cast<std::int64_t>(next) + displacement;
            if (destination < 0 ||
                static_cast<std::uint64_t>(destination) >= code_size)
                return stop(state, X64StopReason::InvalidInstructionPointer, executed,
                            "CALL target is outside the code buffer.");
            if (!push64(state, next))
                return stop(state, X64StopReason::StackFault, executed,
                            "CALL could not push a return address onto the sandbox stack.");
            ++call_depth;
            state.rip = static_cast<std::uint64_t>(destination);
            ++executed;
            continue;
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


        if (opcode == 0x89 || opcode == 0x8b ||
            opcode == 0x31 || opcode == 0x33) {
            if (cursor >= code_size)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "Register instruction is missing its ModRM byte.");
            const std::uint8_t modrm = code[cursor++];
            const std::uint8_t mod = modrm >> 6;
            if (mod != 3)
                return stop(state, X64StopReason::UnsupportedInstruction, executed,
                            "Memory-addressing ModRM forms are not implemented.");
            const std::size_t reg = ((modrm >> 3) & 7U) + ((rex & 0x04U) ? 8U : 0U);
            const std::size_t rm = (modrm & 7U) + ((rex & 0x01U) ? 8U : 0U);
            const bool wide = (rex & 0x08U) != 0;
            const unsigned width = wide ? 64U : 32U;
            const std::uint64_t mask = wide
                ? std::numeric_limits<std::uint64_t>::max() : 0xffffffffULL;
            const bool writes_rm = opcode == 0x89 || opcode == 0x31;
            const std::size_t destination = writes_rm ? rm : reg;
            const std::uint64_t left = state.registers[destination] & mask;
            const std::uint64_t right =
                state.registers[writes_rm ? reg : rm] & mask;

            if (opcode == 0x89 || opcode == 0x8b) {
                state.registers[destination] = right;
            } else {
                const std::uint64_t value = (left ^ right) & mask;
                state.registers[destination] = value;
                set_logic_flags(state, value, width);
            }
            state.rip = cursor;
            ++executed;
            continue;
        }

        if (opcode == 0x3d) { // CMP accumulator, imm32
            if (cursor > code_size || 4U > code_size - cursor)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "CMP immediate is truncated.");
            const bool wide = (rex & 0x08U) != 0;
            const unsigned width = wide ? 64U : 32U;
            const std::uint64_t lhs = state.registers[0];
            const std::uint32_t immediate = read32(code + cursor);
            const std::uint64_t rhs = wide
                ? static_cast<std::uint64_t>(static_cast<std::int64_t>(
                      static_cast<std::int32_t>(immediate)))
                : immediate;
            const std::uint64_t raw = lhs - rhs;
            const std::uint64_t value =
                width == 64 ? raw : (raw & 0xffffffffULL);
            set_arithmetic_flags(state, value, lhs, rhs, width, true);
            state.rip = cursor + 4U;
            ++executed;
            continue;
        }

        if ((opcode == 0x74 || opcode == 0x75 || opcode == 0xeb) && rex == 0) {
            if (cursor >= code_size)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "Short branch displacement is truncated.");
            const auto displacement = static_cast<std::int8_t>(code[cursor++]);
            bool take = opcode == 0xeb;
            if (opcode == 0x74) take = (state.rflags & FLAG_ZF) != 0;
            if (opcode == 0x75) take = (state.rflags & FLAG_ZF) == 0;
            const std::int64_t next = static_cast<std::int64_t>(cursor);
            const std::int64_t target = take ? next + displacement : next;
            if (target < 0 || static_cast<std::uint64_t>(target) >= code_size)
                return stop(state, X64StopReason::InvalidInstructionPointer, executed,
                            "Short branch target is outside the code buffer.");
            state.rip = static_cast<std::uint64_t>(target);
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
