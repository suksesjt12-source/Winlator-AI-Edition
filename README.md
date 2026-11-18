# Winlator AI Edition

**An Android ARM64 build of upstream Winlator with an added offline AI Assistant screen.**

> **Current status:** The upstream-based debug APK compiled and uploaded successfully in GitHub Actions on 18 November 2026. It has not yet been installed or tested on a physical Helio G100 / Mali-G57 device. A successful CI build is not proof of game compatibility.

## Download the correct APK

For the actual Winlator runtime base, use the **Upstream Winlator AI Build** output, not the standalone prototype:

- [GitHub Releases — direct APK assets](https://github.com/suksesjt12-source/Winlator-AI-Edition/releases)
- [Latest upstream build workflow](https://github.com/suksesjt12-source/Winlator-AI-Edition/actions)

The workflow now publishes the APK itself and a SHA-256 checksum as prerelease assets. Open the newest **Winlator AI Edition Debug** prerelease and download **app-debug.apk** directly; no ZIP extraction is required. The Actions artifact remains available as a separate backup.

Latest verified direct APK: [app-debug.apk — ai-debug-8](https://github.com/suksesjt12-source/Winlator-AI-Edition/releases/download/ai-debug-8/app-debug.apk). The matching checksum file is [app-debug.apk.sha256](https://github.com/suksesjt12-source/Winlator-AI-Edition/releases/download/ai-debug-8/app-debug.apk.sha256).

This build uses package ID `com.winlator.aiedition` and FileProvider authority `com.winlator.aiedition.FileProvider`, so it is designed to coexist with a standard Winlator install that uses `com.winlator`. It targets ARM64 (`arm64-v8a`) and Android API 26 or later.

The separate **Android CI** artifact named **Winlator-AI-Edition-debug** is only the original standalone prototype. It is not the Winlator runtime build and cannot run Windows EXE files.

## Included in the upstream-based build

- Upstream Winlator Android application and its existing container, shortcut, file-management and input-control flows.
- Upstream native application modules checked out through the original Git submodules, including Vortek and Gladio.
- An added AI Assistant screen reachable from the Winlator navigation drawer.
- Local device information: Android version, ABI, SoC model when exposed by Android, and memory information.
- Conservative offline guidance for graphics compatibility and starting performance profiles.

## Important limitations

- The artifact is a **debug build**, not a signed release.
- The APK build has succeeded in CI, but installation and game execution have not yet been verified on physical hardware.
- Wine, Box64, graphics translation layers and other runtime components remain subject to the upstream Winlator installation and component-management flow. The build does not bundle every game-specific dependency or guarantee every component is already installed.
- The added assistant is an offline rule-based advisor. It is not a connected large language model and does not automatically change container settings, CPU/GPU clocks or Android system settings.
- Do not assume Turnip is suitable for Mali GPUs. Graphics drivers must match the device GPU and the renderer available in the upstream build.

## Build workflows

- **Upstream Winlator AI Build:** builds the upstream Winlator source, applies the AI Assistant extension, uploads the debug artifact, and publishes the APK plus SHA-256 checksum as prerelease assets.
- **Android CI:** builds the original standalone prototype and uploads its separate debug artifact.

The upstream workflow uses JDK 17 and installs Android SDK/NDK/CMake build dependencies on the GitHub runner.

## Target device

Initial target: Android 14, ARM64, MediaTek Helio G100 / Mali-G57 class hardware, 8 GB RAM. This is a development target, not a statement that the APK has already been verified on that device.

## Licensing

Original project additions are under the MIT License. The upstream Winlator application and native dependencies retain their own licenses and notices, including LGPL-2.1 where applicable. Third-party runtime components retain their individual licenses.
