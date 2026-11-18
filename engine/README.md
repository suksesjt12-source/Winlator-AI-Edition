# AI Edition Native Execution Engine

This directory starts the independent native engine track. It is deliberately separate from the upstream Winlator integration.

## Current implementation

- A bounds-checked PE/COFF metadata inspector for PE32 x86 and PE32+ x86-64 executables.
- Machine/format validation, entry-point RVA and preferred image-base extraction.
- Section-table parsing and raw-section range validation.
- Host-side CMake tests for valid and malformed PE files.

## Not implemented yet

This is **not an executable Windows runtime**. It does not map PE sections into process memory, apply relocations, resolve imports, execute x86 instructions, provide Windows APIs, or implement graphics/audio/input. Selecting an EXE in the Android UI must not be presented as proof that it can run.

## Planned engine layers

1. PE loader: image mapping, relocations, imports, TLS and exception metadata.
2. x86/x86-64 execution: a documented interpreter/JIT or a separately licensed translation backend, with ARM64 host integration.
3. Windows ABI and system services: process/thread, memory, files, registry, synchronization and loader APIs.
4. Graphics/audio/input: Direct3D-to-Vulkan/OpenGL translation, audio backend and Android input bridge.
5. Android process/container manager: per-game sandbox, lifecycle, storage grants and diagnostics.

The target is an ARM64 Android device. The first hardware validation target is Helio G100 / Mali-G57, but no performance or game-compatibility claim is made until real-device tests pass.
