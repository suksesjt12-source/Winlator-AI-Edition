# Winlator AI Edition

**An Android Windows-compatibility project transitioning from an upstream Winlator frontend to an independently developed ARM64 execution engine.**

## Two development tracks

The existing Android application still uses the upstream Winlator runtime. In parallel, a separate native engine core is being developed under `engine/` as the intended long-term execution backend. The native core now maps PE images and executes a tiny x86-64 instruction subset in tests, but it cannot yet run ordinary Windows applications.

## Independent engine: current status

- Bounds-checked PE32 / PE32+ executable metadata parser.
- x86 and x86-64 machine-type detection.
- Entry-point, image-base and section-table inspection.
- PE header/section mapping, PE32 HIGHLOW and PE32+ DIR64 relocations.
- Import descriptor/thunk parsing through an explicit caller-supplied resolver.
- A deliberately small x86-64 interpreter subset (NOP, RET, MOV immediate/register, XOR register, ADD/SUB/CMP RAX immediate, short JMP/JE/JNE).
- JNI bridge packaged into the Android APK for native engine status and PE metadata inspection.
- Debug-mode native CMake tests for parsing, loading, relocations, import resolution and interpreter behavior.
- An Android NDK cross-compile job targeting `arm64-v8a` that publishes the native static library as a CI artifact.

**Important:** this is not yet a working Windows emulator. The import resolver does not provide Windows DLLs, and the CPU interpreter supports only a handful of instructions. There is no full x86/x64-to-ARM64 execution engine, Windows API layer, or graphics/audio/input compatibility. JNI integration is limited to reporting native status and inspecting PE headers.

## Android application today

- Custom AI Edition dashboard, cover art and application icon.
- Offline, rule-based device advisor.
- Existing container, shortcut, file-management, input and runtime component flows supplied by upstream Winlator.
- Distinct Android application ID: `com.winlator.aiedition`.

The current installable APK is still upstream-runtime-based. The independent native engine is built/tested separately and its ARM64 static library is not yet linked into the APK or called through JNI.

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
