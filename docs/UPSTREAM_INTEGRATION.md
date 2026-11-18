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

This repository currently contains an independently written Android prototype. It does not yet contain the upstream Winlator source, Wine, Box64, DXVK, Vulkan drivers or a working container implementation. The integration phase has not been marked complete.
