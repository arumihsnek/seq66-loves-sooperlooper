# Roadmap

This roadmap orders work by dependency and risk. A later phase must not bypass
an unmet acceptance gate from an earlier phase.

Day-to-day execution is tracked in `WORK-QUEUE.md`. Current state is tracked in
`PROJECT-MANIFEST.json` and `checkpoints/CURRENT.md`. This file changes only
when phase scope, ordering or gate status changes.

## Status summary

| Phase | Name | Status | Gate record |
|---|---|---|---|
| 0 | Project contract and control plane | complete | CP-001 |
|| 1 | Bidirectional protocol core | complete | CP-016 |
| 2 | Managed engine and backend gate | complete | CP-023 |
| 3 | Allocation and performer integration | ready | pending |
| 4 | First native Qt audio slot | blocked by Phase 3 | pending |
| 5 | Exact musical recording | blocked by Phase 4 | pending |
| 6 | Transactional persistence | blocked by Phase 5 | pending |
| 7 | Advanced controls | deferred until core gates | pending |
| 8 | Hardening and release | deferred | pending |

A phase becomes complete only in the same coherent change that updates:

- this table;
- `PROJECT-MANIFEST.json`;
- `TRACEABILITY.md`;
- the active PR;
- an immutable milestone checkpoint with exact CI evidence.

## Phase 0 — project contract and control plane

Status: **complete**.

Deliverables:

- fork purpose in root README;
- architecture and authority model;
- bidirectional OSC contract;
- normative specification;
- headless test strategy;
- repository agent instructions;
- focused model/mock and real-engine CI;
- machine-readable project manifest;
- bounded-context multi-agent workflow;
- actionable task queue;
- checkpoint protocol and current checkpoint;
- fork changelog;
- decision log;
- requirement/test traceability;
- upstream branch/synchronization policy;
- documentation classification and root TODO/roadmap disambiguation;
- structural project-control CI.

Definition of done:

- documents agree on authority, backend policy and states;
- ALSA-only behaviour is unambiguous;
- outbound and inbound protocol surface is catalogued;
- mock and pinned real-engine foundations pass CI;
- next work is derivable from repository state rather than chat history;
- `master` is reserved as upstream mirror and `fork-main` as fork integration;
- an agent can recover current state using the five-minute path in
  `WORKFLOW.md`.

Gate evidence: `CP-001` and required workflows recorded in the current
checkpoint.

## Phase 1 — bidirectional protocol core

Status: **complete**.
Gate record: `CP-016`.

Current milestone: `M1`.

Current task: `M1-001` in `WORK-QUEUE.md`.

Deliverables:

- typed OSC command/control/output identifiers;
- strict outbound validation and canonical mappings;
- local OSC server/receiver lifecycle;
- ping and version discovery;
- typed feedback events;
- thread-safe observed-state cache;
- desired/observed state separation;
- timestamps and freshness;
- engine-generation handling;
- stale/old-generation feedback rejection;
- required update subscriptions;
- command confirmation contracts;
- fake-engine malformed/delayed/duplicated/reordered feedback tests.

Definition of done:

- exact paths and signatures are tested;
- malformed and late feedback are safe;
- no UI or real-time thread blocks on OSC;
- every exposed command can define a bounded confirmation contract;
- restart invalidates runtime indexes and stale callbacks;
- ready, stale and offline states are deterministic;
- traceability marks all mandatory Phase 1 requirements verified at the
  appropriate mock/real-engine level;
- M1-008 integration gate passes.

Phase 1 may not add native Qt slots, persistence or production process
supervision except narrow scaffolding required to test protocol ownership.

## Phase 2 — managed engine and backend gate

Status: **complete**.
Gate record: `CP-023`.

Deliverables:

- backend capability detector;
- explicit PipeWire-JACK/native-JACK support;
- ALSA-only `backend_unavailable` hard block;
- process supervisor;
- deterministic OSC/JACK names;
- owned-child shutdown/restart;
- JACK client/port discovery;
- Seq66-owned routing and reconnection;
- readiness gate combining process, ping, topology, subscriptions and routing;
- process crash and restart reconciliation tests.

Definition of done:

- engine readiness is verified, not assumed from process existence;
- ALSA-only cannot create, launch or unlock audio clips;
- crash/restart invalidates indexes and reconciles safely;
- no SooperLooper GUI is needed;
- native JACK and PipeWire-JACK capability paths are separately tested;
- failure never deletes project clip state.

## Phase 3 — loop allocation and performer integration

Status: **blocked by Phase 2**.

Deliverables:

- stable clip UUID/runtime-index mapper;
- tested pool, append/remove-last or controlled rebuild policy;
- performer audio command dispatch;
- desired/pending/observed operation model;
- command deadlines and verification queries;
- transport and tempo policy application;
- headless non-Qt control path.

Definition of done:

- one headless audio clip can be allocated, recorded, launched, muted and
  overdubbed through Seq66 control paths;
- displayed state derives from feedback;
- restart rebuilds mapping without persisting raw indexes;
- topology conflicts are blocked during reconciliation;
- MIDI-only behaviour remains functional.

## Phase 4 — first native Qt audio slot

Status: **blocked by Phase 3**.

Deliverables:

- `New MIDI pattern` / `New audio loop` slot actions;
- disabled audio action with backend explanation;
- audio-specific slot widget derived from the common slot abstraction;
- state, progress, pending, stale and error rendering;
- basic record/launch/mute/overdub controls;
- cached meter display;
- equivalent non-GUI control paths for core actions.

Definition of done:

- one audio slot behaves like a first-class Seq66 grid item;
- it remains visible but hard-blocked in ALSA-only mode;
- keyboard, MIDI automation and headless paths cannot bypass the gate;
- paint and UI events perform no synchronous OSC calls;
- UI state is driven by observed snapshots.

## Phase 5 — exact musical recording

Status: **blocked by Phase 4**.

Deliverables:

- arbitrary N-bar recording scheduler;
- time-signature support;
- quantize/round policy;
- observed length verification;
- deterministic synthetic-audio tests;
- transport start/stop edge-case tests;
- timing diagnostics and documented tolerance;
- representative Raspberry Pi validation.

Definition of done:

- required bar/meter matrix passes within documented tolerance;
- failures are surfaced rather than rounded away;
- tempo changes during and after recording follow specified policy;
- target Raspberry Pi configuration is validated under representative load;
- no claim of sample accuracy exceeds executable evidence.

## Phase 6 — transactional persistence

Status: **blocked by Phase 5**.

Deliverables:

- versioned audio project manifest/schema;
- transactional project bundle;
- WAV and/or SooperLooper session save/load policy;
- operation IDs and callback errors;
- missing-media recovery;
- backend-unavailable project loading;
- migration and rollback strategy;
- interrupted-save and partial-failure tests.

Definition of done:

- MIDI and audio project state survive restart;
- failed save never replaces the prior valid project;
- clips remain preserved when opened without JACK/PipeWire-JACK;
- runtime indexes are absent from stable identity;
- migration/version compatibility is explicit and tested.

## Phase 7 — advanced controls

Status: **deferred until core gates**.

Candidate features:

- replace, substitute, multiply and insert workflows;
- undo/redo history indication;
- reverse, one-shot and trigger modes;
- per-loop routing and pan;
- feedback, dry, wet and input gain;
- latency controls;
- clip duplication/import/export;
- waveform/thumbnail rendering;
- audio clips in song mode and playlists.

Each feature requires:

- a stable task ID;
- specified musical meaning;
- outbound validation;
- feedback confirmation;
- negative-path tests;
- headless parity before UI exposure.

## Phase 8 — hardening and release

Status: **deferred**.

Deliverables:

- MIDI-only regression coverage;
- PipeWire and native JACK validation;
- Raspberry Pi soak tests;
- xrun, CPU and memory telemetry;
- accessibility and headless parity review;
- packaging and dependency documentation;
- upstream sync/rebase policy exercised in practice;
- user manual updates and release notes;
- versioned compatibility and migration matrix.

Definition of done:

- normal use never requires the SooperLooper GUI;
- project recovery and diagnostics are actionable;
- unsupported backends fail closed without data loss;
- release artefacts identify exact Seq66 and SooperLooper revisions;
- required workflows and target-hardware gates pass;
- a release checkpoint records all evidence and accepted residual risks.

## Deferred decisions

The following remain open until named evidence exists:

- fixed loop pool versus append/remove-last versus controlled rebuild;
- session file as primary media source versus per-clip WAV authority;
- exact auto-update intervals for meters and position;
- whether an upstream SooperLooper extension is required for exact N-bar close;
- whether future Seq66-managed JACK-over-ALSA is worth supporting;
- waveform cache format;
- integration into song mode and playlists;
- when to switch the GitHub default branch from `master` to `fork-main`.

Deferred decisions are resolved through `DECISIONS.md`, tests, profiling and a
checkpoint. They must not be silently hard-coded by an implementation agent.
