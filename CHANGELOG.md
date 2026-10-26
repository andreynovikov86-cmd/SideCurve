# 0.5.5 — 26 October 2026

- Make the two-line header more compact: slightly smaller SideCurve title and
  less space before the regular `by Andrey Novikov` credit.
- Keep both lines inside the AN mark's visible top/bottom extent.
- Preserve the Andrey Novikov manufacturer and all processing/state identifiers.

# 0.5.4 — 26 October 2026

- Replace PK with the approved AN maker mark.
- Use a light, upright `by Andrey Novikov` credit below SideCurve, matching the
  selected readable signature rather than the heavy condensed company wordmark.
- Restore Andrey Novikov in manufacturer/author metadata.
- Preserve class IDs, parameter layout, saved-state format and audio processing.
- Install and visually verify both formats in an isolated REAPER project.

# 0.5.3 — 25 October 2026

- Set the maker name to Plugin King in VST3 factory/class metadata and JSFX
  author metadata. Keep legal copyright notices and existing plugin IDs.
- Hosts may need a rescan to refresh their cached developer name.

# 0.5.2 — 25 October 2026

- Increase the maker wordmark from 8.8 to 12.5 logical units and brighten it.
- Preserve type C, using 15% wider letter shapes, extra letter spacing and an
  explicit wider gap between PLUGIN and KING for small-size legibility.
- Align the JSFX product title with its maker signature and improve its scale.
- Audio processing and saved-state compatibility are unchanged.

# 0.5.1 — 25 October 2026

- Replace the crown with the approved original C PK monogram.
- Add a smaller `by PLUGIN KING` line beneath SideCurve; preserve the selected
  condensed italic maker lettering as shared vector outlines.
- Center the complete two-line signature between the existing toolbar buttons.
- Keep DSP, parameter IDs, gesture behavior and saved-state format unchanged.

# 0.5.0 preview — 24 October 2026

- Rename to SideCurve and SideCurve.vst3; preserve VST3 class and parameter IDs.
- Plugin King crown C1, larger heading, smaller bold text, equal upper buttons,
  AUDIO mode label, unified saturated purple and larger control points.
- New enlarged shaded MIX face, near-rim white dot, value-following thick arc,
  closer MIX label and press/drag-only percent readout.
- Uniform rails, shorter detector controls, right-aligned values with dB/ms.
- Vertical amplitude INVERT; hover-only grouped transition dragging; double-click
  point time snapping. Shared gesture model has direct automated coverage.

# Changelog

## 0.4.1

- Lift the head/tail endpoint pairs by dragging their curve segments vertically.
- Show the transition highlight only while hovering that region.
- Double-click the middle handle to snap only points 2 and 4 to the time grid.
- Slow MIX vertical dragging by 12%.

- Square-root waveform display is now the default for both processed audio and
  sidechain peaks, making quiet signals easier to see. Audio processing is unchanged.

## 0.4.0 — VST3 preview

- Ported SideKick's audio engine to native C++ with conformance checks against JSFX.
- Added a native VST3 component/controller and stereo auxiliary sidechain bus.
- Retained the custom vector interface and direct envelope editing.
- Added saved global window dimensions for newly opened instances.
- Added native HiDPI sizing, host state and automation support.
- Added a REAPER host callback for RESET to its saved user default, with a factory
  fallback in other contexts. No Lua bridge is required.
- Added universal macOS packaging and Windows x64 build/validation workflow.

## 0.3.0 — JSFX

The previous standalone JSFX remains available separately. A frozen copy of its
engine is included as a test reference; the VST3 has no dependency on it.
