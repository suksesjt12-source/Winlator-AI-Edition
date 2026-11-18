# Development Roadmap

This roadmap tracks the work required to turn the current Android prototype into a usable Winlator-based application.

## Phase 1 — Android foundation
- [x] Android application project and launcher screen
- [x] GitHub Actions debug APK build
- [x] Device and memory information screen
- [x] Local rule-based assistant
- [x] Initial advisory performance profiles

## Phase 2 — Game library and per-game settings
- [x] Android Storage Access Framework picker for `.exe` files
- [x] Persist selected document URIs without broad storage permissions
- [x] Store a profile for each selected game
- [ ] Validate file-provider edge cases on real devices
- [ ] Add editable game metadata and profile export/import

## Phase 3 — Integrate the actual Windows compatibility runtime
- [ ] Use the upstream Winlator Android project as the compatibility base
- [ ] Preserve upstream source attribution and LGPL-2.1 obligations
- [ ] Build and package compatible ARM64 native components
- [ ] Add a component catalog with version, ABI, source URL and checksum verification
- [ ] Install required Wine, Box64 and graphics components into the application data area
- [ ] Handle installation failure, insufficient storage, cancellation and resume safely

## Phase 4 — Container and executable workflow
- [ ] Create, list, configure and remove Wine containers
- [ ] Import an executable into a selected container
- [ ] Create per-game launch shortcuts
- [ ] Launch the selected executable through the real Winlator runtime
- [ ] Preserve container isolation and user data

## Phase 5 — Device-aware graphics and performance
- [ ] Detect Android ABI, SoC, GPU and Vulkan/OpenGL capabilities
- [ ] Apply conservative renderer defaults for Mali-class and Adreno-class GPUs
- [ ] Add resolution, DX wrapper, audio and Box64 presets by game
- [ ] Persist settings and provide rollback to known-safe defaults
- [ ] Monitor frame rate and thermal status only where Android exposes reliable data

## Phase 6 — AI assistance
- [ ] Provide offline rule-based diagnosis and setup guidance
- [ ] Parse user-approved Winlator logs and explain likely causes
- [ ] Recommend reversible per-game configuration changes
- [ ] Make any cloud AI integration optional, opt-in and independent of core game execution

## Phase 7 — Verification and release
- [ ] Build debug and signed release APKs reproducibly
- [ ] Run unit tests and Android lint checks
- [ ] Install and launch on physical ARM64 hardware
- [ ] Verify container creation, component installation and a legally obtained test executable
- [ ] Verify controls, audio, graphics, storage permissions, suspend/resume and thermal behaviour
- [ ] Publish release notes, checksums and third-party notices

## Definition of done

The project is considered ready for an initial public release only when a release APK can be installed, create a container, install or select the required runtime components, and launch a supported Windows test application on physical ARM64 hardware. A successful Gradle build alone is not sufficient.

No compatibility promise can cover every Windows game. Results depend on the CPU instruction set, Wine version, Box64 build, GPU driver, graphics API and the game itself.
