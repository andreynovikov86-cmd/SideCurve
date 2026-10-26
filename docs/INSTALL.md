# Install and update SideCurve VST3

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

For compatibility with early preview builds, the settings directory still uses
the old internal name `SideKick`. The following are the actual settings paths
used by the current **SideCurve**; do not rename these folders.

macOS preferences: `~/Library/Application Support/SideKick/window.txt`.
Windows preferences: `%APPDATA%\SideKick\window.txt`.

## Existing JSFX projects

The VST3 is a separate plug-in format. Existing JSFX instances are preserved;
REAPER does not replace them automatically. The reference JSFX included in the
source repository is for testing, and is not a VST3 dependency.

## Updating SideCurve

1. Download the latest package for your platform from
   [GitHub Releases](https://github.com/andreynovikov86-cmd/SideCurve/releases).
2. Save your projects and close the DAW so the loaded plug-in can be replaced.
3. Replace the entire installed `SideCurve.vst3` bundle in the same VST3 folder.
4. Reopen the DAW and refresh its VST3 plug-in list if needed.

The existing plug-in IDs and window preferences are retained. Updates currently
use the release archives; ReaPack installation and updates are not configured.

### Migrating from an early preview

Early previews were called **SideKick**. If an old `SideKick.vst3` bundle is
still installed, remove it from the active VST3 folder when installing
`SideCurve.vst3`, to avoid two bundles with the same plug-in IDs. Existing saved
VST3 instances use those retained IDs. The legacy settings directory above is
kept so SideCurve remembers the window size you already chose.
