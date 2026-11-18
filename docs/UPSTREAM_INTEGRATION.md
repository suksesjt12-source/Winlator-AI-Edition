# Upstream Winlator integration notes

## Selected foundation

The project builds on the open-source Android Winlator project:

- Main repository: https://github.com/brunodev85/winlator
- Android application source: https://github.com/brunodev85/winlator-app
- Main repository license: GNU Lesser General Public License v2.1 (LGPL-2.1)

The main repository tracks the Android application, renderer projects and other dependencies through Git submodules. The workflow checks out the original app, Vortek and Gladio references recursively.

## AI Edition interface

This repository adds a distinct launcher layer on top of the upstream runtime:

- `LauncherActivity` is now the Android MAIN/LAUNCHER activity.
- The custom dashboard provides Game Library, Containers, AI Assistant, and Components & Settings entry points.
- The AI Edition cover is reused in the hero banner and Android adaptive application icon.
- The upstream `MainActivity` remains available internally and receives the selected runtime section from the new dashboard.
- The AI Assistant remains an offline rule-based advisor and can also be opened from the runtime navigation drawer.

The redesigned home screen is not a replacement for the Wine/Box64/graphics engines. Existing upstream management pages still handle container, shortcut and component operations. Replacing every internal management screen is a later task.

## GPU targeting

The first device target is an ARM64 Android phone using MediaTek Helio G100 and Mali-G57 class graphics. Turnip is primarily an Adreno driver path and must not be selected as a generic Mali driver. The app should offer only renderers and drivers supported by the target build and hardware.

## Licensing and supply chain

- Preserve upstream license files and copyright notices.
- Keep the upstream Winlator source and third-party runtime licenses identifiable.
- Do not repackage proprietary game files, firmware or undocumented binary blobs.
- Download runtime components only from trusted project release sources.
- Verify component checksums before installation and keep component version metadata.
- Document the source and license of every bundled or downloaded component.

## Build and validation status

The GitHub Actions workflow compiles the upstream app, applies the custom launcher/cover/AI Assistant overlay, then runs Gradle `assembleDebug`. A successful CI build validates compilation only; installation, container creation, component downloads and game execution must still be checked on physical hardware.

Initial target: Android 14, ARM64, Helio G100 / Mali-G57 class hardware, 8 GB RAM. This is a target configuration, not a claim of completed device testing.
