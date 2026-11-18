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
- [x] Map PE headers and sections into a bounded virtual byte array
- [x] Select load base and apply PE32 HIGHLOW / PE32+ DIR64 relocations
- [x] Parse import descriptors and resolve entries through a caller-supplied resolver callback
- [ ] Handle TLS and define exception/unwind metadata strategy
- [x] Add malformed-image, relocation and import-resolution tests

### Milestone 3: x86/x86-64 execution on ARM64
- [x] Define initial x86-64 GPR, RIP and RFLAGS state
- [x] Implement initial interpreter subset: NOP, MOV immediate/register, XOR register, ADD/SUB/CMP accumulator immediate and short branches
- [x] Add bounded 4 KiB interpreter-local stack with register/immediate PUSH and register POP
- [x] Add near relative CALL and nested RET handling
- [x] Add semantic tests for stack bounds and local function calls
- [x] Add short JMP/JE/JNE branch handling and semantic tests
- [x] Add initial mapped-entry byte-sequence test
- [ ] Expand instruction decoding, memory operands, stack frames and exception behavior
- [x] Compile and package the native core into the Android APK through a JNI shared library (metadata inspection only)
- [x] Invoke a fixed, built-in x86-64 smoke-test program through JNI (not user-supplied executable bytes)
- [ ] Add ARM64 JIT only after broad interpreter correctness is established
- [ ] Add differential tests against a trusted reference implementation

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
- [x] Build and package native engine sources for Android ARM64 using NDK
- [x] JNI boundary for bounded PE metadata inspection and fixed interpreter smoke test
- [ ] JNI execution interface with explicit memory ownership and error reporting
- [ ] Sandboxed per-game process/session management
- [ ] Connect AI Assistant to measured engine diagnostics
- [ ] Keep the current upstream runtime optional during migration

## Validation gates

- A parser test passing is not execution support.
- A PE image loading is not equivalent to Windows API compatibility.
- Running a small console test is not proof that complex 3D games work.
- No claim of EXE or game compatibility until a reproducible test passes on physical ARM64 hardware.

Initial hardware target: Android 14, MediaTek Helio G100, Mali-G57 MC2 and 8 GB RAM. This is a target for validation, not a claim of measured compatibility or performance.
