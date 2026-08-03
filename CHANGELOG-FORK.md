# Changelog — Seq66 Loves SooperLooper

All notable changes specific to this fork are recorded here.

This file is separate from upstream Seq66 `ChangeLog`, `NEWS` and `RELNOTES`.
Those files remain upstream historical/release references and are not the fork's
work queue or product changelog.

The format follows Keep a Changelog principles without assigning release
versions before the fork reaches a releasable state.

## [Unreleased]

### Added

- M1-002 SooperLooper OSC receiver (`sooperlooper_receiver`) with liblo server
  thread lifecycle, strict `(path, types)` allow-list rejecting unsupported OSC
  type tags, catch-all trampoline routing all inbound messages through a
  thread-safe event queue, typed `poll_event`/`wait_event`/`dispatch` receive
  paths, bounded queue with atomic overflow counter, and deterministic
  start/stop lifecycle (idempotent, no-op safe).  The start/stop concurrency
  contract is documented: single-thread control only; external synchronization
  required for concurrent calls.  Comprehensive test suite covering lifecycle,
  handler registration, duplicate rejection, type-tagged send/receive,
  unhandled-path poll, blocking wait, overflow, and concurrent registration
  with dispatch.  CI integration via `audio-core.yml` receiver test step.
  PR #4 merged to fork-main (merge commit `1131a33d`).

- M1-001 typed SooperLooper OSC protocol identifiers, bidirectional string
  mappings, per-control range/type metadata and bounded observed-state integer
  parsing in `libseq66/include/audio/sooperlooper_protocol.hpp` and
  `libseq66/src/audio/sooperlooper_protocol.cpp`, plus a focused
  `tests/audio/sooperlooper_protocol_test.cpp` covering all command,
  loop-control and global-control identifiers and the canonical SooperLooper
  state integers (-1, 0..14, 20).

- Fork purpose and authority model in the root README.
- `audio_clip` model with arbitrary positive bar counts and time signatures.
- Free, tape and elastic tempo policies plus independent pitch shift.
- Initial non-real-time SooperLooper OSC command adapter.
- Fake-engine OSC contract test with strict path/signature validation.
- Pinned SooperLooper 1.7.9 real-engine smoke test over JACK dummy.
- Architecture, bidirectional OSC contract, normative specification, headless
  testing strategy, development guide and phased roadmap.
- `AGENTS.md` operating contract for Codex, Hermes and human contributors.
- Machine-readable `PROJECT-MANIFEST.json`.
- Multi-agent workflow, actionable work queue, checkpoint protocol, decision
  log, documentation map and requirement/test traceability.
- Immutable checkpoint `CP-001` plus the small mutable
  `checkpoints/CURRENT.md` resume pointer.
- Branch policy separating upstream mirror `master`, fork integration
  `fork-main` and short-lived feature branches.
- Dedicated upstream synchronization policy preserving a clean import line.
- Structural CI validation for project-control metadata, canonical files,
  active task, checkpoint schema and required workflows.
- Root fork entry points replacing the inherited unstructured `TODO` and
  possible-v2 `ROADMAP.md` on the fork line while preserving their originals on
  `master` and in upstream Seq66.

### Changed

- Pull request #1 now targets `fork-main` instead of upstream-mirror `master`.
- Documentation distinguishes fork plans from upstream Seq66 TODOs, roadmap,
  release notes and historical changelog.
- The integration documentation index now provides a progressive-disclosure
  recovery path designed to minimize agent context use.
- Control confirmation models SooperLooper updates as eventually consistent,
  based on real-engine evidence.
- Material agent sessions must leave an immutable checkpoint, update the work
  queue and record exact CI evidence before handoff.

### Fixed

- Real-engine smoke test no longer assumes a `/set` is visible to an immediate
  `/get`; it waits for bounded observed confirmation.
- Project-control bootstrap now includes the checkpoint and `CURRENT.md` files
  required by its own validator.

### Security and safety

- ALSA-only audio is specified to fail closed as `backend_unavailable` without
  deleting or silently muting persisted clips.
- OSC callback paths, signatures and file operations are constrained by the
  integration specification.

## Changelog rules

Update this file in the same coherent change for:

- user-visible behaviour;
- supported backend or compatibility changes;
- architecture or persistence changes;
- new required workflows or tooling;
- migrations;
- important fixes;
- deprecations or removed fork functionality.

Do not add every internal refactor. Checkpoints record session-level progress;
this changelog records durable project changes.
