# Changelog — Seq66 Loves SooperLooper

All notable changes specific to this fork are recorded here.

This file is separate from upstream Seq66 `ChangeLog`, `NEWS` and `RELNOTES`.
Those files remain upstream historical/release references and are not the fork's
work queue or product changelog.

The format follows Keep a Changelog principles without assigning release
versions before the fork reaches a releasable state.

## [Unreleased]

### Added

- M1-004A generation atomicity corrective: TOCTOU race fixed (generation
  check inside m_mutex), wildcard bypass removed (no default
  event_generation=0), event provenance via receiver_event.generation
  stamped at receive time, typed apply_event() entry point, NaN/Inf
  rejection in apply_field(float).  27 tests pass.  TSAN compiled but
  cannot execute on ARM64 kernel 6.17 (documented).  PR #8 merged
  (merge commit `691241dc`).

- M1-004 engine generation tracking (`set_generation()`, generation-aware
  `apply()`, `snapshot_data.generation`).  uint64 monotonic counter
  advances on engine restart; `set_generation()` clears all cached state
  atomically; `apply()` rejects events from stale generations; backward
  compatible via default `event_generation=0`.  6 new test groups
  covering generation lifecycle, stale rejection, matching acceptance,
  and multiple resets.  PR #6 merged to fork-main
  (merge commit `bf030007`).

- M1-003 observed-state cache (`sooperlooper_observed_cache`) providing
  thread-safe storage of SooperLooper feedback with per-field freshness
  timestamps, zero-vs-absent distinction, meter/position coalescing,
  immutable snapshots, and generation-reset hook (`clear()`).  Unknown
  controls safely rejected at typed boundary.  14 test groups covering
  empty state, apply, zero vs absent, freshness, coalescing, transitions,
  unknown control rejection, snapshot immutability, clear(), global state,
  multiple loops, concurrent read/write, and argument validation.
  Co-consulted with codex-senior-consult (plan + merge-gate).
  PR #5 merged to fork-main (merge commit `da8ec5cb`).

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
