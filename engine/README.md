# AI Edition native engine core

This is a separate C++17 execution-engine foundation, built for host tests and Android ARM64 cross-compilation.

Current implemented layers:
- Bounds-checked PE32 / PE32+ metadata inspection for supported x86 and x86-64 images.
- Bounded PE image mapping, PE32 HIGHLOW and PE32+ DIR64 relocation handling, and import resolution through an explicit caller-provided callback.
- A deliberately small x86-64 interpreter subset: NOP, MOV immediate/register, XOR register, ADD/SUB/CMP RAX immediate, short JMP/JE/JNE, register/immediate PUSH, register POP, near relative CALL, and nested RET.
- A 4 KiB bounded interpreter-local stack with checked PUSH/POP/CALL/RET accesses.
- CMake tests for parsing, loading, relocations, imports, supported instructions, branches, stack operations and local function calls.

Important limitations:
- This is not a general x86-64 CPU emulator. ModRM memory operands, most instruction families, floating-point/SIMD, privilege behavior, interrupts and precise exception semantics are not implemented.
- The interpreter's stack is a private 4 KiB test sandbox, not a Windows process stack or a general process virtual-memory model.
- PE imports still require an externally supplied resolver. No Windows DLL exports or Windows API implementation is provided.
- The JNI bridge currently inspects PE metadata only. It does not execute the selected executable or invoke this interpreter from Android UI.
- There is no x86-64-to-ARM64 JIT, process/thread model, TLS callback execution, Windows ABI, graphics, audio or input compatibility layer.

A passing unit test or ARM64 compile is not evidence that arbitrary Windows applications or games can run.
