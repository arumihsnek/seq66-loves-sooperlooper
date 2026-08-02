# Roadmap

This roadmap orders work by dependency and risk. A later phase must not bypass
an unmet acceptance gate from an earlier phase.

## Phase 0 — project contract

Status: in progress.

Deliverables:

- fork purpose in root README;
- architecture;
- bidirectional OSC contract;
- normative specification;
- headless test strategy;
- repository agent instructions;
- initial focused CI.

Definition of done:

- documents agree on authority, backend policy and states;
- ALSA-only behaviour is unambiguous;
- outbound and inbound protocol surface is catalogued;
- next implementation tasks can be derived from requirements rather than chat
  history.

## Phase 1 — protocol core

Deliverables:

- typed OSC command/control identifiers;
- strict outbound validation;
- local OSC server/receiver;
- ping and version discovery;
- typed feedback events;
- thread-safe observed-state cache;
- engine generation handling;
- fake-engine protocol tests.

Definition of done:

- exact paths and signatures are tested;
- malformed/late feedback is safe;
- no UI or real-time thread blocks on OSC;
- every command can define a confirmation contract.

## Phase 2 — managed engine and backend gate

Deliverables:

- backend capability detector;
- explicit PipeWire-JACK/native-JACK support;
- ALSA-only `backend_unavailable` hard block;
- process supervisor;
- deterministic OSC/JACK names;
- owned-child shutdown/restart;
- JACK port discovery and routing;
- real SooperLooper + JACK dummy smoke CI.

Definition of done:

- engine readiness is verified by ping, topology and routing;
- ALSA-only cannot create or unlock audio clips;
- crash/restart invalidates indexes and reconciles safely;
- no SooperLooper GUI is needed.

## Phase 3 — loop allocation and performer integration

Deliverables:

- stable clip UUID/runtime-index mapper;
- safe pool or append/remove-last allocation policy;
- performer audio command dispatch;
- desired/pending/observed operation model;
- command deadlines and verification queries;
- transport/tempo policy application.

Definition of done:

- one headless audio clip can be allocated, recorded, launched, muted and
  overdubbed through Seq66 control paths;
- state shown by Seq66 comes from feedback;
- restart rebuilds mapping without persisting raw indexes.

## Phase 4 — first native Qt audio slot

Deliverables:

- `New MIDI pattern` / `New audio loop` slot actions;
- disabled audio action with backend explanation;
- audio-specific slot widget derived from the common slot abstraction;
- state/progress/pending/error rendering;
- basic record/launch/mute/overdub controls;
- cached meter display.

Definition of done:

- one audio slot behaves like a first-class Seq66 grid item;
- it remains visible but hard-blocked in ALSA-only mode;
- paint and UI events perform no synchronous OSC calls.

## Phase 5 — exact musical recording

Deliverables:

- arbitrary N-bar recording scheduler;
- time-signature support;
- quantize/round policy;
- observed length verification;
- deterministic synthetic-audio tests;
- timing diagnostics.

Definition of done:

- required bar/meter matrix passes within documented tolerance;
- failures are surfaced rather than rounded away;
- target Raspberry Pi configuration is validated under representative load.

## Phase 6 — persistence

Deliverables:

- audio project manifest;
- transactional project bundle;
- WAV and/or SooperLooper session save/load;
- operation IDs and callback errors;
- missing-media recovery;
- backend-unavailable project loading.

Definition of done:

- MIDI and audio project state survive restart;
- failed save never replaces the prior valid project;
- clips remain preserved when opened without JACK/PipeWire-JACK.

## Phase 7 — advanced controls

Candidate features:

- replace, substitute, multiply and insert workflows;
- undo/redo history indication;
- reverse, one-shot and trigger modes;
- per-loop routing and pan;
- feedback/dry/wet/input gain;
- latency controls;
- clip duplication/import/export;
- waveform/thumbnail rendering;
- audio clips in song mode and playlists.

Each feature requires defined feedback and headless tests before UI exposure.

## Phase 8 — hardening and release

Deliverables:

- MIDI-only regression coverage;
- PipeWire and native JACK validation;
- Raspberry Pi soak tests;
- xrun/CPU/memory telemetry;
- accessibility and headless parity review;
- packaging and dependency documentation;
- upstream sync/rebase policy;
- user manual updates and release notes.

Definition of done:

- normal use never requires the SooperLooper GUI;
- project recovery and diagnostics are actionable;
- unsupported backends fail closed without data loss;
- release artefacts identify exact Seq66 and SooperLooper revisions.

## Deferred decisions

The following remain open until evidence exists:

- fixed loop pool versus controlled rebuild;
- session file as primary media source versus per-clip WAV authority;
- exact auto-update intervals for meters and position;
- whether an upstream SooperLooper extension is required for exact N-bar close;
- whether future Seq66-managed JACK-over-ALSA is worth supporting;
- waveform cache format;
- integration into song mode and playlists.

Deferred decisions must be resolved by tests, profiling and documented tradeoffs,
not by silently hard-coding the first convenient behaviour.
