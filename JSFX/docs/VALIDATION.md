# Validation — SideCurve JSFX 0.4.5

Installed and visually checked on 26 October 2026.

The harness compiles and executes the actual sections of `SideKick.jsfx` with
Cockos WDL EEL2 revision `61c5553aad0d257775a4dd7bf36b8be617ec8281`.

- This source passed all **74 UI checks**: point and pair dragging, snapping,
  hover behavior, inversion, MIX behavior, detector controls and preset loading.
- The unchanged DSP previously passed **42 audio checks** at 44.1/48/96 kHz,
  including timing, early retriggers, endpoint holding, filter response and MIX.
- The current AN maker mark and upright regular `by Andrey Novikov` credit were
  visually inspected in isolated REAPER 7.82. Actual screenshot: `SideCurve.png`.
- The complete title block is cached at 4x scale. It requires no external bitmap
  or font file. Its vector contours match the native VST3 maker artwork.
- Author metadata is **Andrey Novikov**. The physical `SideKick/SideKick.jsfx`
  path and existing slider slots remain stable for saved projects.

Drawing adapters in the UI harness do not replace the host screenshot check.
Windows/Linux native UI and broader listening in real projects remain unverified.
REAPER retains sizes per existing JSFX instance; no global new-instance size is promised.

## Compact title update

The current compact title was inspected in both formats on 26 October 2026:
the product title is slightly smaller, the author line is closer, and visible
text stays within the AN mark's top/bottom bounds. The current source passed
all 74 real-EEL UI checks; the current VST3 binary passed all 47 SDK validator
checks and macOS signature verification. Current screenshots: `SideCurve.png`.
