# Changelog — Seq66 Loves SooperLooper

All notable changes specific to this fork are recorded here.

This file is separate from upstream Seq66 `ChangeLog`, `NEWS` and `RELNOTES`.
Those files remain upstream historical/release references and are not the fork's
work queue or product changelog.

The format follows Keep a Changelog principles without assigning release
versions before the fork reaches a releasable state.

## [Unreleased]

### Added

- Machine-validated autonomous project governance: `PROJECT-AUTONOMY.json`,
  L1-L4 decision authority, independent `codex-senior-consult` contracts,
  exact-head autonomous merge gates, bounded human-decision packets, a complete
  recover/implement/review/merge/checkpoint mission lifecycle, CI validation and
  task/PR/report templates. Ordinary task execution and eligible merges can now
  continue without repeated human coordination inside an approved milestone.

- M1-008 Phase 1 integration gate PASS (CP-016): all three required CI
  workflows green on gate head `50d39dfb`, `codex-senior-consult` verdict
  `accept`, Phase 2 authorized. PR #14 awaits human approval.

- M2-003 managed process supervisor core: injectable `process_adapter`
  interface, owned-child PID verification, generation tracking, bounded
  shutdown escalation (graceful → SIGTERM → SIGKILL), non-blocking poll,
  deterministic argv. 86 test assertions. PR #19 merged to fork-main (merge commit `e09930f7`).

- M2-004 engine launch orchestration: backend-gated launch, deterministic
  OSC/JACK naming from instance_id + generation hash, generation sync
  between supervisor/observed_cache/engine_monitor, stale callback
  rejection. 62 test assertions. PR #20 merged to fork-main (merge commit `5b8e2ba7`).

- Phase 1 merged into `fork-main` via PR #14 (merge commit `75c57c4a`).
  Human approval recorded. CP-017 governance correction published.
  CP-018 opens Phase 2. First M2 task `M2-001` selected.

- M1-005B subscription wire protocol correction: exact loop/global paths,
  `sss`/`siss` signatures, callback URL/path, integer auto-update interval,
  unregister/cancellation paths, 16 tests.

- M1-006B generation-aware reconciliation: engine generation in pending
  operations, immutable reconciliation requests, generation cancellation,
  unknown-UUID handling, 25 tests.

- M1-007B reordered event semantics: monotonic timestamp handling,
  stale-event rejection, explicit arrival-order semantics, 38 tests.

- M1-005A real OSC ping/subscription transport (PR #11)
- M1-006A safe reconciliation without mutex (PR #12)
- M1-007A meaningful fault-injection assertions (PR #13)

- M1-007 fault-injection matrix: comprehensive test suite covering delayed, duplicate, reordered, lost, malformed callbacks, shutdown races, queue overflow, generation rollover. PR #10 merged (merge commit `a19ea20c`).

- M1-006 command confirmation contracts: `command_confirmation_tracker` with UUID-keyed pending operations, deadline evaluation, reconciliation, confirm/fail/cancel outcomes, 12 test groups. PR #9 merged (merge commit `d56f8edb`).

- M1-005 ping, discovery and subscriptions: `sooperlooper_engine_monitor`
  lifecycle state machine (disabled -> starting -> reconciling -> ready ->
  stale -> engine_offline), configurable deadlines (ping interval 1s,
  timeout 1s, startup 5s, stale threshold 3), client ping/version/
  subscribe API, 11 test groups.  PR #7 merged
  (merge commit `2228cfd0`).

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

- `AGENTS.md`, task proposals and PR templates now classify L1-L4 authority,
  require exact-head senior review where applicable and permit ordinary
  autonomous merges only through the versioned merge gate. Milestone
  transitions remain explicit human decisions.
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

- Autonomous operation explicitly forbids force pushes, shared-history rewrite,
  silent requirement relaxation, red-check merges, implicit approval and
  destructive/irreversible changes without the corresponding human gate.
- ALSA-only audio is specified to fail closed as `backend_unavailable` without
  deleting or silently muting persisted clips.
- OSC callback paths, signatures and file operations are constrained by the
  integration specification.

## Changelog rules

Update this file in the same coherent change for:

- user-visible behaviour;
- supported backend or compatibility changes;
- architecture or persistence changes;
- autonomy, merge or human-authority changes;
- new required workflows or tooling;
- migrations;
- important fixes;
- deprecations or removed fork functionality.

Do not add every internal refactor. Checkpoints record session-level progress;
this changelog records durable project changes.
