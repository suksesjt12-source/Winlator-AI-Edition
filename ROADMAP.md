# Development Roadmap

This roadmap tracks the work required to turn the project into a tested Winlator-based Android application with a distinct launcher and additional AI assistance.

## Phase 1 — Android foundation
- [x] Android application project and launcher prototype
- [x] GitHub Actions debug APK build
- [x] Device and memory information screen
- [x] Local rule-based assistant
- [x] Initial advisory performance profiles

## Phase 2 — Standalone prototype library
- [x] Android Storage Access Framework picker for EXE files
- [x] Persist selected document URIs without broad storage permissions
- [x] Store a profile for each selected game
- [ ] Validate file-provider edge cases on real devices
- [ ] Add editable game metadata and profile export/import

## Phase 3 — Upstream Winlator integration and AI Edition launcher
- [x] Use upstream Winlator Android project as the compatibility base in CI
- [x] Preserve upstream source attribution and original Git submodule references
- [x] Add the offline AI Assistant Activity
- [x] Replace the first-launch screen with an AI Edition dashboard
- [x] Add custom launcher cover art and adaptive application icon
- [x] Route dashboard actions to the upstream game shortcuts, container and settings flows
- [x] Keep the upstream runtime available behind the new launcher
- [ ] Build and install the redesigned APK on physical ARM64 hardware
- [ ] Verify first-run permissions, system files and component installation

## Phase 4 — Runtime and executable validation
- [x] Include upstream container creation and management implementation
- [x] Include upstream executable shortcut and file-management implementation
- [ ] Verify creating and configuring a container on a physical device
- [ ] Verify installing/selecting runtime components in the container
- [ ] Verify launching a legally obtained Windows test executable
- [ ] Verify container isolation and persistence across app restarts

## Phase 5 — Device-aware graphics and performance
- [ ] Detect Android ABI, SoC, GPU and Vulkan/OpenGL capabilities reliably
- [ ] Apply conservative renderer guidance for Mali-class and Adreno-class GPUs
- [ ] Add tested resolution, DX wrapper, audio and Box64 recommendations by game
- [ ] Persist recommendations and provide rollback to known-safe settings
- [ ] Monitor frame rate and thermal status only where Android exposes reliable data
- [ ] Validate performance on Helio G100 / Mali-G57 hardware

## Phase 6 — AI assistance
- [x] Provide initial offline device information and rule-based setup guidance
- [ ] Parse user-approved Winlator logs and explain likely causes
- [ ] Recommend reversible per-game configuration changes
- [ ] Connect recommendations to actual container settings with explicit user approval
- [ ] Keep any cloud AI integration optional and opt-in

## Phase 7 — Verification and release
- [x] Build and upload an upstream-based debug APK reproducibly in GitHub Actions
- [x] Publish debug APK and SHA-256 checksum as prerelease assets
- [ ] Run Android lint and targeted automated tests
- [ ] Install and launch on physical ARM64 hardware
- [ ] Verify container creation, component installation and a Windows test executable
- [ ] Verify controls, audio, graphics, storage permissions, suspend/resume and thermal behaviour
- [ ] Produce a signed release APK, release notes, checksums and third-party notices

## Definition of done

The project is ready for an initial public release only when a release APK can be installed, create a container, install or select the required runtime components, and launch a supported Windows test application on physical ARM64 hardware. A successful Gradle build alone is not sufficient.

No compatibility promise can cover every Windows game. Results depend on the CPU instruction set, Wine version, Box64 build, GPU driver, graphics API and the game itself.
