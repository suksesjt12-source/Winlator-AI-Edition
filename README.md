# Winlator AI Edition

**Android ARM64 launcher prototype with a local gaming assistant and conservative profile recommendations.**

> **Status: Phase 1.** This is an Android application shell and a GitHub Actions debug-APK pipeline. It is not yet a complete Windows compatibility layer and cannot currently launch Windows `.exe` games.

## Included in Phase 1
- Device information: Android version, supported ABI, device identifiers and available memory.
- Local assistant that explains compatibility and performance trade-offs without an API key.
- Advisory profiles: Balanced, Performance and Cool & Stable.
- Initial game-profile screen.
- No root requirement and no changes to Android system settings.
- GitHub Actions workflow to build and upload a debug APK.

## Build APK
1. Open the **Actions** tab in this repository.
2. Select **Android CI**.
3. Open the latest successful run.
4. Download the `Winlator-AI-Edition-debug` artifact.

The build uses JDK 17, Gradle 8.9 and Android Gradle Plugin 8.7.3.

## Important technical scope
A launcher UI cannot execute Windows programs by itself. Real `.exe` support requires compatible ARM64 builds and integration of Wine, Box64 and suitable graphics translation components (for example DXVK for supported Direct3D paths). Those components must be integrated, licensed, packaged and tested separately. They are **not** bundled in this prototype.

The assistant in this phase is a local rule-based helper, not a connected large language model. Its optimization output is advisory and does not overclock the device or alter system settings.

## Initial target
Android 14, ARM64, MediaTek Helio G100 / Mali-G57 class hardware, 8 GB RAM. Performance recommendations must be validated per game and runtime; they are not guaranteed FPS results.

## License
Project source is provided under the MIT License. Third-party runtime components retain their own licenses and are not bundled by this prototype.
