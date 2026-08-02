# Architecture

## Purpose

This fork turns SooperLooper into a fully managed audio-loop engine underneath
Seq66. Seq66 is the only user-facing project authority. SooperLooper remains a
separate headless process responsible for real-time capture, playback and loop
DSP.

> Seq66 owns intent. SooperLooper owns observed real-time truth. Seq66
> continuously reconciles the two.

Sending an OSC command is not proof that it succeeded. Completion is confirmed
only by SooperLooper feedback or a bounded verification query.

## Process topology

```text
+----------------------------------+
| Seq66                            |
| project and clip model           |
| performer / musical scheduler    |
| Qt UI and headless control       |
| OSC controller + receiver        |
| observed-state cache             |
| process and routing supervisor   |
+----------------+-----------------+
                 | bidirectional OSC/UDP
                 v
+----------------------------------+
| SooperLooper headless engine     |
| JACK client                      |
| capture, playback and loop DSP   |
| state, position, meters, errors  |
+----------------+-----------------+
                 | JACK graph
                 v
+----------------------------------+
| PipeWire-JACK or native JACK     |
+----------------------------------+
```

SooperLooper is not embedded, linked as a DSP library or allowed to own the
project lifecycle.

## Authority

### Seq66 owns

- project directory and persistence;
- stable clip UUIDs and grid positions;
- requested channel count and musical length;
- recording, launch, mute, overdub and replacement intent;
- tempo, time signature, quantization and transport policy;
- free, tape and elastic sync modes;
- independent pitch shift;
- SooperLooper executable, process arguments, OSC port and JACK client name;
- audio-port routing;
- save/load ordering, recovery and shutdown;
- all visible UI state and user-facing errors.

### SooperLooper owns

- the real-time audio callback and sample buffers;
- actual loop/cycle length and position;
- current and next DSP state;
- engine undo/redo history;
- actual meters, rate and latency values;
- execution of WAV/session file operations;
- engine-side error results.

An external SooperLooper GUI may be used only as a development diagnostic. It
is never required in production and never becomes project authority.

## Desired and observed state

Every audio slot has two distinct representations.

### Desired state

Persisted by Seq66:

- clip UUID;
- target routing and channel count;
- musical length and recorded BPM;
- requested action and sync policy;
- requested control values;
- project audio/session references.

### Observed state

Ephemeral and populated from SooperLooper feedback:

- engine generation and health;
- runtime loop index;
- current state, next state and waiting flag;
- loop position, loop length and cycle length;
- true output rate;
- input and output peak meters;
- selected/solo status and channel count;
- last update timestamp and stale flag;
- last confirmed command or operation error.

The UI may show desired state as pending, but must never render it as confirmed
engine state.

## Slot availability states

An audio slot has an availability state independent from its SooperLooper DSP
state.

- `available`: the backend and engine are ready and the slot can be controlled.
- `backend_unavailable`: Seq66 has no supported JACK-compatible audio graph.
- `engine_offline`: a supported backend exists, but the engine is not ready.
- `reconciling`: the engine is being probed or restored.
- `stale`: feedback has stopped within the configured deadline.
- `error`: a non-recoverable project, routing or engine operation failed.

`backend_unavailable` is a hard block, not mute:

- it cannot be cleared with a mute/unmute command;
- no OSC loop command is sent;
- transport cannot launch or record the slot;
- controls that imply an operational loop are disabled;
- the clip remains visible and persisted;
- the UI shows `Audio unavailable — JACK or PipeWire-JACK required`;
- headless mode reports a stable machine-readable error code;
- switching to a supported backend starts normal engine reconciliation.

## Stable identity and loop indexes

SooperLooper addresses loops by mutable numeric indexes. Seq66 stores stable
UUIDs. Runtime therefore maintains:

```text
clip UUID <-> engine generation <-> SooperLooper loop index
```

Restarting SooperLooper invalidates every prior loop index. Since SooperLooper
recommends deleting only the last loop, Seq66 must use a fixed pool,
append/remove-last allocation, or a controlled rebuild while transport is
stopped. Arbitrary deletion followed by index reuse is forbidden.

## Audio backend policy

SooperLooper currently constructs a JACK audio driver. The initial integration
therefore supports only environments where a JACK graph is available.

### Supported

- **PipeWire:** through PipeWire's JACK compatibility layer.
- **Native JACK:** using the selected/default JACK server.

### Unsupported in the initial implementation

- **ALSA-only audio:** no SooperLooper-backed audio tracks.

Seq66 may still use ALSA for MIDI. MIDI backend selection and audio backend
availability are separate facts. When no JACK-compatible audio graph exists:

- existing MIDI functionality remains available;
- `New audio loop` is disabled;
- loading a project containing audio clips preserves them in
  `backend_unavailable` state;
- Seq66 does not silently launch JACK over ALSA;
- the warning identifies PipeWire-JACK or JACK as the required remedy.

Direct ALSA support must not be claimed unless SooperLooper gains a real ALSA
audio driver or a future, explicitly specified Seq66 supervisor manages a
JACK-over-ALSA lifecycle.

## Process supervision

Seq66 launches SooperLooper with deterministic arguments, including:

- zero or a controlled number of initial loops;
- channel count and minimum loop memory policy;
- a chosen OSC port;
- a unique JACK client name;
- an optional JACK server name;
- quiet/headless operation;
- an optional session to restore.

Lifecycle states:

```text
disabled -> starting -> probing -> reconciling -> ready
                                      |             |
                                      v             v
                                    failed <----- stale
                                      |
                                      v
                                  restarting
```

Readiness requires a successful OSC ping, a compatible engine version, the
expected loop count and valid JACK routing. A process ID alone is insufficient.

Seq66 distinguishes between a child process it owns and an explicitly
configured external engine. It may terminate only an owned child.

## OSC threading

- OSC receive runs outside Qt paint handlers and outside real-time MIDI/audio
  callbacks.
- Incoming messages are schema-validated and converted into typed events.
- A thread-safe state cache is updated atomically or through a bounded queue.
- UI and performer callbacks consume snapshots; they never block waiting for
  OSC.
- Meter and position updates may be coalesced.
- State transitions, operation results and errors must not be dropped.

No synchronous network round-trip is allowed from a paint event or real-time
path.

## Transport and tempo

Seq66 is the musical authority. The preferred initial topology is:

- Seq66/JACK transport owns start, stop and tempo intent;
- SooperLooper uses JACK sync where appropriate;
- Seq66 sends explicit tempo/eighths controls required by the selected policy;
- feedback verifies actual loop state and rate.

Three clip policies are supported:

- **free:** no tempo following; duration is sample-time based;
- **tape:** rate follows target tempo and pitch changes with speed;
- **elastic:** duration follows tempo while pitch is preserved by time stretch.

Independent pitch shift may be layered on top within the tested SooperLooper
range.

## Recording exactly N bars

Seq66 schedules the musical operation. The intended sequence is:

1. validate backend, engine, routing and slot allocation;
2. apply quantize, round, sync and tempo controls;
3. arm the slot and expose a pending state;
4. issue record at the selected musical boundary;
5. count the requested bars using Seq66's transport timeline;
6. issue the closing action before/on the final boundary according to the
   tested OSC timing strategy;
7. confirm `playing` or the requested end state through feedback;
8. compare observed length against the expected musical duration;
9. accept within tolerance or surface a deterministic timing error.

OSC delivery is not assumed to be sample accurate. SooperLooper's own
quantization and rounding are part of the contract. If real-engine tests show
that arbitrary N-bar recording cannot be made reliable under load, the project
may add a narrow SooperLooper extension rather than hiding timing errors.

## Routing

Seq66 owns and verifies the audio graph:

- capture source -> SooperLooper loop input;
- SooperLooper loop/common output -> configured playback or downstream bus;
- optional per-loop discrete ports;
- stable logical routing stored by role, not volatile JACK port number.

A slot is not ready until required ports exist and connections match policy.
Unexpected disconnects move it to `stale` or `error`, never silently to
`muted`.

## Persistence

A future project bundle should contain at least:

```text
project.seq66-project/
  project.midi
  audio-manifest.json
  sooperlooper.slsess
  audio/
    <clip-uuid>.wav
```

The manifest maps stable Seq66 identities to media and musical metadata. Raw
runtime loop indexes are never persisted as stable identities.

Saving is transactional at the project level:

1. freeze or snapshot mutable audio state;
2. request SooperLooper session/WAV saves;
3. wait for explicit success/error callbacks;
4. write Seq66 manifest and MIDI data to temporary paths;
5. atomically publish the complete bundle;
6. retain the prior valid project on failure.

## Shutdown and recovery

Graceful shutdown order:

1. stop new commands;
2. finish or cancel pending save operations;
3. unregister updates;
4. request `/quit` from an owned engine;
5. wait a bounded interval;
6. terminate only the owned child if required;
7. disconnect local OSC resources.

On engine loss, Seq66 invalidates the engine generation and every loop index,
marks observed state stale, preserves desired/project state, restarts only when
policy permits, reloads the session or media, rebuilds mappings and verifies
feedback before re-enabling controls.
