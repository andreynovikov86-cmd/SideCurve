# Install and update SideCurve with ReaPack

[English overview](../README.md#install-and-update-with-reapack) · [Инструкция на русском](../README.ru.md#установка-и-обновление-через-reapack)

The repository provides **VST3 and JSFX installer actions for REAPER**. You may
install either format or both. The VST3 itself also works in other VST3 hosts;
REAPER is only needed to run these installation actions. Manual ZIP downloads
remain available from [Releases](https://github.com/andreynovikov86-cmd/SideCurve/releases).

## Add the repository once

1. Install [ReaPack](https://reapack.com/) if it is not already available in REAPER.
2. Choose **Extensions → ReaPack → Import repositories** and paste:

   ```text
   https://raw.githubusercontent.com/andreynovikov86-cmd/SideCurve/main/index.xml
   ```

3. In **Extensions → ReaPack → Browse packages**, search for **SideCurve**.
4. Select **SideCurve - VST3 and JSFX installers**, mark it for installation and
   click **Apply**.
5. Open **Actions → Show action list**. Run one or both actions:
   - **SideCurve - Install or Update VST3**
   - **SideCurve - Install or Update JSFX**
6. Save your projects and restart REAPER. Refresh the FX browser if needed.

Before updating a loaded VST3, save and close projects using it. If Windows says
the plug-in is in use, restart REAPER with an empty project and run the action
before opening the project again. Close other DAWs using SideCurve as well.

## Update

Choose **Extensions → ReaPack → Synchronize packages**, then **run the same
installer action again**. ReaPack downloads the new installer and plugin files;
the action copies the selected format into its actual plugin folder. ReaPack
synchronization alone does not replace the installed VST3 or JSFX.

The first repository package is **1.0.0**. This is the installer package version:
it installs **VST3 0.5.5** and/or **JSFX 0.4.5**.

## Requirements and installed files

| Format | Requirements | Destination |
| --- | --- | --- |
| VST3 on macOS | macOS 11+, Apple Silicon or Intel | `~/Library/Audio/Plug-Ins/VST3/SideCurve.vst3` |
| VST3 on Windows | Windows 10+ x64 with the system `tar.exe` utility | `%LOCALAPPDATA%\Programs\Common\VST3\SideCurve.vst3` |
| JSFX | REAPER on macOS or Windows | REAPER resource folder: `Effects/SideKick/SideKick.jsfx` |

The actions install for the current user. Add the Windows user VST3 folder to
your host's scan paths if it is not discovered automatically. The macOS preview
is ad hoc signed and not Apple notarized.

The VST3 action verifies the published archive's SHA-256 before extraction and
replacement. If installation fails, the previous version is retained; if it
cannot be restored automatically, the error message gives its recovery path.
The JSFX action configures the REAPER resource path needed by RESET. Existing
project IDs, presets and window preferences are retained.

The old `SideKick` JSFX filename and preferences directory are intentional
compatibility paths. Do not rename them. If an early `SideKick.vst3` preview is
still present, move that old bundle out of the VST3 scan folder before installing
SideCurve. [Migration details](INSTALL.md#migrating-from-an-early-preview).

## Remove

Uninstalling the ReaPack package removes its actions and downloaded payloads.
It does **not** remove the plugin copies installed by those actions. To remove
SideCurve itself, close the DAW and remove `SideCurve.vst3` from the destination
above and/or `Effects/SideKick/SideKick.jsfx` from the REAPER resource folder.
Saved projects using those plugins will then need them reinstalled.

## Maintaining the repository package

The release archives are the same builds as the manual downloads. No separate
DSP build is used for ReaPack. To publish a later package:

1. Publish and validate the platform release archives.
2. Update their filenames, versions and SHA-256 values in
   `distribution/SideCurve/installer.lua`, and the independent expected values in
   `tests/test_installer.py`.
3. Bump `M.version` and both action headers together. Commit the sources, JSFX
   payload and tests. Run the repository installer checks on macOS and Windows.
4. Run `python3 tools/reapack_index.py --commit FULL_SOURCE_COMMIT_SHA` from the
   repository, then `python3 tests/test_reapack_index.py`. Commit `index.xml`.

The index pins script sources to that full commit and native archives to their
release URLs. Keep existing release assets and index versions available. The
generator rejects a reused package version and preserves older versions.
