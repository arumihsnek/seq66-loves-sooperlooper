# Audio integration tests

This directory contains headless tests for the Seq66/SooperLooper integration.

## Files

### `audio_clip_test.cpp`

Pure/focused model test covering:

- bar and time-signature duration;
- free, tape and elastic tempo calculations;
- supported tempo/rate boundaries;
- pitch-shift validation;
- basic compile-time OSC support setup.

It does not start an OSC server or SooperLooper process.

### `sooperlooper_osc_contract_test.cpp`

Fast liblo fake-engine test. It starts a local OSC server and verifies the
actual packets emitted by `sooperlooper_client`:

- command path, signature and name;
- per-loop control set;
- global control set;
- loop add/remove;
- elastic policy messages;
- invalid indexes, channel counts and non-finite values emit no packet.

This is the first protocol gate and runs in `audio-core.yml`.

### `sooperlooper_protocol_test.cpp`

Fast liblo test for the typed protocol layer. Verifies typed paths, command
construction, argument validation and rejection of malformed input before any
packet is emitted.

### `sooperlooper_receiver_test.cpp`

Fast liblo fake-engine test for the inbound OSC receiver. It starts a receiver,
registers allow-listed handlers, sends messages to itself and verifies:

- lifecycle: start, port binding, stop, restart;
- dispatch: handled messages invoke their exact (path, types) callback;
- poll_event: unhandled messages remain observable (quarantine);
- wait_event: blocking receive;
- bounded queue: overflow is counted via `dropped_count()`, never grows unbounded;
- concurrency: handler registration racing with dispatch does not corrupt state.

This is the second protocol gate and runs in `audio-core.yml`.

### `sooperlooper_real_engine_smoke.cpp`

Bidirectional smoke probe for a real pinned SooperLooper engine running against
JACK dummy or another controlled JACK-compatible graph.

It verifies:

- ping callback and version;
- initial loop count;
- loop creation and topology feedback through ping;
- per-loop value feedback;
- control set/get;
- loop removal;
- graceful engine quit.

The fixture is defined by `sooperlooper-real-engine.yml` and uses:

- engine OSC port 19953;
- callback port 19952;
- zero initial loops;
- unique JACK client name.

## Test hierarchy

These tests are only the beginning of the hierarchy defined in
`doc/sooperlooper/HEADLESS-TESTING.md`.

Future files should add:

- typed inbound state-cache tests;
- feedback subscription tests;
- old-generation and malformed callback tests;
- process supervisor/backend capability tests;
- ALSA-only hard-block tests;
- deterministic synthetic-audio recording tests;
- persistence and restart tests.

## Rules

- Tests must be headless and bounded.
- Do not use the SooperLooper GUI.
- Do not assume a sent message means the engine changed state.
- Prefer state/feedback waits over fixed sleeps.
- Use unique ports, JACK names and temporary directories when parallelizing.
- Clean up only processes created by the fixture.
- Print actionable diagnostics on failure.
- Update the OSC contract documentation when protocol expectations change.
