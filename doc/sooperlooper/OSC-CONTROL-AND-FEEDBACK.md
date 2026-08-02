# OSC control and feedback contract

## Scope

This document defines the bidirectional protocol used between Seq66 and a
headless SooperLooper engine.

It covers:

- discovery and health;
- commands from Seq66 to SooperLooper;
- loop/global controls;
- feedback subscriptions and callbacks;
- file-operation results;
- loop topology changes;
- validation, timeouts and reconciliation.

The protocol is not treated as fire-and-forget. Seq66 sends intent and consumes
feedback until observed state confirms or rejects that intent.

## Source of truth

The historical SooperLooper `OSC` document is useful but incomplete compared
with the current source. This fork verifies behaviour against:

1. `src/control_osc.cpp` for accepted paths and OSC type signatures;
2. `src/command_map.cpp` for commands, controls, ranges and defaults;
3. the `OSC` document for public semantics;
4. headless integration tests for actual runtime behaviour.

Any mismatch is recorded as a compatibility note and tested. Code must not add
an unverified string merely because it looks plausible.

## Addressing

Loop paths use:

```text
/sl/<index>/...
```

Indexes:

- `0..N-1`: one concrete loop;
- `-1`: all loops where the operation supports it;
- `-3`: selected loop;
- `-2`: internal/global real-time control path used by SooperLooper for some
  global controls; application code should normally use `/set` and `/get`.

Seq66 does not persist indexes as clip identity.

## Discovery and health

### Ping request

```text
/ping  s:return_url  s:return_path
/ping  s:return_url  s:return_path  i:request_id
```

Expected reply payload:

```text
s:engine_url  s:version  i:loop_count
```

Implementations supporting the optional request ID may append/echo it according
to verified engine behaviour. The receiver must tolerate only documented,
tested variants.

A successful ping establishes reachability, not full readiness. Seq66 must also
verify:

- compatible version;
- expected engine generation;
- loop count/topology;
- feedback subscriptions;
- JACK client and required ports;
- required graph connections.

### Health deadlines

- Ping is used during startup, after restart and when feedback becomes stale.
- Repeated missed pings move the engine from `ready` to `stale`, then
  `engine_offline` according to configured deadlines.
- Meter or position silence alone is not proof of death when those values are
  unchanged; state heartbeat/ping and socket/process health are evaluated
  separately.

## Command forms

SooperLooper supports command gestures:

```text
/sl/<index>/down     s:command
/sl/<index>/up       s:command
/sl/<index>/upforce  s:command
/sl/<index>/hit      s:command
```

Seq66 uses `hit` for atomic actions unless a command's verified semantics
require press/release behaviour.

Current command names in SooperLooper source include:

- `record`, `overdub`, `multiply`, `insert`, `replace`, `reverse`;
- `mute`, `mute_on`, `mute_off`, `mute_trigger`;
- `undo`, `redo`, `undo_all`, `redo_all`;
- `scratch`, `trigger`, `oneshot`, `substitute`;
- `pause`, `pause_on`, `pause_off`;
- `solo`, `solo_next`, `solo_prev`;
- `record_solo`, `record_solo_next`, `record_solo_prev`;
- `set_sync_pos`, `reset_sync_pos`;
- `record_or_overdub`;
- `record_exclusive`, `record_exclusive_next`, `record_exclusive_prev`;
- `record_or_overdub_excl` and its next/previous variants;
- `record_or_overdub_solo` and its next/previous/trigger variants;
- `record_overdub_end_solo` and trigger variant.

Not every source command is automatically exposed in the Seq66 UI. Exposure
requires a specified musical meaning, feedback confirmation and tests.

## Per-loop control

### Set

```text
/sl/<index>/set  s:control  f:value
```

### Get

```text
/sl/<index>/get  s:control  s:return_url  s:return_path
```

Expected callback:

```text
i:loop_index  s:control  f:value
```

### Input controls

The current source defines the following loop controls. Ranges/defaults shown
here are source-level values and must be verified against the engine revision
used in CI.

Signal and mix:

- `rec_thresh`: 0..1, default 0;
- `feedback`: 0..1, default 1;
- `use_feedback_play`: boolean, default 0;
- `dry`: 0..1, default 0;
- `wet`: 0..1, default 1;
- `input_gain`: 0..1, default 1;
- `use_safety_feedback`: boolean, default 1.

Rate, stretch and pitch:

- `rate`: 0.25..4, default 1;
- `use_rate`: boolean;
- `stretch_ratio`: source map 0.5..4, default 1; DSP source may internally
  clamp some paths differently, so boundary tests are required;
- `pitch_shift`: -12..12 semitones, default 0;
- `tempo_stretch`: boolean;
- `round_integer_tempo`: boolean.

Sync and quantization:

- `quantize`: indexed mode; historical public values include off/cycle/eighth/
  loop, while current source accepts a wider indexed range. Seq66 must use only
  tested named modes rather than arbitrary integers;
- `round`: boolean;
- `sync`: boolean;
- `playback_sync`: boolean;
- `relative_sync`: boolean;
- `mute_quantized`: boolean;
- `overdub_quantized`: boolean;
- `replace_quantized`: boolean;
- `redo_is_tap`: boolean;
- `jack_timebase_master`: boolean.

Latency and fades:

- `fade_samples`: 0..4096, default 64;
- `input_latency`: 0..32768 samples;
- `output_latency`: 0..32768 samples;
- `trigger_latency`: 0..32768 samples;
- `autoset_latency`: boolean, default 1;
- `auto_latency`/related persisted properties must be treated according to
  tested source behaviour;
- `discrete_prefader`: boolean.

Routing and channels:

- `use_common_ins`: boolean, default 1;
- `use_common_outs`: boolean, default 1;
- `pan_1`, `pan_2`, `pan_3`, `pan_4`: 0..1, default 0.5.

Special controls:

- `scratch_pos`: 0..1;
- `delay_trigger`: change-triggered control.

Seq66 validates ranges before sending and records a deterministic validation
error rather than relying on engine clamping.

## Per-loop observed outputs

These values are queried or subscribed to:

- `waiting`: whether an operation is waiting for a quantized boundary;
- `state`: current engine state;
- `next_state`: pending state;
- `loop_len`: seconds;
- `loop_pos`: seconds;
- `cycle_len`: seconds;
- `free_time`: seconds;
- `total_time`: seconds;
- `rate_output`: actual/true output rate;
- `has_discrete_io`: boolean;
- `channel_count`: integer;
- `in_peak_meter`: absolute sample peak, normally 0..1 but may exceed 1;
- `out_peak_meter`: absolute sample peak, normally 0..1 but may exceed 1;
- `is_soloed`: boolean.

### Engine states

Known public state values:

- `-1`: unknown;
- `0`: off;
- `1`: wait start;
- `2`: recording;
- `3`: wait stop;
- `4`: playing;
- `5`: overdubbing;
- `6`: multiplying;
- `7`: inserting;
- `8`: replacing;
- `9`: delay;
- `10`: muted;
- `11`: scratching;
- `12`: one-shot;
- `13`: substitute;
- `14`: paused;
- `20`: off-muted.

Unknown future values are preserved as raw integers and rendered as
`unknown(<value>)`; they must not crash the receiver or be silently mapped to
`off`.

## Feedback subscriptions

### Per-loop change subscription

```text
/sl/<index>/register_update
    s:control  s:return_url  s:return_path
/sl/<index>/unregister_update
    s:control  s:return_url  s:return_path
```

### Per-loop automatic subscription

```text
/sl/<index>/register_auto_update
    s:control  i:interval_ms  s:return_url  s:return_path
/sl/<index>/unregister_auto_update
    s:control  s:return_url  s:return_path
```

Automatic intervals are accepted in the 10..100 ms range and rounded by the
engine to 10 ms granularity.

Callback payload:

```text
i:loop_index  s:control  f:value
```

### Global subscriptions

```text
/register_update
    s:control  s:return_url  s:return_path
/unregister_update
    s:control  s:return_url  s:return_path
/register_auto_update
    s:control  i:interval_ms  s:return_url  s:return_path
/unregister_auto_update
    s:control  s:return_url  s:return_path
```

Seq66 uses one controlled callback namespace and validates path, type signature,
loop index and control name before updating state.

## Subscription policy

On readiness, Seq66 subscribes to state-bearing values for every allocated
loop.

High-priority, loss-intolerant semantics:

- `state`, `next_state`, `waiting`;
- channel/topology changes;
- operation errors and save/load results.

Periodic/coalescible semantics:

- `loop_pos`;
- `in_peak_meter`, `out_peak_meter`;
- optionally `rate_output`.

Low-rate or change-only semantics:

- `loop_len`, `cycle_len`;
- `is_soloed`, `channel_count`, `has_discrete_io`;
- configured loop controls relevant to the UI.

A reasonable initial policy is:

- position: 20..50 ms;
- meters: 30..100 ms;
- state/waiting: change subscription plus bounded verification;
- length/rate: change subscription or slower auto update.

Exact defaults are performance-tested on the target hardware before becoming
stable configuration.

## Global controls and feedback

### Set/get

```text
/set  s:control  f:value
/get  s:control  s:return_url  s:return_path
```

Current global controls include:

- `tempo`: 0..1000 BPM, default 120;
- `eighth_per_cycle`: 0..2048;
- `sync_source`: indexed source;
- `tap_tempo`;
- `save_loop`;
- `auto_disable_latency`: boolean, default 1;
- `select_next_loop`, `select_prev_loop`, `select_all_loops`;
- `selected_loop_num`: -1 or concrete loop index;
- `output_midi_clock`: boolean;
- `smart_eighths`: boolean;
- `use_midi_start`, `use_midi_stop`: boolean;
- `send_midi_start_on_trigger`: boolean;
- `global_cycle_len`, `global_cycle_pos`.

Historical public sync-source values are:

- `-3`: internal;
- `-2`: MIDI;
- `-1`: JACK;
- `0`: none;
- positive values: loop-based sync using engine-specific indexing.

The integration prefers JACK sync for supported audio backends and uses explicit
Seq66 policy rather than inheriting an arbitrary external engine setting.

## Loop topology

### Add

```text
/loop_add  i:channels  f:minimum_seconds
```

Current source also accepts a request-ID variant. Seq66 may use it only after
its reply/config behaviour is covered by tests.

### Remove

```text
/loop_del  i:index
```

Only `-1` (remove last loop) is considered safe by the public protocol. Seq66
must not delete an arbitrary middle index in normal operation.

### Configuration registration

Current source accepts:

```text
/register    s:return_url  s:return_path
/unregister  s:return_url  s:return_path
```

This mechanism emits engine configuration/topology updates. Its exact callback
schema must be captured from the pinned engine in protocol tests before it is
used as the sole topology source. Until then, Seq66 verifies topology with ping,
loop-count expectations and per-loop queries.

## Load/save operations

### Loop files

```text
/sl/<index>/load_loop
    s:filename  s:return_url  s:error_path

/sl/<index>/save_loop
    s:filename  s:format  s:endian  s:return_url  s:error_path
```

The historical engine writes 32-bit IEEE float WAV regardless of format/endian
arguments. Seq66 treats the resulting file and callback as truth, not the
requested labels.

### Sessions

```text
/load_session
    s:filename  s:return_url  s:error_path

/save_session
    s:filename  s:return_url  s:error_path
/save_session
    s:filename  s:return_url  s:error_path  i:write_audio
```

Every asynchronous operation receives a unique operation ID in Seq66's local
state. Because upstream callback schemas are not uniformly documented, the
receiver must test and normalize success/error variants for the pinned engine.
A timeout is an indeterminate result, not success.

## Errors

Errors are categorized as:

- local validation error;
- backend unavailable;
- engine unreachable;
- OSC send failure;
- malformed/unexpected callback;
- command confirmation timeout;
- routing failure;
- save/load engine error;
- file-system/persistence failure;
- incompatible engine version;
- reconciliation mismatch.

Raw engine text may be retained for diagnostics, but application behaviour uses
stable Seq66 error codes.

## Command confirmation

Each user-visible operation has:

1. preconditions;
2. one or more outbound OSC messages;
3. expected feedback transition(s);
4. a deadline;
5. success, rejection or indeterminate outcome;
6. optional reconciliation query.

Examples:

### Record

Expected progression depends on quantization:

```text
off/playing -> wait_start? -> recording -> wait_stop? -> playing
```

### Mute

```text
playing/overdubbing -> muted
```

A slot in `backend_unavailable` never enters this state machine.

### Apply elastic tempo policy

Seq66 sends global tempo and loop controls, then verifies relevant control/rate
feedback and stable playback. The UI displays a pending indicator until the
observed policy is consistent.

## Reconciliation after startup/restart

Seq66 performs:

1. ping and version check;
2. create/load expected loops;
3. rebuild UUID-to-index mapping for the new engine generation;
4. subscribe to required feedback;
5. query initial state and controls;
6. restore routing;
7. compare observed and desired state;
8. apply missing policy in deterministic order;
9. mark slots available only after verification.

During reconciliation, launch/record controls remain blocked.

## Security and locality

The default endpoint is loopback UDP. Remote OSC control is out of scope for the
initial release.

- callback URLs must be generated by Seq66;
- inbound paths and signatures are allow-listed;
- file paths are constrained to the active project/session policy;
- arbitrary OSC forwarding and arbitrary file writes are forbidden;
- a received message cannot directly invoke UI code or shell execution.
