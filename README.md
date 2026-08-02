# Seq66 Loves SooperLooper

> **Experimental fork:** make SooperLooper-backed audio loops behave as native
> Seq66 tracks and grid slots, controlled entirely from Seq66.

This repository starts from Seq66 0.99.26 and develops a tightly integrated,
bidirectional audio-loop workflow:

- Seq66 owns the project, musical timeline, UI, transport intent, process
  lifecycle and audio routing;
- SooperLooper runs as a separate headless real-time audio engine;
- Seq66 sends commands and consumes state, position, length, meters, topology
  and operation errors from SooperLooper;
- audio clips sit beside MIDI patterns and eventually support recording,
  launch, mute, overdub, replace, arbitrary bar lengths, tempo following and
  project persistence;
- normal operation does not require the SooperLooper GUI.

## Why this fork exists

Seq66 is a strong pattern-oriented MIDI sequencer and live-performance grid.
SooperLooper is a capable JACK audio looper with an OSC-controlled headless
engine. Combining them allows one musical surface and scheduler to control MIDI
patterns and live-recorded audio loops without turning Seq66 into an audio DSP
engine or forcing the musician to coordinate two independent interfaces.

The goal is deeper than MIDI mapping or launching another application. Audio
slots must have native project identity, observed runtime state, quantized
musical length, routing, save/load behaviour and recovery.

## Authority model

Seq66 is the sole user-facing authority. SooperLooper is a managed servant of
the Seq66 project:

```text
Seq66 desired state -> OSC commands -> SooperLooper DSP
Seq66 UI/state      <- OSC feedback <- SooperLooper runtime truth
```

A command is not considered complete merely because it was sent. Seq66 confirms
state through SooperLooper feedback or a bounded verification query.

## Audio backend support

Initial SooperLooper-backed tracks require a JACK-compatible audio graph:

- **supported:** PipeWire through PipeWire-JACK compatibility;
- **supported:** native JACK;
- **not supported initially:** ALSA-only audio.

Seq66 may still use ALSA for MIDI. In an ALSA-only audio environment, existing
audio clips are preserved and displayed in a hard `backend_unavailable` state.
They are not treated as muted and cannot be unlocked with launch, mute/unmute,
MIDI automation or headless commands. Creating new audio loops is disabled with
an explanation that JACK or PipeWire-JACK is required.

Seq66 will not silently launch JACK over ALSA in the initial implementation.

## Current status

The active integration work currently provides:

- a transport-independent audio-clip model;
- arbitrary positive bar counts and time signatures;
- free, tape and elastic tempo policies;
- independent pitch shift;
- an initial outbound SooperLooper OSC client;
- focused compile/unit CI;
- architecture, bidirectional OSC contract, normative specification, roadmap,
  headless testing plan and agent instructions.

Still pending:

- inbound OSC receiver and observed-state cache;
- ping/subscriptions and command confirmation;
- managed SooperLooper process and JACK routing;
- ALSA-only capability gate in application code;
- performer and Qt grid integration;
- exact N-bar real-engine recording tests;
- transactional audio project persistence.

See the draft integration PR and its CI before assuming a feature is complete.

## Integration documentation

Start with [the audio integration documentation index](doc/sooperlooper/README.md):

- [Architecture](doc/sooperlooper/ARCHITECTURE.md)
- [OSC control and feedback contract](doc/sooperlooper/OSC-CONTROL-AND-FEEDBACK.md)
- [Normative specification](doc/sooperlooper/SPECIFICATION.md)
- [Headless testing strategy](doc/sooperlooper/HEADLESS-TESTING.md)
- [Roadmap](doc/sooperlooper/ROADMAP.md)
- [Instructions for Codex, Hermes and contributors](AGENTS.md)

These documents are part of the implementation contract, not informal notes.

## Development principles

- preserve upstream Seq66 MIDI behaviour;
- keep SooperLooper external and headless;
- distinguish desired state from observed engine state;
- never persist mutable loop indexes as identity;
- keep OSC/network work outside UI and real-time paths;
- fail closed on unsupported backends without losing project data;
- require headless tests and feedback confirmation for user-facing controls;
- keep the integration optional where platform dependencies are unavailable.

---

# Upstream Seq66 README — Seq66 0.99.26 (2026-07-06)

__Seq66__ MIDI sequencer/live-looper with a hardware-sampler grid interface;
pattern banks, triggers, and playlists for song management; scale and chord
aware piano-roll; song layout for creative composition; control/status via MIDI
automation, and mute-groups to enable/disable sets of patterns. Tools for live
performance and for composing great-sounding MIDI tracks. Supports the Non/New
Session Manager; can run headless and on a small computer like the Pi. And
now it provides a form of MIDI Learn. Upstream Seq66 does not support audio
samples, just MIDI; audio support is the purpose of this fork.

__Seq66__ Seq24/Kepler34 on steroids with
modern C++ and new features. Linux and Windows users can build this application
from source code. See the extensive INSTALL files. Includes a comprehensive PDF
user-manual. As of this release, employs the __Meson__ build system.

*The current development-in-progress branch is now "Meson". Seq66 now
builds using Meson, and supports Qt6. Bootstrap and build now done via
the work.sh script. The old-style build setup is preserved in branch
"Autoconf." The lib66 library project is downloaded automatically as a
Meson subproject.*

The release includes an installer for the 64-bit Windows version of
Seq66. Initial work has been done on getting Seq66 to build and run
in FreeBSD using the Clang compiler. A C++17 capable-compiler is
needed.

See NEWS for updates and RELNOTES for the latest highlights.
See the two "INSTALL" files for installation.

The figure below shows Seq66 with modified palette and a style-sheet
in force. Otherwise the application uses the current Qt theme.

![Alt text](doc/latex/images/main-window/main-windows-perstfic.png?raw=true "Seq66")

# Features

## User interface

    *   Qt 5 or Qt 6 (cross-platform). Loop-button grid. Qt style-sheet
        support.
    *   Colorable pattern slots; the palette can be saved in a '*.palette' file.
    *   Drag-and-drop a MIDI file onto the main grid to load it.
    *   Tabs and external windows for patterns, sets, mute-groups, song
        layout, event-editing, play-lists, and session information.
    *   Low-frequency oscillator (LFO) to modify continuous controller
        and velocity values.
    *   A "fixer" for expansion/compression/alignment of note patterns.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   Extremely resizable.
    *   A headless/daemon version can be built to use with a MIDI grid
        controller.

## Configuration files

    *   Supports configuration files: '.rc', '.usr', '.ctrl', '.mutes',
        '.playlist', '.drums' ('.notemap'), '.palette', and Qt '.qss'.
    *   Separates MIDI control and mute-group setting into their own files.
    *   Unified keystroke and MIDI controls in the '.ctrl' file; defines MIDI
        controls for automation/display of Seq66 status in grid controllers
        (e.g. LaunchPad). Sample '.ctrl' files provided for Launchpad Mini.

## Non/New Session Manager

    *   Support for NSM/New Session Manager, RaySession, Agordejo....
    *   Handles starting, stopping, hiding, and session saving.
    *   Displays details about the session.

## Multiple Builds

    *   Meson:
        *   ALSA/JACK: `qseq66` using an rtmidi-based library
        *   PortMidi: `qseq66` using a portmidi-based library
        *   Command-line/headless: `seq66cli`
    *   qmake:
        *   PortMidi: `qpseq66`
        *   Windows: `qpseq66.exe`

## More Features

    *   Supports configurable PPQN from 32 to 19200 (default is 192).
    *   Transposable triggers to re-use patterns more comprehensively.
    *   Song import/export from/to stock MIDI (SMF 0 or 1).
    *   Highly configurable MIDI-based metronome.
    *   Management of scales, keys, and chords.
    *   Improved non-U.S. keyboard support.
    *   Many demonstration and test MIDI files.

## Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce polling.
    *   A ton of clean-up and refactoring.

Seq66 has a user-interface based on Kepler34 and the Seq66 *rtmidi* (Linux)
and *portmidi* (Windows) engines. MIDI devices are detected, inaccessible
devices are ignored, with playback (e.g. to the Windows wavetable synth).
It is built on "linux" via *Meson*, *Qt Creator* or *qmake*, and on
Windows using *MingW*.

*IMPORTANT*: *GNU Autotools* is no longer supported, except in the "Autoconf"
branch.

The INSTALL has been streamlined, see that file for build-from-source
instructions for Linux or Windows, and using a conventional source tarball.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

## Recent Changes

    *   See the NEWS file.

// vim: sw=4 ts=4 wm=2 et ft=markdown