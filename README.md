# Winlator AI Edition

**An Android Windows-compatibility project transitioning from an upstream Winlator frontend to an independently developed ARM64 execution engine.**

## Two development tracks

The existing Android application still uses the upstream Winlator runtime. In parallel, a separate native engine core is now being developed under `engine/`. The native core is intended to become the long-term execution backend; it is not yet capable of running Windows applications.

## Independent engine: current status

- Bounds-checked PE32 / PE32+ executable metadata parser.
- x86 and x86-64 machine-type detection.
- Entry-point, image-base and section-table inspection.
- Native CMake test target and GitHub Actions test workflow.

**Important:** the current native engine is an initial loader-analysis milestone, not a working Windows emulator. It does not yet map or execute PE images, translate x86/x64 instructions to ARM64, implement Windows APIs, or provide Direct3D/audio/input compatibility.

## Android application today

- Custom AI Edition dashboard, cover art and application icon.
- Offline, rule-based device advisor.
- Existing container, shortcut, file-management, input and runtime component flows supplied by upstream Winlator.
- Distinct Android application ID: `com.winlator.aiedition`.

The current installable APK is still upstream-runtime-based. The independent native engine is tested separately as a host-native C++ component and has not yet been integrated as an Android execution backend.

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
