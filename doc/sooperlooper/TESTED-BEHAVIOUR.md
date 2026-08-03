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
