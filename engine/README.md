# AI Edition Native Execution Engine

This is the independent native-engine development track. It is integrated into the upstream-based Android APK as a JNI shared library for status reporting and PE metadata inspection. It remains separate from the Wine/Box64 compatibility runtime used to launch Windows applications.

## Implemented core

- Bounds-checked PE32 x86 and PE32+ x86-64 metadata inspection.
- PE image mapping into a bounded, zero-initialized virtual byte array.
- Header and section copying with range and overlap checks.
- PE32 HIGHLOW and PE32+ DIR64 base relocations.
- Import descriptor/thunk parsing with a caller-provided symbol resolver.
- A small x86-64 interpreter subset: NOP, RET, MOV immediate/register, XOR register, ADD/SUB/CMP RAX immediate, and short JMP/JE/JNE.
- A test that runs a tiny synthetic entry-point sequence from mapped PE section bytes.
- Debug-mode CMake tests for valid, malformed, relocation, import, branch and interpreter cases.
- Android NDK cross-build targeting arm64-v8a.
- JNI shared library packaged into the upstream-based Android APK. The Java assistant can request PE32/PE32+ metadata inspection of a user-selected .exe file.

## Explicit limitations

This is still **not a complete Windows emulator**. The import resolver is a callback; no Windows DLL exports are implemented. The interpreter is intentionally a tiny instruction subset and is not a general x86-64 CPU. The PE loader does not automatically transfer control to a loaded image's entry point. The synthetic test does not demonstrate execution of a real Windows program. JNI currently exposes status reporting and executable metadata inspection only; it does not execute the selected file. Android CI confirms compilation, not correct behavior on a physical device.

Not implemented: full x86/x64 instruction decoding, JIT, Windows process/thread model, system DLLs, Windows API layer, TLS callbacks, exception dispatch, graphics, audio, input, and game compatibility. The existing upstream Wine/Box64 runtime remains responsible for Windows application execution.

## Planned layers

1. Expand PE loader coverage: TLS, exception/unwind metadata, delay imports, more relocation types and robust malformed-file corpus.
2. Expand x86/x64 CPU semantics and differential tests, then evaluate a separately licensed decoder/JIT backend.
3. Implement a Windows ABI/system-service compatibility layer and module registry.
4. Add graphics/audio/input backends and connect them to Android surfaces.
5. Develop the native execution path independently from the upstream Wine/Box64 path, adding device validation only after the CPU and OS layers are sufficient.

## Safety and validation gates

- Parsing a PE is not loading it.
- Mapping and relocating an image is not executing it.
- Executing a small instruction subset is not Windows compatibility.
- Imported addresses must come only from an explicit, trusted resolver.
- No game compatibility or performance claim is valid until reproducible tests pass on physical ARM64 hardware.

The target hardware is Android ARM64, initially Helio G100 / Mali-G57. That is a validation target, not a statement that this code currently runs Windows software on the phone.
