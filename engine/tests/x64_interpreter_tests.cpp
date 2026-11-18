#include "aie/x64_interpreter.hpp"
#include "aie/guest_memory.hpp"

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

    // MOV [RBX+disp8],RAX and MOV RCX,[RBX+disp8] use bounded guest memory.
    aie::engine::GuestMemory guest_memory(4096);
    assert(guest_memory.map(0x40000U, 4096,
        aie::engine::GuestMemoryRead | aie::engine::GuestMemoryWrite));
    const std::vector<std::uint8_t> memory_ops{
        0x48, 0xb8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
        0x48, 0x89, 0x43, 0x08,
        0x48, 0x8b, 0x4b, 0x08,
        0xc3
    };
    aie::engine::X64CpuState memory_state;
    memory_state.registers[3] = 0x40000U;
    const auto memory_result = aie::engine::run_x64_subset(
        memory_ops.data(), memory_ops.size(), memory_state, 1000, &guest_memory);
    assert(memory_result.returned());
    assert(memory_result.state.registers[1] == 0x1122334455667788ULL);

    // ADD/SUB/XOR/CMP/TEST support register and guest-memory operands.
    const std::vector<std::uint8_t> arithmetic_memory_ops{
        0x48, 0xbb, 0x00, 0x00, 0x04, 0x00, 0, 0, 0, 0, // mov rbx,0x40000
        0x48, 0xb8, 0x0a, 0, 0, 0, 0, 0, 0, 0,         // mov rax,10
        0x48, 0x89, 0x43, 0x08,                       // mov [rbx+8],rax
        0x48, 0xb9, 0x05, 0, 0, 0, 0, 0, 0, 0,       // mov rcx,5
        0x48, 0x01, 0x4b, 0x08,                       // add [rbx+8],rcx
        0x48, 0x8b, 0x53, 0x08,                       // mov rdx,[rbx+8]
        0x48, 0x2b, 0xd1,                             // sub rdx,rcx
        0x48, 0x33, 0xd0,                             // xor rdx,rax
        0x48, 0x85, 0xc2,                             // test rdx,rax
        0x48, 0x8d, 0x4b, 0x10,                       // lea rcx,[rbx+0x10]
        0xc7, 0x43, 0x20, 0x78, 0x56, 0x34, 0x12,     // mov dword [rbx+0x20],0x12345678
        0x8b, 0x43, 0x20,                             // mov eax,[rbx+0x20]
        0xc3
    };
    const auto arithmetic_result = aie::engine::run_x64_subset(
        arithmetic_memory_ops.data(), arithmetic_memory_ops.size(),
        memory_state, 1000, &guest_memory);
    assert(arithmetic_result.returned());
    assert(arithmetic_result.state.registers[0] == 0x12345678U);
    assert(arithmetic_result.state.registers[1] == 0x40010U);
    assert(arithmetic_result.state.registers[2] == 0);
    assert((arithmetic_result.state.rflags & (1ULL << 6)) != 0); // ZF from TEST.
    std::uint64_t stored_sum = 0;
    assert(guest_memory.read(0x40008U, &stored_sum, sizeof(stored_sum)));
    assert(stored_sum == 15U);

    // SIB addressing: [RBX + RCX*4 + 0x10].
    const std::vector<std::uint8_t> sib_ops{
        0x48, 0xb8, 0x2a, 0, 0, 0, 0, 0, 0, 0,
        0x48, 0x89, 0x44, 0x8b, 0x10,
        0x48, 0x8b, 0x4c, 0x8b, 0x10,
        0xc3
    };
    memory_state.registers[1] = 3;
    const auto sib_result = aie::engine::run_x64_subset(
        sib_ops.data(), sib_ops.size(), memory_state, 1000, &guest_memory);
    assert(sib_result.returned());
    assert(sib_result.state.registers[1] == 42);

    // Read-only mappings reject guest stores with an explicit memory fault.
    assert(guest_memory.protect(0x40000U, 4096, aie::engine::GuestMemoryRead));
    const std::vector<std::uint8_t> faulting_store{
        0x48, 0x89, 0x43, 0x08, 0xc3
    };
    const auto memory_fault = aie::engine::run_x64_subset(
        faulting_store.data(), faulting_store.size(), memory_state, 1000, &guest_memory);
    assert(memory_fault.reason == aie::engine::X64StopReason::MemoryFault);
    const std::vector<std::uint8_t> readonly_arithmetic{
        0x48, 0xb9, 0x01, 0, 0, 0, 0, 0, 0, 0,
        0x48, 0x01, 0x4b, 0x08,
        0xc3
    };
    const auto arithmetic_fault = aie::engine::run_x64_subset(
        readonly_arithmetic.data(), readonly_arithmetic.size(),
        memory_state, 1000, &guest_memory);
    assert(arithmetic_fault.reason == aie::engine::X64StopReason::MemoryFault);


    // The JNI smoke test includes a guest-memory store and load.
    const auto self_test = aie::engine::run_x64_self_test();
    assert(self_test.returned());
    assert(self_test.state.registers[0] == 45);
    assert(self_test.state.registers[1] == 40);
    assert(self_test.state.registers[2] == 40);
    assert(self_test.state.registers[4] == aie::engine::X64_STACK_SIZE);

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
