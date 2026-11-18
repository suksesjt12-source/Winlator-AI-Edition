# Winlator AI Edition

**An upstream Winlator ARM64 runtime with a redesigned AI Edition launcher, custom artwork and an offline device advisor.**

## Current development focus

The app now has its own Android launch screen rather than opening directly into the original Winlator navigation screen. The new home dashboard provides entry points for Game Library, Containers, AI Assistant, and Components & Settings. The Windows compatibility runtime remains based on upstream Winlator.

## Visual identity

- Custom AI Edition cover artwork is used in the launcher hero and Android adaptive application icon.
- New dark navy interface with cyan and violet accents.
- The new LauncherActivity is the Android MAIN/LAUNCHER activity. The original Winlator MainActivity is retained as an internal runtime workspace.

## Download

- [GitHub Releases — direct APK assets](https://github.com/suksesjt12-source/Winlator-AI-Edition/releases)
- [GitHub Actions — upstream build](https://github.com/suksesjt12-source/Winlator-AI-Edition/actions)

Open the newest prerelease and download `app-debug.apk` directly. The workflow also publishes a SHA-256 checksum. Debug APKs are development builds and may require allowing installation from the browser or file manager.

## What is included

- Upstream Winlator Android application and its container, shortcut, file-manager, input-control and runtime component flows.
- A custom launcher dashboard and app identity.
- An AI Assistant screen with device and memory information plus offline, rule-based setup guidance.
- ARM64 build target and a distinct Android application ID: `com.winlator.aiedition`.

## Important limitations

- The launcher redesign does not replace the underlying Wine, Box64 or graphics translation engines. Those remain from the upstream Winlator project.
- The Game Library, Containers and Components & Settings actions open the upstream runtime's corresponding management screens. The new AI Edition dashboard is the first screen; a complete replacement of every internal management screen is a separate development phase.
- The offline assistant provides recommendations only. It does not automatically change CPU/GPU clocks or modify Android system settings.
- Wine, Box64, DXVK, drivers and other runtime components may still need to be installed through the app's component-management flow.
- CI compilation does not prove installation or game compatibility on a physical device. Helio G100 / Mali-G57 testing remains outstanding.
- This is a debug build, not a signed production release.

## Build

The **Upstream Winlator AI Build** workflow checks out the upstream repository and its app, Vortek and Gladio submodules, applies this repository's launcher/branding/assistant overlay, then runs Gradle `assembleDebug`. It uploads the APK and publishes direct prerelease assets.

## Licensing

Original project additions are under the MIT License. Upstream Winlator and third-party dependencies retain their own licenses and copyright notices, including LGPL-2.1 where applicable.
