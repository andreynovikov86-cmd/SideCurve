# Install SideCurve VST3

Unzip the package and copy the **entire SideCurve.vst3 bundle**.

## macOS

Copy `SideCurve.vst3` into:

```text
~/Library/Audio/Plug-Ins/VST3/
```

Create the VST3 folder if it is missing. The universal build contains Apple
Silicon and Intel code and targets macOS 11 or later. The preview build is
ad hoc signed; it is not Apple notarized.

## Windows

Copy `SideCurve.vst3` into your user VST3 folder:

```text
%LOCALAPPDATA%\Programs\Common\VST3\
```

Alternatively use the system-wide VST3 folder configured in your DAW. The
Windows build is x64. Keep all files inside the bundle together.

## In the DAW

Refresh the VST3 plug-in list and insert **SideCurve (Sidechain Shaper)**.
This is a native VST3: no installer script needs to be running.

In REAPER, use a track with at least four channels. Send the trigger source's
channels 1/2 to the receiving track's channels 3/4. Switch SideCurve to AUDIO.
Other DAWs expose the auxiliary bus through their external-sidechain routing.

## Window size

Resizing is saved automatically for later instances. UI preferences are separate
from the audio preset and do not alter saved projects' audio settings.

macOS preferences: `~/Library/Application Support/SideKick/window.txt`.
Windows preferences: `%APPDATA%\SideKick\window.txt`.

## Existing JSFX projects

The VST3 is a separate plug-in format. Existing JSFX instances are preserved;
REAPER does not replace them automatically. The reference JSFX included in the
source repository is for testing, and is not a VST3 dependency.

## Updating SideKick

Replace the old installed bundle with SideCurve.vst3; remove SideKick.vst3 from
the active VST3 folder to avoid duplicate class IDs. Save projects and restart
the DAW so loaded instances use the new binary. Old instances use the retained
VST3 class IDs. The previous SideKick preference directory is retained to keep
the user's saved window size.
