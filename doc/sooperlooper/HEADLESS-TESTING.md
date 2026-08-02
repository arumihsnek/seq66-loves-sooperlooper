# Headless testing strategy

## Goals

The integration must be testable without a desktop, audio interface or
SooperLooper GUI.

Tests are layered so that protocol regressions are fast, deterministic and easy
to diagnose, while real-engine and real-hardware tests validate assumptions
that mocks cannot prove.

## Test layers

### Layer 0 — pure model/unit tests

No OSC, JACK or external process.

Covers:

- bar/time-signature duration mathematics;
- free/tape/elastic rate calculations;
- pitch and tempo boundaries;
- availability state machine;
- engine generation and UUID/index mapping;
- desired-versus-observed reconciliation;
- timeout/deadline calculations;
- error normalization;
- persistence schema validation.

These tests MUST run on every change to audio integration code.

### Layer 1 — OSC protocol contract with a fake engine

A small liblo-based fake SooperLooper receives Seq66 messages and emits
controlled callbacks.

Covers:

- exact OSC path and type signature;
- command-name mapping;
- loop/global set/get messages;
- loop add/remove messages;
- ping request/reply;
- registration/unregistration messages;
- valid state/position/meter callbacks;
- malformed paths, signatures, indexes and values;
- delayed, duplicated, reordered and dropped feedback;
- packets from an obsolete engine generation;
- operation timeout and verification behaviour.

The fake engine MUST record received packets as structured data. Assertions
must not depend on console text.

### Layer 2 — real SooperLooper with JACK dummy

A real headless SooperLooper process is launched against a non-hardware JACK
server using the dummy backend.

Representative setup:

```text
jackd -d dummy -r 48000 -p 256
sooperlooper --quiet --loopcount=0 --osc-port=<port> \
  --jack-name=<unique-name>
```

Exact arguments are discovered from the tested engine version and never copied
blindly from this example.

Covers:

- process launch and graceful shutdown;
- ping/version/loop-count reply;
- loop creation/removal;
- command-to-state transitions;
- change and auto-update subscriptions;
- loop position, length and meters;
- control range behaviour;
- save/load result callbacks;
- session restore and topology rebuild;
- engine crash/restart/reconciliation;
- JACK port discovery and connection verification.

The test MUST use bounded waits and print a diagnostic snapshot on failure.
Fixed sleeps may be used only as small polling intervals, never as the sole
success condition.

### Layer 3 — synthetic audio integration

A deterministic JACK test client generates and captures audio.

Inputs may include:

- impulse at a known sample;
- sine wave of known frequency/amplitude;
- click track aligned to bars;
- deterministic pseudo-random signal.

Covers:

- actual capture and playback;
- mono/stereo routing;
- loop-boundary continuity;
- expected loop length in frames;
- tape-mode speed/pitch relation;
- elastic-mode duration and pitch preservation;
- independent pitch shift;
- meter feedback;
- save/reload audio equivalence within defined tolerance.

Audio comparisons use measurable criteria such as frame count, correlation,
frequency estimate, peak and RMS. They do not rely only on listening.

### Layer 4 — PipeWire-JACK integration

Run the engine and tests in a PipeWire environment using JACK compatibility.

Covers:

- backend detection;
- port discovery and connection;
- same logical routing model as native JACK;
- process restart and graph restoration;
- no dependency on the SooperLooper GUI.

This layer may run in a dedicated container/VM or scheduled CI if hosted CI is
not reliable enough for PipeWire services.

### Layer 5 — ALSA-only negative tests

No JACK-compatible graph is available.

Covers:

- MIDI operation remains available;
- `New audio loop` is disabled;
- loaded audio clips become `backend_unavailable`;
- clips remain persisted and visible;
- mute/unmute/launch/record cannot unlock them;
- no OSC loop command is emitted;
- no JACK server is silently launched;
- stable GUI message and headless error code are produced.

This is a required product test, not merely an error-path unit test.

### Layer 6 — target hardware and soak tests

Run on Raspberry Pi 5 and representative desktop systems with real PipeWire or
JACK and an audio interface.

Covers:

- 1, 3, 4 and 5-bar recording;
- 3/4, 4/4, 6/8 and an irregular meter;
- multiple simultaneous MIDI and audio loops;
- tempo changes while playing;
- long-running position/meter subscriptions;
- CPU pressure and scheduling jitter;
- engine restart during idle and playback;
- repeated project save/load;
- at least one multi-hour soak session;
- xruns, memory growth and stale feedback.

Results SHOULD record sample rate, period size, backend, kernel, CPU governor,
SooperLooper revision and Seq66 commit.

## Test matrix

Minimum automated matrix:

```text
C++ compiler: GCC, Clang
Build mode: audio enabled, audio disabled
OSC: mock server, real SooperLooper
JACK: dummy/native
Backend availability: supported, ALSA-only blocked
Channels: mono, stereo
Time signatures: 3/4, 4/4, 6/8
Bars: 1, 3, 4, 5
Sync: free, tape, elastic
```

PipeWire and hardware tests may be scheduled separately but remain release
gates.

## Fake-engine scenarios

The fake engine must support scripts/scenarios such as:

- `happy_record_4_bars`;
- `ping_wrong_version`;
- `loop_add_no_topology_update`;
- `record_wait_start_then_play`;
- `record_timeout_then_get_confirms_playing`;
- `duplicate_state_callback`;
- `late_callback_old_generation`;
- `unknown_state_21`;
- `meter_flood`;
- `malformed_callback_signature`;
- `save_error`;
- `engine_disappears`;
- `routing_missing`.

Scenarios should be data-driven where practical.

## Real-engine fixture lifecycle

Each test suite:

1. allocates unique temporary directories and UDP/JACK names;
2. starts JACK dummy and waits for readiness;
3. starts SooperLooper with deterministic arguments;
4. pings until ready or deadline;
5. runs the test;
6. unregisters callbacks;
7. requests graceful quit;
8. kills only owned remaining processes after a deadline;
9. collects logs, session files and diagnostics on failure;
10. verifies no fixture process leaked.

Parallel tests must not share OSC ports, JACK client names or project paths.

## Timing assertions

Avoid assertions tied to one machine's wall-clock scheduling.

For N-bar recording:

```text
expected_seconds = bars * beats_per_bar * (4 / beat_width) * 60 / bpm
expected_frames  = expected_seconds * sample_rate
```

Tolerance is specified from verified engine behaviour and JACK period size.
Tests report both absolute frame error and musical fraction error.

A test passes because the observed loop is within the defined tolerance and
state feedback is correct, not because a sleep elapsed.

## Feedback freshness tests

The state cache must distinguish:

- known value updated recently;
- known value stale;
- value never observed;
- backend unavailable;
- engine generation changed.

Tests use a controllable clock where possible. Meter zero and missing meter
feedback are distinct cases.

## Fault injection

Required faults include:

- outbound OSC send failure;
- unavailable callback port;
- dropped state transition;
- malformed callback;
- engine SIGTERM/SIGKILL;
- JACK server loss;
- port disconnect;
- disk full/permission error for save;
- missing session/WAV on load;
- incompatible engine version;
- unexpected loop count/index shift.

The expected result is a deterministic availability/operation error and no
corruption of persisted project state.

## CI stages

### Fast PR checks

- compile with warnings as errors;
- pure unit tests;
- fake-engine protocol tests;
- audio-disabled build;
- documentation link/lint checks where available.

### Real-engine PR or merge checks

- build/install pinned SooperLooper;
- JACK dummy fixture;
- discovery, commands, feedback, topology and save/load smoke tests.

### Scheduled/nightly

- full real-engine suite;
- sanitizers where compatible;
- repeated restart/save/load;
- longer meter/position run;
- optional PipeWire environment.

### Hardware validation

Executed by a documented script and producing a machine-readable report stored
as an artifact or attached to a release gate.

## Pinned engine policy

CI MUST identify the exact SooperLooper revision or package version tested.

A version bump requires:

1. source/protocol diff review;
2. contract-test update if needed;
3. real-engine test run;
4. documentation compatibility note;
5. explicit commit/PR evidence.

`master` without a recorded SHA is insufficient for reproducible CI.

## Test naming and diagnostics

Test names should describe behaviour, for example:

```text
backend_unavailable_does_not_emit_unmute
old_generation_feedback_is_ignored
record_four_bars_confirms_length_and_playing_state
unknown_engine_state_is_preserved
save_timeout_does_not_publish_project
```

On failure, output should include:

- Seq66 commit;
- SooperLooper version/revision;
- backend and JACK server details;
- engine generation;
- clip UUID/index mapping;
- recent outbound/inbound OSC events;
- desired and observed snapshots;
- process exits and stderr;
- routing graph;
- temporary artifact paths.

## Initial implementation order

1. fake OSC receiver asserting current outbound client messages;
2. typed feedback event/state cache tests;
3. ping and subscription support;
4. real-engine JACK-dummy smoke test;
5. command confirmation tests;
6. ALSA-only hard-block test;
7. deterministic synthetic-audio N-bar test;
8. PipeWire and Raspberry Pi validation.
