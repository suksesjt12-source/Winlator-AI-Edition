# AI Edition native engine core

This is a separate C++17 execution-engine foundation, built for host tests and Android ARM64 cross-compilation.

Current implemented layers:
- Bounds-checked PE32 / PE32+ metadata inspection for supported x86 and x86-64 images.
- Bounded PE image mapping, PE32 HIGHLOW and PE32+ DIR64 relocation handling, and import resolution through an explicit caller-provided callback.
- A deliberately small x86-64 interpreter subset: NOP, MOV immediate/register, XOR register, ADD/SUB/CMP RAX immediate, short JMP/JE/JNE, register/immediate PUSH, register POP, near relative CALL, and nested RET.
- A 4 KiB bounded interpreter-local stack with checked PUSH/POP/CALL/RET accesses.
- A bounded guest virtual-memory map with page-aligned mappings, read/write/execute permissions, protect/unmap operations and a configurable total allocation limit. Instruction fetch checks execute permission separately from data reads.
- Initial 32/64-bit MOV loads and stores for register-indirect, displacement and SIB effective-address forms; unmapped or disallowed access returns an explicit memory fault.
- CMake tests for parsing, loading, relocations, imports, supported instructions, branches, stack operations, local function calls and guest-memory protections.
- A fixed built-in x86-64 smoke-test program, exposed to Android through JNI; it verifies register results and stack restoration without executing user-provided PE code.

Important limitations:
- This is not a general x86-64 CPU emulator. Most instruction families, RIP-relative memory operands, floating-point/SIMD, privilege behavior, interrupts and precise exception semantics are not implemented.
- The interpreter's stack is a private 4 KiB test sandbox. GuestMemory is an early bounded byte-map and is not yet a Windows process address space, page-table implementation or demand-paged memory manager.
- PE imports still require an externally supplied resolver. No Windows DLL exports or Windows API implementation is provided.
- The JNI bridge can inspect PE metadata and invoke one fixed, built-in x86-64 smoke test with guest-memory loads and stores from Android UI. It does not execute the selected executable or arbitrary supplied instruction bytes.
- There is no x86-64-to-ARM64 JIT, process/thread model, TLS callback execution, Windows ABI, graphics, audio or input compatibility layer.

A passing unit test or ARM64 compile is not evidence that arbitrary Windows applications or games can run.
