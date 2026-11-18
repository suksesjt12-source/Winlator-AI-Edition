# Upstream Winlator integration notes

## Selected foundation

The project will build on the open-source Android Winlator project:

- Main repository: https://github.com/brunodev85/winlator
- Android application source: https://github.com/brunodev85/winlator-app
- Main repository license: GNU Lesser General Public License v2.1 (LGPL-2.1)

The main repository tracks the Android application, renderer projects and other dependencies through Git submodules. Its current submodule declarations include `app`, `vortek` and `gladio`. A correct integration must preserve those references rather than copying a partial selection of files.

## Why this is the selected path

A genuine Windows-on-Android environment requires more than a launcher screen. The existing Winlator project already supplies the Android container manager and the compatibility stack integration points for Wine, Box64, graphics translation and related components. Reimplementing all of these systems in a small standalone application would be a much larger and less reliable path.

## GPU targeting

The first device target is an ARM64 Android phone using MediaTek Helio G100 and Mali-G57 class graphics. Turnip is primarily an Adreno driver path and must not be selected as a generic Mali driver. The app must detect the available renderer/driver capabilities and offer only compatible choices. Vortek, VirGL or other paths must be validated on the target device before being set as defaults.

## Licensing and supply chain

- Preserve upstream license files and copyright notices.
- Keep the upstream Winlator source and third-party runtime licenses identifiable.
- Do not repackage proprietary game files, firmware or undocumented binary blobs.
- Download runtime components only from trusted project release sources.
- Verify component checksums before installation and keep component version metadata.
- Document the source and license of every bundled or downloaded component.

## Current state

The upstream integration now builds successfully in GitHub Actions. The workflow checks out the upstream Winlator repository and its app, Vortek and Gladio submodules, applies the AI Assistant Activity and navigation item, then builds and uploads a debug APK.

Verified build:
- Workflow run: https://github.com/suksesjt12-source/Winlator-AI-Edition/actions/runs/44256112036
- Artifact: Winlator-AI-Edition-upstream-debug
- The artifact is a debug ZIP containing the built APK; it is not a signed release.

The build reuses the upstream container, shortcut, file manager and component-management implementation. This does not mean all runtime payloads are embedded in the APK: Wine, Box64, system files and other components continue to follow the upstream installation/download model where required.

The new AI Assistant is currently an offline, rule-based information and setup-advice screen. It does not yet inspect live container settings or apply optimizations automatically.

The CI build has not been installed or tested on physical ARM64 hardware. Container creation, component installation, EXE launch, graphics, controls, audio and thermal behaviour remain device-validation tasks.
