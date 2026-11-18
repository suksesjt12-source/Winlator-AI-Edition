# Build Status — 18 November 2026

## Latest published upstream integration build

- Repository: https://github.com/suksesjt12-source/Winlator-AI-Edition
- Workflow run: https://github.com/suksesjt12-source/Winlator-AI-Edition/actions/runs/44257340216
- Repository commit: `8ea09cb45a4a8027f44f427d1be85f7ff118134c`
- Result: **success**
- Gradle result: `BUILD SUCCESSFUL in 1m 39s`
- GitHub prerelease: https://github.com/suksesjt12-source/Winlator-AI-Edition/releases/tag/ai-debug-8
- Direct APK: https://github.com/suksesjt12-source/Winlator-AI-Edition/releases/download/ai-debug-8/app-debug.apk
- SHA-256 file: https://github.com/suksesjt12-source/Winlator-AI-Edition/releases/download/ai-debug-8/app-debug.apk.sha256
- APK size: 157,551,627 bytes
- APK SHA-256: `3b0e5ca13d0e00e9a35950550dadf60f6452e674260e5c2e593a973f69cf8046`
- Android application ID: `com.winlator.aiedition`
- FileProvider authority: `com.winlator.aiedition.FileProvider`
- ABI filter: `arm64-v8a`
- Minimum Android API: `26` (Android 8.0)

## Source revisions used by the workflow

- Winlator main repository: `30aafbdb9eea47b92669201621e03cd2cf04540d`
- Android app submodule: `a030f552f452158a2db64fdb32b490fa19c0b48d`
- Gladio submodule: `116c0d14dedbea3bd057f98f1db138bb1efe225e`
- Vortek submodule: `b1730c5def9b575672e671aee11d79ae7adc63d1`

## What this proves

The upstream Android project and its native modules were checked out, the AI Assistant source was copied into the upstream app, navigation and manifest edits were applied, the Android application ID and FileProvider authority were changed to a distinct AI Edition identity, Gradle completed the debug build, and GitHub Actions uploaded the APK and published the prerelease assets.

## What this does not prove

The APK has not been installed or exercised on a physical Android device. A successful CI build does not verify first-run component downloads, container creation, Windows executable launch, Mali-G57 graphics compatibility, controller input, audio, or thermal stability.

The artifact is a debug build, not a signed release. Runtime components may still need to be installed through the upstream application's own component-management flow.
