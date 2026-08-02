# Seq66 Loves SooperLooper — integration documentation

This directory is the canonical source of truth for the audio-loop integration
developed in this fork. It complements the inherited Seq66 user manual; it does
not replace upstream MIDI documentation.

## Resume work with minimal context

Agents and humans continuing the project should open only:

1. [`PROJECT-MANIFEST.json`](../../PROJECT-MANIFEST.json);
2. [`checkpoints/CURRENT.md`](checkpoints/CURRENT.md);
3. the active task in [`WORK-QUEUE.md`](WORK-QUEUE.md);
4. the specification/protocol sections referenced by that task;
5. relevant code, tests and current PR evidence.

Do not begin by loading the full upstream manual, root history files or every
integration document. The checkpoint and manifest are designed to keep handoffs
small and reliable.

## Project goal

The fork aims to make audio loops feel like native Seq66 patterns while keeping
SooperLooper as a separate, headless, real-time audio engine.

A musician should eventually be able to create an audio slot in the Seq66 grid,
choose its musical length, record it, launch it, mute it, overdub it and arrange
it alongside MIDI patterns without operating the SooperLooper GUI.

## Product and protocol documents

- [ARCHITECTURE.md](ARCHITECTURE.md) — process boundaries, authority, backends,
  threads, transport, loop identity, persistence and failure handling.
- [OSC-CONTROL-AND-FEEDBACK.md](OSC-CONTROL-AND-FEEDBACK.md) — commands sent to
  SooperLooper and all runtime feedback received from it.
- [SPECIFICATION.md](SPECIFICATION.md) — normative behaviour and acceptance
  criteria for audio clips, recording, sync, feedback and supervision.
- [HEADLESS-TESTING.md](HEADLESS-TESTING.md) — unit, protocol, real-engine,
  negative-backend, fault-injection and soak-test strategy.
- [TESTED-BEHAVIOUR.md](TESTED-BEHAVIOUR.md) — revision-specific executable
  evidence, including asynchronous control confirmation.
- [DEVELOPMENT.md](DEVELOPMENT.md) — reproducible dependencies, focused builds,
  pinned engine, JACK dummy fixture and diagnostics.
- [ROADMAP.md](ROADMAP.md) — phase order, gates and definitions of done.

## Project-control and multi-agent documents

- [WORK-QUEUE.md](WORK-QUEUE.md) — executable tasks with stable IDs,
  dependencies, ownership, acceptance criteria and required tests.
- [CHECKPOINTS.md](CHECKPOINTS.md) — mandatory immutable checkpoint and handoff
  protocol.
- [checkpoints/CURRENT.md](checkpoints/CURRENT.md) — mutable pointer to the
  latest immutable checkpoint and minimal resume summary.
- [WORKFLOW.md](WORKFLOW.md) — task claiming, branch, PR, review and exit flow.
- [TRACEABILITY.md](TRACEABILITY.md) — mapping between requirements,
  implementation and executable tests.
- [DECISIONS.md](DECISIONS.md) — accepted, superseded and deferred decisions.
- [DOCUMENTATION-MAP.md](DOCUMENTATION-MAP.md) — which documents are canonical
  fork truth and which are inherited upstream references.
- [UPSTREAM-SYNC.md](UPSTREAM-SYNC.md) — clean-mirror and integration-branch
  policy for importing Seq66 fixes.
- [`CHANGELOG-FORK.md`](../../CHANGELOG-FORK.md) — durable changes specific to
  this fork.
- [`AGENTS.md`](../../AGENTS.md) — mandatory operating contract for Codex,
  Hermes and human contributors.
- [`PROJECT-MANIFEST.json`](../../PROJECT-MANIFEST.json) — compact
  machine-readable project state.
- [Audio test directory](../../tests/audio/README.md) — executable test files
  and their responsibilities.

## Branch model

- `master`: clean upstream Seq66 mirror/import line.
- `fork-main`: product integration branch.
- `feature/*`, `fix/*`, `docs/*`, `sync/*`: short-lived branches targeting
  `fork-main`.

Fork product changes must not target `master`. Upstream changes are imported into
`master` and then integrated deliberately into `fork-main` with focused
regression evidence.

After the first integration PR is merged, the repository's GitHub default branch
should be changed manually from `master` to `fork-main` so normal visitors and
agents land on the fork documentation while `master` remains available as the
clean upstream line.

## Source-of-truth order

When sources disagree, use this order:

1. tested source revisions of Seq66 and SooperLooper;
2. executable evidence in CI and `TESTED-BEHAVIOUR.md`;
3. normative `SPECIFICATION.md`;
4. `ARCHITECTURE.md` and accepted decisions;
5. current checkpoint and work queue;
6. implementation comments and UI copy.

Differences between public SooperLooper documentation and current source must be
recorded and tested rather than silently guessed.

## Current status

The active integration line currently contains:

- a transport-independent `audio_clip` model;
- musical duration in bars and time signatures;
- free, tape and elastic tempo policies;
- independent pitch shift;
- an initial outbound OSC client;
- fake-engine OSC contract tests;
- a pinned real-engine/JACK-dummy smoke fixture;
- a machine-validated project-control plane and checkpoint protocol.

It does not yet contain the production bidirectional OSC receiver,
observed-state cache, process supervisor, backend gate in application code,
`performer` integration, Qt audio slots, persistence or waveform display.

## Backend decision

SooperLooper tracks initially require native JACK or PipeWire-JACK. Seq66 may
continue using ALSA for MIDI, but ALSA-only audio places clips in hard
`backend_unavailable`. This state is not mute, cannot be unlocked by track
controls and never causes project data to be discarded.

## Design principle

Seq66 owns musical intent and project structure. SooperLooper owns real-time
audio processing and reports runtime truth back to Seq66. Neither side may
pretend that a command completed until feedback confirms the resulting engine
state.
