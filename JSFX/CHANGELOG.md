# 0.4.5 — 26 October 2026

- Make the two-line header more compact: slightly smaller SideCurve title and
  less space before the regular `by Andrey Novikov` credit.
- Keep both lines inside the AN mark's visible top/bottom extent.
- Preserve the Andrey Novikov manufacturer and all processing/state identifiers.

# 0.4.4 — 26 October 2026

- Replace PK with the approved AN maker mark.
- Use a light, upright `by Andrey Novikov` credit below SideCurve, matching the
  selected readable signature rather than the heavy condensed company wordmark.
- Restore Andrey Novikov in manufacturer/author metadata.
- Preserve class IDs, parameter layout, saved-state format and audio processing.
- Install and visually verify both formats in an isolated REAPER project.

# 0.4.3 — 25 October 2026

- Set the maker name to Plugin King in VST3 factory/class metadata and JSFX
  author metadata. Keep legal copyright notices and existing plugin IDs.
- Hosts may need a rescan to refresh their cached developer name.

# 0.4.2 — 25 October 2026

- Increase the maker wordmark from 8.8 to 12.5 logical units and brighten it.
- Preserve type C, using 15% wider letter shapes, extra letter spacing and an
  explicit wider gap between PLUGIN and KING for small-size legibility.
- Align the JSFX product title with its maker signature and improve its scale.
- Audio processing and saved-state compatibility are unchanged.

# 0.4.1 — 25 October 2026

- Replace the crown with the approved original C PK monogram.
- Add a smaller `by PLUGIN KING` line beneath SideCurve; preserve the selected
  condensed italic maker lettering as shared vector outlines.
- Center the complete two-line signature between the existing toolbar buttons.
- Keep DSP, parameter IDs, gesture behavior and saved-state format unchanged.

# 0.4.0 — 24 October 2026

- Rename visible effect to SideCurve; preserve existing JSFX path and slider IDs.
- Add Plugin King crown C1, larger heading, equal GRID/AUDIO/INVERT/RESET buttons,
  smaller bold text, one saturated purple, and slightly larger control points.
- Enlarge and simplify MIX; add subtle shading, an inset near-rim white dot,
  value-following purple arc, closer label and momentary percent readout.
- Uniform 8-unit rails with pixel-aligned end caps; right-aligned dB/ms values.
- INVERT now reflects levels vertically and preserves all point times.
- Hover-only transition-region dragging; point double-click snaps to time grid.
- Supersample and cache crown/knob drawing to avoid ragged JSFX edges.

# Changelog

## 0.3.1

- Lift the head/tail endpoint pairs by dragging their curve segments vertically.
- Show the transition highlight only while hovering that region.
- Double-click the middle handle to snap only points 2 and 4 to the time grid.
- Slow MIX vertical dragging by 12%.

- Square-root waveform display is now the default for both processed audio and
  sidechain peaks, making quiet signals easier to see. Audio processing is unchanged.

## 0.3.0

- Standalone JSFX with a native resizable interface in the track FX chain.
- FX name: SideKick (Sidechain Shaper); English interface.
- GRID, SIDECHAIN, INVERT, RESET, musical LENGTH and MIX controls.
- RESET reads the current REAPER user default, including a preset named No preset;
  a one-time installer configures its path without a running Lua frontend.
- Compact starting window size matching the selected reference (761 × 533 host
  client units in the checked macOS setup).
- One default shape, four freely movable anchors and one curvature handle.
- Both outer anchors move horizontally, preserve inner-anchor times, and control
  the smooth GRID join across the period boundary.
- SIDECHAIN replays the full tail-plus-head transition from actual current gain,
  restarts on accepted attacks and holds the last point until the next attack.
- Fixed sidechain waveform/cursor phase calculation to retain fractional positions.
- Double-click resets only the selected point. INVERT reflects custom shapes.
- Straight durations from 2/1 to 1/64, with larger relative duration labels.
- Independent HP/LP handles from 10 Hz to 20 kHz, 48 dB/octave edges, whole-band
  dragging and headphone audition. Detector controls are disabled in GRID.
- Compact aligned controls, a smooth MIX arc, no percentage below MIX, smaller
  headphones, larger text, and a muted yellow reduction indicator.
- Removed the Lua control window, ReaImGui dependency, six-preset selector,
  beat snapping, track selectors, language switch, internal bypass and extra meters.
- Automated checks execute the actual audio and native-interface logic.

## 0.2.2

Experimental Lua/ReaImGui interface with a separate JSFX audio engine.
Superseded by 0.3.0. Save a copy of preview projects before replacing that
version: several parameter meanings have changed, including outer-anchor times.
