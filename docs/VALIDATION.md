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

Windows x64 CI is prepared but has not run. Intel code is compiled, but Intel
hardware and broader real-project listening checks remain pending.

## Compact title update

The current compact title was inspected in both formats on 26 October 2026:
the product title is slightly smaller, the author line is closer, and visible
text stays within the AN mark's top/bottom bounds. The current source passed
all 74 real-EEL UI checks; the current VST3 binary passed all 47 SDK validator
checks and macOS signature verification. Current screenshots: `SideCurve.png`.
