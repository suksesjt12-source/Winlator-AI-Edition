#include "aie/x64_interpreter.hpp"
#include "aie/guest_memory.hpp"

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



enum class AddressDecodeResult { Success, Truncated, Unsupported };

AddressDecodeResult decode_memory_address(
    const std::uint8_t* code, std::size_t code_size, std::size_t& cursor,
    std::uint8_t modrm, std::uint8_t rex,
    const X64CpuState& state, std::uint64_t& address) {
    const std::uint8_t mod = modrm >> 6;
    const std::uint8_t rm_field = modrm & 7U;
    if (mod == 3) return AddressDecodeResult::Unsupported;

    std::uint64_t base = 0;
    std::uint64_t index = 0;
    unsigned scale = 1;
    bool has_base = true;
    bool has_index = false;

    if (rm_field == 4) {
        if (cursor >= code_size) return AddressDecodeResult::Truncated;
        const std::uint8_t sib = code[cursor++];
        scale = 1U << (sib >> 6);
        const std::uint8_t index_field = (sib >> 3) & 7U;
        const std::uint8_t base_field = sib & 7U;
        const bool extend_index = (rex & 0x02U) != 0;
        const bool extend_base = (rex & 0x01U) != 0;

        if (index_field != 4U || extend_index) {
            const std::size_t index_register =
                index_field + (extend_index ? 8U : 0U);
            index = state.registers[index_register];
            has_index = true;
        }

        if (mod == 0 && base_field == 5U) {
            has_base = false; // SIB no-base form with a signed disp32.
        } else {
            const std::size_t base_register =
                base_field + (extend_base ? 8U : 0U);
            base = state.registers[base_register];
        }
    } else if (mod == 0 && rm_field == 5U) {
        // RIP-relative addressing is deliberately deferred until code has a
        // guest virtual base and instruction fetch uses the same address map.
        return AddressDecodeResult::Unsupported;
    } else {
        const std::size_t base_register =
            rm_field + ((rex & 0x01U) ? 8U : 0U);
        base = state.registers[base_register];
    }

    std::int64_t displacement = 0;
    if (mod == 1 || (mod == 0 && !has_base)) {
        if (mod == 1) {
            if (cursor >= code_size) return AddressDecodeResult::Truncated;
            displacement = static_cast<std::int8_t>(code[cursor++]);
        } else {
            if (cursor > code_size || 4U > code_size - cursor)
                return AddressDecodeResult::Truncated;
            displacement = static_cast<std::int32_t>(read32(code + cursor));
            cursor += 4U;
        }
    } else if (mod == 2) {
        if (cursor > code_size || 4U > code_size - cursor)
            return AddressDecodeResult::Truncated;
        displacement = static_cast<std::int32_t>(read32(code + cursor));
        cursor += 4U;
    }

    address = (has_base ? base : 0U) +
              (has_index ? index * static_cast<std::uint64_t>(scale) : 0U) +
              static_cast<std::uint64_t>(displacement);
    return AddressDecodeResult::Success;
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
                            X64CpuState state, std::size_t maximum_steps,
                            GuestMemory* guest_memory) {
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


        if (opcode == 0x8d) { // LEA reg, memory
            if (cursor >= code_size)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "LEA is missing its ModRM byte.");
            const std::uint8_t modrm = code[cursor++];
            if ((modrm >> 6) == 3)
                return stop(state, X64StopReason::UnsupportedInstruction, executed,
                            "LEA requires a memory addressing form.");
            const std::size_t reg =
                ((modrm >> 3) & 7U) + ((rex & 0x04U) ? 8U : 0U);
            std::uint64_t address = 0;
            const auto address_result = decode_memory_address(
                code, code_size, cursor, modrm, rex, state, address);
            if (address_result == AddressDecodeResult::Truncated)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "LEA address is truncated.");
            if (address_result != AddressDecodeResult::Success)
                return stop(state, X64StopReason::UnsupportedInstruction, executed,
                            "LEA uses an unsupported addressing mode.");
            const std::uint64_t mask = (rex & 0x08U)
                ? std::numeric_limits<std::uint64_t>::max() : 0xffffffffULL;
            state.registers[reg] = address & mask;
            state.rip = cursor;
            ++executed;
            continue;
        }

        if (opcode == 0xc7) { // MOV r/m32 or r/m64, imm32
            if (cursor >= code_size)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "MOV immediate is missing its ModRM byte.");
            const std::uint8_t modrm = code[cursor++];
            if (((modrm >> 3) & 7U) != 0 || (rex & 0x04U) != 0)
                return stop(state, X64StopReason::UnsupportedInstruction, executed,
                            "Only the C7 /0 MOV immediate form is implemented.");
            if ((modrm >> 6) != 3 && guest_memory == nullptr)
                return stop(state, X64StopReason::MemoryFault, executed,
                            "MOV immediate requires a guest-memory context.");
            std::uint64_t address = 0;
            if ((modrm >> 6) != 3) {
                const auto address_result = decode_memory_address(
                    code, code_size, cursor, modrm, rex, state, address);
                if (address_result == AddressDecodeResult::Truncated)
                    return stop(state, X64StopReason::TruncatedInstruction, executed,
                                "MOV immediate address is truncated.");
                if (address_result != AddressDecodeResult::Success)
                    return stop(state, X64StopReason::UnsupportedInstruction, executed,
                                "MOV immediate uses an unsupported addressing mode.");
            }
            if (cursor > code_size || 4U > code_size - cursor)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "MOV immediate data is truncated.");
            const bool wide = (rex & 0x08U) != 0;
            const std::uint32_t raw_immediate = read32(code + cursor);
            const std::uint64_t value = wide
                ? static_cast<std::uint64_t>(static_cast<std::int64_t>(
                      static_cast<std::int32_t>(raw_immediate)))
                : raw_immediate;
            const std::size_t operand_size = wide ? 8U : 4U;
            if ((modrm >> 6) == 3) {
                const std::size_t rm =
                    (modrm & 7U) + ((rex & 0x01U) ? 8U : 0U);
                state.registers[rm] = value;
            } else {
                std::uint8_t bytes[8]{};
                for (std::size_t i = 0; i < operand_size; ++i)
                    bytes[i] = static_cast<std::uint8_t>(value >> (i * 8U));
                if (!guest_memory->write(address, bytes, operand_size))
                    return stop(state, X64StopReason::MemoryFault, executed,
                                "MOV immediate store targeted unmapped or non-writable guest memory.");
            }
            state.rip = cursor + 4U;
            ++executed;
            continue;
        }

        if (opcode == 0x89 || opcode == 0x8b ||
            opcode == 0x01 || opcode == 0x03 ||
            opcode == 0x29 || opcode == 0x2b ||
            opcode == 0x31 || opcode == 0x33 ||
            opcode == 0x39 || opcode == 0x3b ||
            opcode == 0x85) {
            if (cursor >= code_size)
                return stop(state, X64StopReason::TruncatedInstruction, executed,
                            "Register/memory instruction is missing its ModRM byte.");
            const std::uint8_t modrm = code[cursor++];
            const std::uint8_t mod = modrm >> 6;
            const std::size_t reg =
                ((modrm >> 3) & 7U) + ((rex & 0x04U) ? 8U : 0U);
            const bool wide = (rex & 0x08U) != 0;
            const unsigned width = wide ? 64U : 32U;
            const std::size_t operand_size = wide ? 8U : 4U;
            const std::uint64_t mask = wide
                ? std::numeric_limits<std::uint64_t>::max() : 0xffffffffULL;

            const bool is_mov = opcode == 0x89 || opcode == 0x8b;
            const bool is_add = opcode == 0x01 || opcode == 0x03;
            const bool is_sub = opcode == 0x29 || opcode == 0x2b;
            const bool is_xor = opcode == 0x31 || opcode == 0x33;
            const bool is_cmp = opcode == 0x39 || opcode == 0x3b;
            const bool is_test = opcode == 0x85;

            if (mod != 3) {
                if (guest_memory == nullptr)
                    return stop(state, X64StopReason::MemoryFault, executed,
                                "Instruction requires a guest-memory context.");
                std::uint64_t address = 0;
                const auto address_result = decode_memory_address(
                    code, code_size, cursor, modrm, rex, state, address);
                if (address_result == AddressDecodeResult::Truncated)
                    return stop(state, X64StopReason::TruncatedInstruction, executed,
                                "Memory operand address is truncated.");
                if (address_result != AddressDecodeResult::Success)
                    return stop(state, X64StopReason::UnsupportedInstruction, executed,
                                "Instruction uses an unsupported memory addressing mode.");

                if (opcode == 0x89) {
                    std::uint8_t bytes[8]{};
                    const std::uint64_t value = state.registers[reg] & mask;
                    for (std::size_t i = 0; i < operand_size; ++i)
                        bytes[i] = static_cast<std::uint8_t>(value >> (i * 8U));
                    if (!guest_memory->write(address, bytes, operand_size))
                        return stop(state, X64StopReason::MemoryFault, executed,
                                    "MOV store targeted unmapped or non-writable guest memory.");
                } else if (opcode == 0x8b) {
                    std::uint8_t bytes[8]{};
                    if (!guest_memory->read(address, bytes, operand_size))
                        return stop(state, X64StopReason::MemoryFault, executed,
                                    "MOV load targeted unmapped or non-readable guest memory.");
                    std::uint64_t value = 0;
                    for (std::size_t i = 0; i < operand_size; ++i)
                        value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8U);
                    state.registers[reg] = value;
                } else {
                    std::uint8_t bytes[8]{};
                    if (!guest_memory->read(address, bytes, operand_size))
                        return stop(state, X64StopReason::MemoryFault, executed,
                                    "Arithmetic operand targeted unmapped or non-readable guest memory.");
                    std::uint64_t memory_value = 0;
                    for (std::size_t i = 0; i < operand_size; ++i)
                        memory_value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8U);

                    const std::uint64_t register_value = state.registers[reg] & mask;
                    const bool memory_is_destination =
                        opcode == 0x01 || opcode == 0x29 ||
                        opcode == 0x31 || opcode == 0x39 || is_test;
                    const std::uint64_t lhs = memory_is_destination
                        ? memory_value : register_value;
                    const std::uint64_t rhs = memory_is_destination
                        ? register_value : memory_value;
                    std::uint64_t value = lhs;
                    if (is_add) value = (lhs + rhs) & mask;
                    else if (is_sub || is_cmp) value = (lhs - rhs) & mask;
                    else if (is_xor || is_test) value = (lhs ^ rhs) & mask;

                    if (is_add) set_arithmetic_flags(state, value, lhs, rhs, width, false);
                    else if (is_sub || is_cmp)
                        set_arithmetic_flags(state, value, lhs, rhs, width, true);
                    else if (is_xor || is_test) set_logic_flags(state, value, width);

                    if (is_add || is_sub || is_xor) {
                        if (memory_is_destination) {
                            std::uint8_t out[8]{};
                            for (std::size_t i = 0; i < operand_size; ++i)
                                out[i] = static_cast<std::uint8_t>(value >> (i * 8U));
                            if (!guest_memory->write(address, out, operand_size))
                                return stop(state, X64StopReason::MemoryFault, executed,
                                            "Arithmetic store targeted non-writable guest memory.");
                        } else {
                            state.registers[reg] = value;
                        }
                    }
                }
                state.rip = cursor;
                ++executed;
                continue;
            }

            const std::size_t rm =
                (modrm & 7U) + ((rex & 0x01U) ? 8U : 0U);
            if (is_mov) {
                const std::size_t destination = opcode == 0x89 ? rm : reg;
                const std::size_t source = opcode == 0x89 ? reg : rm;
                state.registers[destination] = state.registers[source] & mask;
            } else {
                const bool rm_is_destination =
                    opcode == 0x01 || opcode == 0x29 ||
                    opcode == 0x31 || opcode == 0x39 || is_test;
                const std::uint64_t lhs =
                    state.registers[rm_is_destination ? rm : reg] & mask;
                const std::uint64_t rhs =
                    state.registers[rm_is_destination ? reg : rm] & mask;
                std::uint64_t value = lhs;
                if (is_add) value = (lhs + rhs) & mask;
                else if (is_sub || is_cmp) value = (lhs - rhs) & mask;
                else if (is_xor || is_test) value = (lhs ^ rhs) & mask;

                if (is_add) set_arithmetic_flags(state, value, lhs, rhs, width, false);
                else if (is_sub || is_cmp)
                    set_arithmetic_flags(state, value, lhs, rhs, width, true);
                else if (is_xor || is_test) set_logic_flags(state, value, width);

                if (is_add || is_sub || is_xor) {
                    const std::size_t destination = rm_is_destination ? rm : reg;
                    state.registers[destination] = value;
                }
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


X64RunResult run_x64_self_test() {
    // mov rax,40; push rax; mov rcx,2; pop rcx; mov rax,rcx;
    // call local_add_three; add rax,2; ret
    // local_add_three: add rax,3; ret
    static constexpr std::uint8_t program[] = {
        0x48, 0xb8, 0x28, 0, 0, 0, 0, 0, 0, 0,
        0x50,
        0x48, 0xb9, 0x02, 0, 0, 0, 0, 0, 0, 0,
        0x59,
        0x48, 0x89, 0xc8,
        0x48, 0x89, 0x43, 0x08,
        0x48, 0x8b, 0x4b, 0x08,
        0x48, 0x89, 0x4b, 0x10,
        0x48, 0x01, 0x4b, 0x10,
        0x48, 0x8b, 0x53, 0x10,
        0x48, 0x2b, 0xd1,
        0xe8, 0x07, 0, 0, 0,
        0x48, 0x05, 0x02, 0, 0, 0,
        0xc3,
        0x48, 0x05, 0x03, 0, 0, 0,
        0xc3
    };
    GuestMemory memory;
    std::string error;
    if (!memory.map(0x10000U, static_cast<std::size_t>(GUEST_PAGE_SIZE),
                    GuestMemoryRead | GuestMemoryWrite, &error)) {
        X64RunResult failed;
        failed.reason = X64StopReason::MemoryFault;
        failed.error = error;
        return failed;
    }
    X64CpuState initial;
    initial.registers[3] = 0x10000U; // RBX points at the mapped guest page.
    return run_x64_subset(program, sizeof(program), initial, 1000, &memory);
}

} // namespace aie::engine
