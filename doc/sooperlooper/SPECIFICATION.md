# Normative specification

## Conventions

The key words **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT** and **MAY** are
normative.

This specification describes behaviour, not a preferred implementation class
layout. Tests should target these externally observable contracts.

## S1. Product identity

S1.1. The fork MUST preserve upstream Seq66 MIDI behaviour unless a change is
explicitly required for audio-loop integration.

S1.2. Audio loops MUST appear as Seq66 project objects and grid slots, not as a
separate unmanaged application workflow.

S1.3. SooperLooper MUST run headless in normal operation.

S1.4. The SooperLooper GUI MUST NOT be a runtime dependency.

S1.5. Seq66 MUST remain the authority for project state, musical intent,
process lifecycle and UI.

## S2. Backend availability

S2.1. The initial implementation MUST support SooperLooper tracks only when a
JACK-compatible audio graph is available.

S2.2. PipeWire-JACK and native JACK MUST be treated as supported audio paths.

S2.3. ALSA MIDI usage MUST remain possible independently of audio-loop
availability.

S2.4. In an ALSA-only audio environment, Seq66 MUST NOT allow creation,
recording, launch or editing of operational SooperLooper tracks.

S2.5. Existing audio clips loaded in an ALSA-only environment MUST be preserved
and placed in `backend_unavailable` state.

S2.6. `backend_unavailable` MUST NOT be represented as muted.

S2.7. A slot in `backend_unavailable` MUST NOT be unlockable through mute,
unmute, launch or record controls.

S2.8. The GUI MUST display a persistent explanation that JACK or PipeWire-JACK
is required.

S2.9. Headless operation MUST expose a stable error code, initially
`audio_backend_unavailable`.

S2.10. Seq66 MUST NOT silently start JACK over ALSA in the initial release.

## S3. Audio clip model

S3.1. Every audio clip MUST have a stable UUID independent of SooperLooper loop
indexes.

S3.2. Every clip MUST record at least:

- display name;
- channel count;
- number of bars;
- beats per bar;
- beat width;
- recorded BPM;
- sync mode;
- pitch shift;
- media/session reference;
- routing policy.

S3.3. Bar count MUST support arbitrary positive integer values, including odd
lengths such as 1, 3, 5 and 7 bars.

S3.4. Time signatures MUST NOT be restricted to 4/4.

S3.5. Invalid timing, tempo, channel or pitch values MUST be rejected before
sending engine commands.

S3.6. Runtime loop indexes MUST NOT be persisted as stable identity.

S3.7. Every engine restart MUST increment or replace an engine-generation token
and invalidate all prior runtime mappings.

## S4. Availability and runtime states

S4.1. Slot availability MUST be represented separately from SooperLooper DSP
state.

S4.2. Availability values MUST include at least:

- `available`;
- `backend_unavailable`;
- `engine_offline`;
- `reconciling`;
- `stale`;
- `error`.

S4.3. Observed engine state MUST include both raw numeric value and normalized
semantic value.

S4.4. Unknown engine-state integers MUST be preserved and displayed safely.
They MUST NOT be coerced to `off`.

S4.5. Desired state MUST be distinguishable from observed state.

S4.6. A command in flight MAY be rendered as pending, but the final UI state
MUST derive from observed feedback.

S4.7. Loss of feedback MUST result in `stale` or `engine_offline`, not a false
mute/off state.

## S5. Engine lifecycle

S5.1. Seq66 MUST be able to launch a headless SooperLooper child with
deterministic arguments.

S5.2. Arguments MUST include a controlled OSC port and unique JACK client name.

S5.3. Seq66 MUST distinguish owned child engines from externally configured
engines.

S5.4. Seq66 MUST terminate only engines it owns.

S5.5. A process ID or open UDP port MUST NOT alone establish readiness.

S5.6. Readiness MUST require:

- successful ping;
- compatible version;
- expected loop topology;
- active feedback receiver/subscriptions;
- required JACK ports;
- valid routing.

S5.7. Startup and restart MUST use bounded deadlines and stable failure codes.

S5.8. On unexpected engine exit, Seq66 MUST preserve desired/project state,
invalidate runtime mappings and block commands until reconciliation completes.

S5.9. Automatic restart MAY be configurable, but MUST be bounded to avoid an
infinite crash loop.

## S6. OSC transport

S6.1. OSC send and receive MUST run outside real-time MIDI/audio callbacks.

S6.2. Qt paint/event rendering MUST NOT perform synchronous OSC round trips.

S6.3. Every inbound message MUST be validated for path, type signature, value
range and loop index before changing state.

S6.4. Invalid inbound messages MUST be ignored safely and counted/logged.

S6.5. High-frequency meter and position messages MAY be coalesced.

S6.6. State transitions, errors and save/load results MUST NOT be intentionally
coalesced away.

S6.7. Seq66 MUST expose stable internal types rather than spreading raw OSC
strings throughout performer and UI code.

S6.8. Local outbound validation failure MUST NOT emit an OSC packet.

## S7. Bidirectional feedback

S7.1. Seq66 MUST receive SooperLooper feedback; an outbound-only integration is
not feature-complete.

S7.2. Per-loop observed state MUST include at least:

- `state`;
- `next_state`;
- `waiting`;
- `loop_len`;
- `loop_pos`;
- `cycle_len`;
- `rate_output`;
- `channel_count`;
- `is_soloed`;
- input and output peak meters.

S7.3. Seq66 MUST subscribe or query all values needed to render accurate slot
state and confirm operations.

S7.4. Feedback callbacks MUST be associated with the current engine generation.
Late packets from a prior generation MUST NOT mutate current state.

S7.5. Every observed field MUST carry or inherit a freshness timestamp.

S7.6. The UI MUST be able to distinguish a zero meter value from a stale meter.

S7.7. A successful OSC send MUST NOT complete a user operation by itself.

S7.8. Operations MUST define expected state transitions and deadlines.

S7.9. After a timeout, Seq66 MUST classify the operation as indeterminate or
failed and perform a verification query where safe.

## S8. Commands

S8.1. The initial user-facing command set SHOULD include:

- record;
- overdub;
- replace;
- substitute;
- multiply;
- insert;
- mute/unmute;
- trigger;
- one-shot;
- pause;
- reverse;
- undo/redo;
- solo where its project semantics are defined.

S8.2. Source-supported commands MUST NOT automatically appear in the UI without
specified semantics and tests.

S8.3. Seq66 SHOULD use atomic `hit` commands unless press/release semantics are
required and tested.

S8.4. Commands MUST be rejected while a slot is not `available`, except
explicit recovery/diagnostic operations.

S8.5. Command rejection MUST explain the unmet precondition.

## S9. Tempo and pitch policies

S9.1. Clips MUST support `free`, `tape` and `elastic` sync modes.

S9.2. `free` mode MUST keep normal rate/stretch controls neutral unless the
user explicitly changes them.

S9.3. `tape` mode MUST adjust playback rate proportionally to target BPM and
allow pitch to follow speed.

S9.4. `elastic` mode MUST use time stretching to follow target BPM while
preserving pitch, within tested engine limits.

S9.5. Independent pitch shift MUST be constrained to the tested SooperLooper
range, initially -12..12 semitones.

S9.6. Tempo changes outside a clip's supported rate/stretch range MUST NOT be
silently clamped into a musically different result.

S9.7. Unsupported target tempo MUST produce an explicit policy/range error.

S9.8. Seq66 MUST verify relevant rate/stretch feedback after applying a policy.

## S10. Quantized N-bar recording

S10.1. The user MUST be able to request a positive integer number of bars.

S10.2. Seq66 MUST calculate expected duration from bars, time signature and
tempo.

S10.3. Recording start and close MUST be scheduled from Seq66's musical
timeline and configured with SooperLooper quantize/round controls.

S10.4. Seq66 MUST confirm recording state through feedback.

S10.5. Seq66 MUST compare observed loop length with expected musical duration.

S10.6. Length tolerance MUST be defined in samples and/or bounded time relative
to the active sample rate and tested engine behaviour.

S10.7. A loop outside tolerance MUST produce a timing error; it MUST NOT be
silently marked exact.

S10.8. Tests MUST cover at least 1, 3, 4 and 5 bars, 3/4, 4/4 and one compound
or irregular meter.

S10.9. If upstream SooperLooper cannot reliably satisfy arbitrary N-bar
recording under tested load, the project MAY add a narrowly scoped engine
extension with its own specification and tests.

## S11. Routing

S11.1. Seq66 MUST discover SooperLooper JACK ports by controlled client/role
identity, not fragile display ordering.

S11.2. Seq66 MUST create and verify required input/output connections.

S11.3. Routing MUST support at least mono and stereo clips.

S11.4. A slot MUST NOT become `available` before mandatory routing is valid.

S11.5. Unexpected graph disconnection MUST be reported as routing failure or
stale state, not mute.

S11.6. PipeWire-JACK and native JACK routing MUST present the same logical model
to higher layers.

## S12. Loop allocation

S12.1. Seq66 MUST use a policy compatible with SooperLooper's recommendation to
remove only the last loop.

S12.2. Arbitrary middle-index deletion MUST NOT be used in normal operation.

S12.3. Allocation policy MUST be deterministic and tested across restart.

S12.4. A pool/rebuild operation MUST block conflicting transport commands.

S12.5. Runtime mapping MUST be rebuilt and verified after session load.

## S13. Persistence

S13.1. Projects containing audio MUST preserve both MIDI and audio-loop state.

S13.2. Persistence MUST use stable clip UUIDs.

S13.3. Project save MUST wait for explicit SooperLooper save success or error.

S13.4. A timeout MUST NOT be treated as successful save.

S13.5. Project publication MUST be transactional: failure MUST leave the prior
valid project usable.

S13.6. Loading on an unsupported backend MUST preserve audio metadata and media
without attempting playback.

S13.7. Missing media MUST produce a recoverable, visible clip error.

S13.8. Path handling MUST prevent unintended writes outside the active project
or explicitly approved location.

## S14. UI

S14.1. Empty slots MUST eventually offer distinct MIDI-pattern and audio-loop
creation actions when audio is available.

S14.2. When audio is unavailable, the audio creation action MUST be disabled
with an explanation.

S14.3. Audio slots MUST be visually distinguishable from MIDI patterns without
making shared launch/mute concepts inconsistent.

S14.4. Slot feedback SHOULD show current state, progress, pending state and
error/availability status.

S14.5. Meter and progress rendering MUST use cached snapshots and remain
non-blocking.

S14.6. A slot in `backend_unavailable` MUST remain visible, non-launchable and
non-unlockable until backend conditions change.

S14.7. UI operations MUST be possible through equivalent headless/control APIs
where practical.

## S15. Diagnostics

S15.1. Logs MUST identify engine generation, clip UUID and runtime loop index
where applicable.

S15.2. Logs MUST distinguish desired command, OSC send, observed callback and
final operation result.

S15.3. High-rate meter logs MUST be disabled or sampled by default.

S15.4. A diagnostic snapshot SHOULD include backend state, process state, ping
metadata, routing, subscriptions, loop mappings and freshness timestamps.

S15.5. User-facing messages MUST be concise; full raw engine diagnostics MAY be
available in logs/details.

## S16. Compatibility and licensing

S16.1. The integration MUST remain optional at build/runtime where feasible.

S16.2. Platforms without liblo/JACK support MUST still compile with audio-loop
features disabled.

S16.3. Upstream Seq66 and SooperLooper attribution and license requirements MUST
be preserved.

S16.4. Changes intended for upstream Seq66 MUST remain separable from
fork-specific integration when practical.

## S17. Acceptance gates

A phase is not complete until its relevant tests pass.

### Protocol-core gate

- outbound paths/signatures verified by a mock server;
- malformed input rejected;
- state cache and generation handling tested;
- no real-time/UI blocking calls.

### Real-engine headless gate

- engine launch and ping;
- loop allocation;
- command/state feedback;
- meters/position subscriptions;
- save/load callbacks;
- graceful quit and forced-loss recovery.

### Backend gate

- native JACK test;
- PipeWire-JACK test where CI infrastructure permits;
- ALSA-only hard-block test.

### Musical gate

- exact/tolerant N-bar recordings across required meters and tempos;
- tempo changes in free, tape and elastic modes;
- pitch-shift boundaries;
- overload/jitter tests on target hardware.

### Product gate

- MIDI-only regression suite passes;
- project save/load is transactional;
- GUI and headless controls agree;
- no SooperLooper GUI required.
