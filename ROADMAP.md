# Development Roadmap

This roadmap separates the existing upstream-based Android app from the new independent native execution engine.

## Track A — Android application and existing compatibility runtime
- [x] Android launcher, custom cover and application identity
- [x] Offline rule-based AI Assistant
- [x] Upstream Winlator runtime integration for containers, shortcuts and components
- [x] CI debug APK build and prerelease publication
- [ ] Replace remaining upstream-facing screens with AI Edition-owned UI
- [ ] Physical ARM64 installation and lifecycle validation
- [ ] Validate storage grants, containers, components, controls, audio and graphics on device

## Track B — Independent native execution engine

### Milestone 1: executable format inspection
- [x] Bounds-checked PE/COFF parser
- [x] PE32 x86 and PE32+ x86-64 detection
- [x] Parse entry-point RVA, preferred image base and section metadata
- [x] Reject malformed/truncated headers and out-of-file section ranges
- [x] Host-native CMake tests and GitHub Actions workflow

### Milestone 2: PE image loader
- [ ] Map PE headers and sections into a managed virtual address space
- [ ] Implement image-base selection and base relocations
- [ ] Parse import descriptors and resolve imports through a controlled module registry
- [ ] Handle TLS and define exception/unwind metadata strategy
- [ ] Add malformed-image, relocation and import-resolution tests

### Milestone 3: x86/x86-64 execution on ARM64
- [ ] Define CPU state, flags, memory access and exception model
- [ ] Implement a minimal interpreter for a documented instruction subset
- [ ] Add x86-64 decoding and instruction semantic tests
- [ ] Add ARM64 JIT only after interpreter correctness is established
- [ ] Add differential tests against known reference results

### Milestone 4: Windows ABI and system services
- [ ] Process and thread lifecycle
- [ ] Virtual memory and synchronization primitives
- [ ] File and directory APIs with Android storage-grant boundaries
- [ ] Dynamic module loading and core runtime libraries
- [ ] Registry and environment support
- [ ] Document unsupported APIs and compatibility levels

### Milestone 5: graphics, audio and input
- [ ] Define graphics device abstraction
- [ ] Implement a small, testable graphics API subset
- [ ] Vulkan/OpenGL backend selection for Mali-G57
- [ ] Audio and Android input bridges
- [ ] Frame pacing, surface lifecycle and suspend/resume tests

### Milestone 6: Android integration
- [ ] Build native engine for Android ARM64 using NDK
- [ ] JNI boundary with explicit ownership and error reporting
- [ ] Sandboxed per-game process/session management
- [ ] Connect AI Assistant to measured engine diagnostics
- [ ] Keep the current upstream runtime optional during migration

## Validation gates

- A parser test passing is not execution support.
- A PE image loading is not equivalent to Windows API compatibility.
- Running a small console test is not proof that complex 3D games work.
- No claim of EXE or game compatibility until a reproducible test passes on physical ARM64 hardware.

Initial hardware target: Android 14, MediaTek Helio G100, Mali-G57 MC2 and 8 GB RAM. This is a target for validation, not a claim of measured compatibility or performance.
