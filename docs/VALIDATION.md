# Validation — SideCurve VST3 0.5.5 preview

Installed and visually checked on 26 October 2026 on macOS Apple Silicon.

- Universal arm64/x86_64 bundle built with official Steinberg VST3 SDK revision
  `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` (3.8.1).
- This build passed the official Steinberg validator: **47 passed, 0 failed**.
- Factory and both class records report **Andrey Novikov**. Processor/controller
  IDs match the previous installed version; parameter and state layouts are unchanged.
- The unchanged audio/gesture implementation previously passed **773799 assertions**
  and **63 C++/EEL2 conformance cases** at 44.1/48/96 kHz. This update edits artwork,
  text styling and maker metadata, not DSP.
- macOS ad-hoc signature verified. This is not Developer ID signing or notarization.
- The current AN mark, regular `by Andrey Novikov` credit, alignment and toolbar
  spacing were inspected in isolated REAPER 7.82. Actual screenshot: `SideCurve.png`.
- The corresponding current JSFX passed all **74 real-EEL UI checks** and its
  signature was also visually inspected. The user's project was untouched.

The [publication workflow](https://github.com/andreynovikov86-cmd/SideCurve/actions/runs/40307825037)
completed successfully on 26 October 2026 for both macOS universal and Windows x64.
Each platform passed the engine tests and the official validator: **47 passed,
0 failed**. The C++/EEL2 conformance and JSFX audio/UI checks also passed on Linux.
The Windows test translation unit explicitly includes `<string>` for MSVC.
The published [v0.5.5 preview](https://github.com/andreynovikov86-cmd/SideCurve/releases/tag/v0.5.5)
contains both VST3 packages and JSFX 0.4.5. Intel hardware, Windows DAW listening,
and broader real-project listening checks remain pending.

## Compact title update

The current compact title was inspected in both formats on 26 October 2026:
the product title is slightly smaller, the author line is closer, and visible
text stays within the AN mark's top/bottom bounds. The current source passed
all 74 real-EEL UI checks; the current VST3 binary passed all 47 SDK validator
checks and macOS signature verification. Current screenshots: `SideCurve.png`.

## ReaPack distribution

The [repository installer workflow](https://github.com/andreynovikov86-cmd/SideCurve/actions/runs/40422461025)
passed on macOS and Windows for the indexed installer sources. Each platform
passed **14 filesystem integration scenarios**, using the real published ZIP
and the real Lua installer: fresh install, update, missing/corrupt/malformed
archive, extraction failure, publication failure, failed rollback with the
previous bundle preserved, loaded-file failure, early preview detection,
unsupported platform, JSFX fresh install/update and incomplete JSFX payload.

Hashes, extraction, writes and renames operate on real temporary files; REAPER's
host APIs are adapted by the test fixture. Windows commands run directly through
CreateProcess, and macOS arguments follow Cockos WDL parsing rather than shell
parsing. The macOS tests include Unicode, spaces, quotes and shell punctuation
in paths and verify the installed bundle's signature. These are installer
integration checks, not an additional DAW listening or ReaPack GUI test.

Index checks cover both registered Actions, per-platform ZIP selection, source
commit pinning, destination paths, duplicate-version rejection and preservation
of older versions. ReaPack synchronization downloads the package; an explicit
installer action performs each plugin installation/update.
