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

    // Register-to-register MOV/XOR using register-only ModRM forms.
    const std::vector<std::uint8_t> register_ops{
        0xbb, 0x03, 0, 0, 0,
        0x48, 0x89, 0xd8,
        0x48, 0x31, 0xd8,
        0xc3
    };
    const auto register_result =
        aie::engine::run_x64_subset(register_ops.data(), register_ops.size());
    assert(register_result.returned());
    assert(register_result.state.registers[0] == 0);

    // CMP + JE skips MOV EAX,9 and executes MOV EAX,7.
    const std::vector<std::uint8_t> branch_ops{
        0xb8, 0x03, 0, 0, 0,
        0x48, 0x3d, 0x03, 0, 0, 0,
        0x74, 0x05,
        0xb8, 0x09, 0, 0, 0,
        0xb8, 0x07, 0, 0, 0,
        0xc3
    };
    const auto branch_result =
        aie::engine::run_x64_subset(branch_ops.data(), branch_ops.size());
    assert(branch_result.returned());
    assert(branch_result.state.registers[0] == 7);

    // PUSH/POP preserve a 64-bit value and restore RSP.
    const std::vector<std::uint8_t> stack_ops{
        0x48, 0xb8, 0x2a, 0, 0, 0, 0, 0, 0, 0,
        0x50,
        0x48, 0xb8, 0x07, 0, 0, 0, 0, 0, 0, 0,
        0x59,
        0xc3
    };
    const auto stack_result =
        aie::engine::run_x64_subset(stack_ops.data(), stack_ops.size());
    assert(stack_result.returned());
    assert(stack_result.state.registers[0] == 7);
    assert(stack_result.state.registers[1] == 42);
    assert(stack_result.state.registers[4] == aie::engine::X64_STACK_SIZE);

    // Extended-register POP and sign-extended PUSH imm8.
    const std::vector<std::uint8_t> extended_stack{0x6a, 0xff, 0x41, 0x58, 0xc3};
    const auto extended_result =
        aie::engine::run_x64_subset(extended_stack.data(), extended_stack.size());
    assert(extended_result.returned());
    assert(extended_result.state.registers[8] == 0xffffffffffffffffULL);

    // PUSH imm32 sign-extends to 64 bits.
    const std::vector<std::uint8_t> immediate_stack{
        0x68, 0xff, 0xff, 0xff, 0xff,
        0x58,
        0xc3
    };
    const auto immediate_result = aie::engine::run_x64_subset(
        immediate_stack.data(), immediate_stack.size());
    assert(immediate_result.returned());
    assert(immediate_result.state.registers[0] == 0xffffffffffffffffULL);

    // REX.B selects R8 for both PUSH and POP.
    const std::vector<std::uint8_t> extended_register_stack{
        0x49, 0xb8, 0x2a, 0, 0, 0, 0, 0, 0, 0,
        0x41, 0x50,
        0x49, 0xb8, 0x07, 0, 0, 0, 0, 0, 0, 0,
        0x41, 0x58,
        0xc3
    };
    const auto extended_register_result = aie::engine::run_x64_subset(
        extended_register_stack.data(), extended_register_stack.size());
    assert(extended_register_result.returned());
    assert(extended_register_result.state.registers[8] == 42);

    // CALL a local function; nested RET resumes the caller before the outer RET.
    const std::vector<std::uint8_t> call_ops{
        0xe8, 0x06, 0, 0, 0,
        0xb8, 0x07, 0, 0, 0,
        0xc3,
        0xb8, 0x2a, 0, 0, 0,
        0xc3
    };
    const auto call_result =
        aie::engine::run_x64_subset(call_ops.data(), call_ops.size());
    assert(call_result.returned());
    assert(call_result.state.registers[0] == 7);
    assert(call_result.state.registers[4] == aie::engine::X64_STACK_SIZE);
    assert(call_result.instructions_executed == 5);

    // An invalid stack pointer must fail safely instead of indexing the array.
    aie::engine::X64CpuState bad_stack_state;
    bad_stack_state.registers[4] = 4;
    const std::vector<std::uint8_t> push_ops{0x50, 0xc3};
    const auto stack_fault = aie::engine::run_x64_subset(
        push_ops.data(), push_ops.size(), bad_stack_state);
    assert(stack_fault.reason == aie::engine::X64StopReason::StackFault);

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
