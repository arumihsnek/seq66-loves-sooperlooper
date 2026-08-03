# Tested SooperLooper behaviour

This document records behaviour observed from executable headless tests. It is
not a replacement for the protocol specification; it captures evidence that
constrains the implementation.

## Tested engine

```text
Repository: essej/sooperlooper
Commit:     c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec
Version:    1.7.9
Audio:      JACK dummy, 48000 Hz, 256-frame period
Host CI:    Ubuntu 24.04 GitHub-hosted runner
```

The engine was configured with `--without-gui`, compiled from source, installed
into a temporary prefix and launched with zero initial loops.

## Confirmed fixture behaviour

The following has been demonstrated by the real-engine workflow:

- SooperLooper 1.7.9 builds successfully without its GUI on Ubuntu 24.04;
- it starts as a headless JACK client against JACK dummy;
- a controlled JACK client name is respected;
- common input/output ports are created;
- adding a mono loop creates discrete loop input/output ports;
- `/ping` returns engine URL, version and loop count;
- `/loop_add` changes topology and loop count;
- `/sl/0/get channel_count` returns the created channel count;
- the real-engine probe and callback server compile cleanly with C++17 and
  warnings as errors.

## Control application is asynchronous

A real test sent:

```text
/sl/0/set  "wet"  0.75
```

and immediately queried:

```text
/sl/0/get  "wet"  <callback-url>  <callback-path>
```

The first callback could still contain the previous value. This demonstrates
that a successful OSC send is not synchronous confirmation that the engine's
audio/control cycle has applied the value.

Required implementation behaviour:

1. send the requested control change;
2. keep desired state marked pending;
3. consume update feedback or issue bounded verification queries;
4. complete only when the observed value matches the requested value within
   the control's tolerance;
5. classify deadline expiry as timeout/indeterminate, not success.

Tests must poll or subscribe to observed values with a deadline. They must not
use one immediate `/get` as the sole confirmation.

This evidence reinforces the repository-wide rule:

> Outbound delivery is intent; feedback is runtime truth.

## Hosted JACK warnings

JACK dummy on the hosted runner reports inability to use real-time scheduling
and memory locking. These warnings do not by themselves indicate fixture
failure:

```text
Cannot create RT messagebuffer thread: Operation not permitted
JACK server starting in non-realtime mode
Cannot lock down ... memory area
```

CI protocol/smoke tests may run non-realtime. Timing, xrun and musical-accuracy
gates must run in an environment with appropriate real-time configuration,
including the Raspberry Pi target.

## Evidence lifecycle

When the pinned SooperLooper revision, JACK version or test topology changes:

- retain old evidence when it explains compatibility;
- add a dated/revision-specific section;
- rerun protocol and real-engine tests;
- update the normative contract only when the new behaviour is deliberately
  supported;
- never generalize from one passing smoke test to sample-accurate musical
  guarantees.

## M2-008 managed-engine lifecycle evidence

```text
Date:       2026-08-03
Branch:     feature/m2-008-ci-closure
Evidence:   Ad-hoc compilation and execution on Linux aarch64
```

### Lifecycle smoke (21 assertions)

The following managed-engine lifecycle behaviours have been demonstrated
with a fake process adapter (no real SooperLooper required):

- **Full lifecycle**: launch -> running -> crash -> poll detects -> reconciler
  restarts -> generation invalidated -> routing restored via callback ->
  state returns to watching.
- **Graceful shutdown escalation**: shutdown sends SIGTERM when graceful
  (wait_exit) fails.
- **Pending operation cancellation**: all registered pending operations are
  cancelled when a crash is detected.
- **Terminal state**: after exhausting max_restarts, the reconciler enters
  terminal state and does not attempt further restarts.
- **ALSA-only no-launch**: when the backend probe reports ALSA-only audio,
  no launch attempt is made.
- **Stable-interval backoff reset**: after a stable interval elapses
  (configurable, tested at 3000ms), the restart counter resets and
  backoff returns to the base value.

### Crash reconciler (45 assertions, 19 cases)

The crash reconciler has been tested with deterministic injection:

- Crash detection via supervisor poll
- Exponential backoff with configurable multiplier and cap
- Terminal state after max restarts
- Pending operation register / complete / cancel lifecycle
- Generation invalidation callback on crash
- Shutdown-during-startup handling
- Stable-interval backoff reset
- Routing restoration via restart callback
- State machine transitions: idle -> watching -> reconciling -> backoff/terminal

### Regression

All pre-existing test suites pass without modification:

| Suite | Assertions |
|---|---|
| process supervisor | 86 |
| engine launcher | 62 |
| JACK discovery | 38 |
| readiness gate | 30 |
| crash reconciler | 45 |
| lifecycle smoke | 21 |
| **Total** | **282** |

### Evidence level

These are unit/fake-engine level tests. The fake process adapter
simulates process lifecycle without real SooperLooper or JACK.

- Unit tested (45 assertions)
- Fake-engine protocol tested (21 lifecycle assertions)
- Real SooperLooper tested: NO
- Native JACK tested: NO
- PipeWire-JACK tested: NO
- Hardware tested: NO

Real-engine evidence requires a subsequent phase or CI job with pinned
SooperLooper and JACK dummy backend.

## M3-002 performer audio command dispatch evidence

### Compilation

Compiled on Linux aarch64 with g++ 11.4, `-std=c++17 -Wall -Wextra -Wpedantic -Werror`.
Zero warnings. Links against clip mapper, command confirmation tracker,
observed state cache and protocol — no liblo required.

### Tested behaviour

- desired/pending/confirmed/failed/indeterminate/cancelled lifecycle
- dispatch creates desired command with UUID
- empty clip UUID rejected
- submit resolves clip UUID via mapper
- submit without mapper stays desired
- submit failure transitions to failed
- confirmation via tracker (evaluate + reconcile)
- deadline expiry transitions to indeterminate
- cancel desired and pending commands
- cancel confirmed returns false
- cancel unknown UUID returns false
- cancel_generation invalidates old-generation commands
- status callback fires on lifecycle transitions
- multiple concurrent commands tracked independently
- clear removes all commands
- unique UUID generation across commands

### Regression

All pre-existing test suites pass without modification:

| Suite | Assertions |
|---|---|
| process supervisor | 86 |
| crash reconciler | 45 |
| clip mapper | 43 |
| command dispatcher | 60 |
| lifecycle smoke | 21 |
| **Total** | **255** |

### Evidence level

These are unit/fake-engine level tests.

- Unit tested (60 assertions)
- Integration with clip mapper: verified
- Integration with command confirmation tracker: verified
- Integration with observed state cache: verified
- Real SooperLooper tested: NO
- Native JACK tested: NO
- PipeWire-JACK tested: NO
- Hardware tested: NO

## M3-003 transport and tempo policy evidence

### Compilation

Compiled on Linux aarch64 with g++ 11.4, `-std=c++17 -Wall -Wextra -Wpedantic -Werror`.
Zero warnings.

### Tested behaviour

**Transport policy:**
- sync source mapping: jack (-1), midi (-2), internal (-3), none (0)
- start/stop state transitions
- config propagation (midi start, clock output)
- string conversion and parse

**Tempo policy:**
- free mode: use_rate=false, tempo_stretch=false
- tape mode: use_rate=true, rate=1.0
- elastic mode: use_rate=true, tempo_stretch=true, rate=global/recording
- rate clamped to [0.25, 4.0]
- global tempo change propagates to all loops
- add/remove/clear loops
- tempo mode string conversion and parse
- global tempo clamped to [0, 1000]

### Regression

| Suite | Assertions |
|---|---|
| process supervisor | 86 |
| crash reconciler | 45 |
| clip mapper | 43 |
| command dispatcher | 60 |
| lifecycle smoke | 21 |
| transport+tempo | 110 |
| **Total** | **365** |

### Evidence level

- Unit tested (110 assertions)
- Real SooperLooper tested: NO
- Native JACK tested: NO
- PipeWire-JACK tested: NO
- Hardware tested: NO

## M3-004 audio slot widget model evidence

### Compilation

Compiled on Linux aarch64 with g++ 11.4, `-std=c++17 -Wall -Wextra -Wpedantic -Werror`.
Zero warnings.

### Tested behaviour

- Initial state and defaults (runtime_index, clip_uuid, observed_state, command_status, tempo_mode, transport_playing, last_error, last_state_change_ms)
- has_clip() empty and assigned
- command_terminal() for none/pending/confirmed/failed/indeterminate
- state_label() for all 16 loop states
- command_label() for all 5 command statuses
- tempo_label() for all 3 tempo modes
- Action factory: make_transport, make_tempo, make_both
- MIDI-only regression: no clip, no state change
- Command lifecycle: none → pending → confirmed → none → pending → indeterminate → none → pending → failed
- Tempo mode transitions: free ↔ tape ↔ elastic
- Transport transitions: playing ↔ stopped
- Error message set/clear
- Multiple independent slots

### Regression

| Suite | Assertions |
|---|---|
| process supervisor | 86 |
| crash reconciler | 45 |
| clip mapper | 43 |
| command dispatcher | 60 |
| lifecycle smoke | 21 |
| transport+tempo | 110 |
| audio-slot-widget | 80 |
| **Total** | **445** |

### Evidence level

- Unit tested (80 assertions)
- Qt rendering: NOT TESTED (model-only, no display server required)
- Real SooperLooper tested: NO
- Native JACK tested: NO
- PipeWire-JACK tested: NO
- Hardware tested: NO

## M3-005 audio persistence evidence

### Compilation

Compiled on Linux aarch64 with g++ 11.4, `-std=c++17 -Wall -Wextra -Wpedantic -Werror`.
Zero warnings.

### Tested behaviour

- Serialization round-trip (2 slots, all fields preserved)
- Schema version mismatch detection
- Empty project handling
- CRC32 checksum computation
- State validation (UUID, loop number)
- Transactional save (tmp+fsync+rename)
- Load from missing file
- Load from corrupt file
- Backup creation and restore
- First save with no prior backup
- Persisted slot comparison operators
- Multiple incremental saves

### Regression

| Suite | Assertions |
|---|---|
| process supervisor | 86 |
| crash reconciler | 45 |
| clip mapper | 43 |
| command dispatcher | 60 |
| lifecycle smoke | 21 |
| transport+tempo | 110 |
| audio-slot-widget | 80 |
| audio persistence | 59 |
| **Total** | **504** |

### Evidence level

- Unit tested (59 assertions)
- Transactional write: verified (tmp+fsync+rename)
- Rollback: code-inspected
- Real SooperLooper tested: NO
- Native JACK tested: NO
- PipeWire-JACK tested: NO
- Hardware tested: NO

## M4-001 audio slot view evidence

### Compilation

Compiled on Linux aarch64 with g++ (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0:
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -pthread \
  -Ilibseq66/include \
  libseq66/src/audio/sooperlooper_audio_slot_widget.cpp \
  tests/widgets/sooperlooper_audio_slot_view_test.cpp \
  -o /tmp/slot_view_test

Exit code: 0

### Test execution

/tmp/slot_view_test
Result: 55/55 pass. All assertions passed.

### Coverage

| Test case | Assertions | Status |
|---|---|---|
| Model initial state | 14 | pass |
| Model state labels | 4 | pass |
| Model command labels | 7 | pass |
| Model tempo labels | 3 | pass |
| Model has_clip | 3 | pass |
| Transport action creation | 4 | pass |
| Tempo action creation | 3 | pass |
| Both action creation | 4 | pass |
| Model transport playing | 3 | pass |
| Model state transitions | 5 | pass |
| Model last_error | 3 | pass |

### Full regression (core 8 suites)

| Suite | Assertions | Status |
|---|---|---|
| process_supervisor | 86 | pass |
| crash_reconciler | 45 | pass |
| clip_mapper | 43 | pass |
| command_dispatcher | 60 | pass |
| lifecycle_smoke | 21 | pass |
| transport_tempo_policy | 110 | pass |
| audio_slot_widget | 80 | pass |
| audio_persistence | 59 | pass |
| audio_slot_view | 55 | pass |
| **Total** | **559** | **all pass** |

### CI steps added

1. Compile audio slot view model test
2. Run audio slot view model test

## M4-002 Qt compilation evidence

### Compilation

Compiled on Linux aarch64 with g++ 11.3.0 and Qt 5.15.13:
QTFLAGS=$(pkg-config --cflags --libs Qt5Widgets Qt5Test)
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -pthread \
  -Ilibseq66/include -I.ci/include -I/tmp \
  libseq66/src/audio/sooperlooper_audio_slot_widget.cpp \
  libseq66/src/widgets/sooperlooper_audio_slot_view.cpp \
  /tmp/moc_sooperlooper_audio_slot_view.cpp \
  tests/widgets/sooperlooper_audio_slot_view_qt_test.cpp \
  $QTFLAGS -o /tmp/qt_widget_test

Exit code: 0

### Test execution

QT_QPA_PLATFORM=offscreen /tmp/qt_widget_test
Result: 22/22 pass. All Qt assertions passed.

### Coverage

| Test case | Assertions | Status |
|---|---|---|
| Widget instantiation | 3 | pass |
| Model update | 7 | pass |
| Transport signal | 1 | pass |
| Tempo signal | 1 | pass |
| No synchronous OSC | 2 | pass |
| State label updates | 2 | pass |
| Command label updates | 3 | pass |

### Qt-specific verification

- Widget compiles with real Qt5Widgets and Qt5Test
- QApplication instantiated in offscreen mode (QT_QPA_PLATFORM=offscreen)
- QSignalSpy verifies signal emission
- No synchronous OSC calls from any handler
- Model layer still passes 55 assertions
