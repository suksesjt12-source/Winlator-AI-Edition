# Winlator AI Edition

**An Android Windows-compatibility project transitioning from an upstream Winlator frontend to an independently developed ARM64 execution engine.**

## Two development tracks

The current Android application is based on the upstream Winlator runtime. The custom AI Edition launcher and advisor are layered on top of that runtime. In parallel, a separate native engine core is developed under `engine/` as a possible long-term execution backend.

## Independent engine: current status

- Bounds-checked PE32 / PE32+ executable metadata parser.
- x86 and x86-64 machine-type detection.
- Entry-point, image-base and section-table inspection.
- PE header/section mapping, PE32 HIGHLOW and PE32+ DIR64 relocations.
- Import descriptor/thunk parsing through an explicit caller-supplied resolver.
- A deliberately small x86-64 interpreter subset with NOP/RET, MOV immediate/register/memory, LEA, C7 immediate stores, register/memory ADD/SUB/XOR/CMP/TEST, accumulator immediate arithmetic, short branches, PUSH/POP and relative CALL.
- A 4 KiB bounded interpreter-local stack with checked PUSH/POP/CALL/RET accesses.
- A bounded guest virtual-memory map with independent read/write/execute permissions per 4 KiB page, protection changes, unmapping and a configurable allocation cap.
- PE Loader integration that maps relocated headers and sections into GuestMemory, derives page permissions from PE section flags, keeps gaps inaccessible and rejects writable+executable pages.
- A limited PE entry-point test runner that fetches an executable section through GuestMemory and executes only the implemented x86-64 subset.
- Initial x86-64 memory operands using base-register and SIB effective addresses, including MOV/LEA and arithmetic/logical operations, with explicit faults for unmapped or protected guest memory. Instruction fetch permission is checked separately.
- JNI shared library integrated into the upstream-based APK. The AI Assistant can inspect selected .exe metadata and run a fixed, built-in x86-64 interpreter smoke test (registers, stack, arithmetic and local CALL/RET).
- Debug-mode native CMake tests for parsing, loading, relocations, import resolution and interpreter behavior.
- An Android NDK cross-build targeting `arm64-v8a`.

**Important:** this is not yet a working standalone Windows emulator. The import resolver does not provide Windows DLLs, and the CPU interpreter supports only a handful of instructions. There is no general x86/x64-to-ARM64 execution engine, Windows API layer, or independent graphics/audio/input compatibility. The JNI integration can run one fixed, built-in interpreter test program that exercises bounded guest-memory load/store, ADD/SUB arithmetic, stack operations and local CALL/RET. The PE-to-GuestMemory mapping and limited PE-entry test runner currently have native unit-test coverage; the Android UI still does not launch a selected .exe or run Windows applications.

## Android application today

- Custom AI Edition dashboard, cover art and application icon.
- Offline, rule-based device advisor.
- Existing container, shortcut, file-management, input and runtime component flows supplied by upstream Winlator.
- Distinct Android application ID: `com.winlator.aiedition`.
- JNI bridge included in the upstream build for PE metadata inspection and a fixed x86-64 interpreter self-test; the upstream Wine/Box64 runtime remains the actual Windows execution backend.

The current installable APK is still upstream-runtime-based. The native core is built as a JNI shared library inside that APK, but it is not yet a capable Windows execution engine and is not a replacement for Wine/Box64.

## Downloads and builds

- [GitHub Releases — Android debug APK](https://github.com/suksesjt12-source/Winlator-AI-Edition/releases)
- [GitHub Actions — Android and native-engine builds](https://github.com/suksesjt12-source/Winlator-AI-Edition/actions)

A successful CI build validates compilation and unit tests only. It does not prove that Windows games run on a physical device. Helio G100 / Mali-G57 hardware validation remains outstanding.

## Planned independent runtime layers

1. PE loader: map image sections, apply relocations, resolve imports, TLS and exception metadata.
2. x86/x86-64 execution: interpreter or JIT translation to ARM64.
3. Windows ABI and system services: processes, threads, memory, files, registry and synchronization.
4. Graphics, audio and input compatibility layers.
5. Android process/container manager, per-game configuration and diagnostics.

Each layer must have tests and explicit compatibility limits. No game-performance claim will be made before real-device validation.

## Licensing

Original project additions are under the MIT License. Upstream Winlator and third-party dependencies retain their own licenses and copyright notices, including LGPL-2.1 where applicable. Any future instruction-translation or compatibility dependency must retain its own license and notices.
