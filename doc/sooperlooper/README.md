# Seq66 Loves SooperLooper — integration documentation

This directory is the source of truth for the audio-loop integration developed
in this fork. It complements the upstream Seq66 user manual; it does not try to
replace the upstream MIDI documentation.

## Project goal

The fork aims to make audio loops feel like native Seq66 patterns while keeping
SooperLooper as a separate, headless, real-time audio engine.

A musician should eventually be able to create an audio slot in the Seq66 grid,
choose its musical length, record it, launch it, mute it, overdub it and arrange
it alongside MIDI patterns without operating the SooperLooper GUI.

## Documents

- [ARCHITECTURE.md](ARCHITECTURE.md) — process boundaries, authority, backends,
  threads, transport, loop identity, persistence and failure handling.
- [OSC-CONTROL-AND-FEEDBACK.md](OSC-CONTROL-AND-FEEDBACK.md) — commands sent to
  SooperLooper and all runtime feedback received from it.
- [SPECIFICATION.md](SPECIFICATION.md) — normative behaviour and acceptance
  criteria for audio clips, recording, sync, feedback and supervision.
- [HEADLESS-TESTING.md](HEADLESS-TESTING.md) — unit, protocol, real-engine,
  negative-backend, fault-injection and soak-test strategy.
- [DEVELOPMENT.md](DEVELOPMENT.md) — reproducible dependencies, focused builds,
  pinned engine, JACK dummy fixture and diagnostics.
- [ROADMAP.md](ROADMAP.md) — staged implementation order and definition of done.
- [Audio test directory](../../tests/audio/README.md) — current executable test
  files and their responsibilities.

Repository-wide agent instructions live in [`/AGENTS.md`](../../AGENTS.md).

## Source-of-truth order

When sources disagree, use this order:

1. the actual SooperLooper source code for the pinned/tested revision;
2. SooperLooper's `OSC` protocol document;
3. this fork's normative specification;
4. implementation comments and UI copy.

Differences between the public SooperLooper OSC document and its current source
must be recorded rather than silently guessed.

## Current status

The branch `feature/sooperlooper-audio-clips` currently contains:

- a transport-independent `audio_clip` model;
- musical duration in bars and time signatures;
- free, tape and elastic tempo policies;
- independent pitch shift;
- an initial outbound OSC client;
- a fake-engine OSC contract test;
- a pinned real-engine/JACK-dummy smoke fixture;
- focused CI and the project contract documents.

It does not yet contain the production bidirectional OSC receiver,
observed-state cache, process supervisor, backend gate in application code,
`performer` integration, Qt audio slots, persistence or waveform display.

## Backend decision

SooperLooper tracks initially require native JACK or PipeWire-JACK. Seq66 may
continue using ALSA for MIDI, but ALSA-only audio places clips in a hard
`backend_unavailable` state. This state is not mute, cannot be unlocked by
track controls and never causes project data to be discarded.

## Design principle

Seq66 owns musical intent and project structure. SooperLooper owns real-time
audio processing and reports runtime truth back to Seq66. Neither side may
pretend that a command was completed until feedback confirms the resulting
engine state.
