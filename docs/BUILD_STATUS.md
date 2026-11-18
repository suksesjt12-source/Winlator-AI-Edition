# Build Status — 18 November 2026

## Successful upstream integration build

- Repository: https://github.com/suksesjt12-source/Winlator-AI-Edition
- Workflow run: https://github.com/suksesjt12-source/Winlator-AI-Edition/actions/runs/44256112036
- Repository commit: d9895b9782e0ca95afdbff93b1b0ecaac4998198
- Result: **success**
- Artifact: Winlator-AI-Edition-upstream-debug
- Artifact page: https://github.com/suksesjt12-source/Winlator-AI-Edition/actions/runs/44256112036/artifacts/14405669003
- Artifact size: 156,658,040 bytes
- Artifact ZIP SHA-256: e4dad15b3dd6bed8fa8c2164209a1055850733621d332610bf413ae285f3b48b

## Source revisions used by the workflow

- Winlator main repository: 30aafbdb9eea47b92669201621e03cd2cf04540d
- Android app submodule: a030f552f452158a2db64fdb32b490fa19c0b48d
- Gladio submodule: 116c0d14dedbea3bd057f98f1db138bb1efe225e
- Vortek submodule: b1730c5def9b575672e671aee11d79ae7adc63d1

## What this proves

The upstream Android project and its native modules were checked out, the AI Assistant source was copied into the upstream app, the navigation and manifest edits were applied, Gradle completed the debug build, and GitHub Actions uploaded the artifact.

## What this does not prove

The APK has not been installed or exercised on a physical Android device. A successful CI build does not verify first-run component downloads, container creation, Windows executable launch, Mali-G57 graphics compatibility, controller input, audio, or thermal stability.

The artifact is a debug build, not a signed release. Runtime components may still need to be installed through the upstream application's own component-management flow.
