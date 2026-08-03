# Requirement and test traceability

## Purpose

This matrix connects normative behaviour to implementation, tests and work
items. It prevents a feature from being called complete because code exists
without the required feedback, failure-path and backend tests.

Status vocabulary:

- `specified`;
- `partially_implemented`;
- `implemented`;
- `verified`;
- `blocked`;
- `deferred`.

`verified` identifies the strongest evidence currently present. Mock, real
engine and target-hardware verification are recorded separately.

## Architecture and backend

|| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
||---|---|---|---|---|---|---|
|| ARCH-001 | SooperLooper remains a separate headless process | ARCHITECTURE, D-001 | **implemented** | `sooperlooper_process_supervisor` with injectable `process_adapter`; owned-child lifecycle, PID verification, generation tracking, bounded shutdown escalation | fake-process 86 tests PASS; compile with `-Wall -Wextra -Wpedantic -Werror`; `Audio integration core` workflow | M2-003 (in_progress) |
|| ARCH-002 | Seq66 owns desired state and lifecycle | ARCHITECTURE, D-002 | specified | initial client only | documentation review | M1/M2 |
|| ARCH-003 | Runtime truth comes from observed feedback | SPECIFICATION, D-002 | partially_implemented | real-engine probe only | real-engine `/get` feedback PASS | M1-003, M1-006 |
|| BACKEND-001 | Native JACK and PipeWire-JACK are supported audio environments | SPECIFICATION, D-003 | specified | none in application | JACK dummy fixture PASS | M2 backend gate |
|| BACKEND-002 | ALSA-only audio yields `backend_unavailable` | SPECIFICATION, D-003, D-004 | specified | none in application | no application test yet | M2 backend gate |
|| BACKEND-003 | `backend_unavailable` is not mute and cannot be bypassed | SPECIFICATION, D-004 | specified | none in application | no application test yet | M2 negative gate |

## Protocol

|| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
||---|---|---|---|---|---|---|
|| OSC-001 | Protocol commands and controls use canonical typed identifiers | OSC contract | **implemented** | `libseq66/include/audio/sooperlooper_protocol.hpp` and `libseq66/src/audio/sooperlooper_protocol.cpp` provide `sooperlooper_command` (16), `loop_control` (51) and `global_control` (17) typed enums with canonical `to_string()` mappings in each direction; the existing `sooperlooper_client` was migrated to use them and no raw OSC string literals remain in integration code | focused `tests/audio/sooperlooper_protocol_test.cpp` PASS; `audio_clip_test` and `sooperlooper_osc_contract_test` PASS against the migrated client | M1-001 (in_progress) |
|| OSC-002 | Outbound indexes, ranges and finite values are validated | OSC contract, SPECIFICATION | **implemented** | per-control wire-level range validation in `is_in_range(loop_control|global_control, float)` with inclusive/exclusive bound modes; non-finite values are always rejected | focused protocol test exercises valid/low/high/+Inf/-Inf/NaN for every identifier (PASS) | M1-001 (in_progress) |
|| OSC-003 | Unknown outbound identifiers cannot be emitted | OSC contract | **implemented** | `to_string(loop_control)` / `to_string(global_control)` return `""` for out-of-range enum values, and the public client setters accept only the typed enums | focused protocol test includes `try_parse("totally_fake_control", ...)` returning `false` and leaving the output unchanged (PASS) | M1-001 (in_progress) |
||| OSC-004 | Inbound paths/signatures/values are strictly validated | OSC contract | **verified** | `sooperlooper_receiver` strict `(path, types)` allow-list rejects unknown OSC type tags at registration; `trampoline()` converts only i f d h c s S; malformed signatures cannot be registered | receiver test PASS; handler registration rejection tested; `Audio integration core` run `30774259960` | M1-002 (done) |
|| OSC-005 | Ping/version/topology and subscriptions establish readiness | OSC contract | **verified** | `sooperlooper_client::ping()` sends /ping, parses version+loop_count; `subscribe_loop/global()` methods added; `sooperlooper_engine_monitor` tracks lifecycle states | engine monitor 11 tests PASS; client ping API compiled; `Audio integration core` run 30779063918 | M1-005 (done) |
|| OSC-006 | Send success is not operation confirmation | SPECIFICATION, D-006 | **verified** | `command_confirmation_tracker` tracks pending operations; confirm requires observed feedback; send success never treated as completion | confirmation tracker 12 tests PASS; `Audio integration core` run 30779297963 | M1-006 (done) |

## State, identity and failure

|| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
||---|---|---|---|---|---|---|
|| STATE-001 | Known protocol/state values have deterministic typed mappings | OSC contract | **implemented** | `parse_state_int(int, state_parse_result&)` maps the 18 canonical SooperLooper state integers (-1, 0..15, 20) to canonical labels; unknown raw values are preserved verbatim in `result.raw` with `result.known == false` | focused `tests/audio/sooperlooper_protocol_test.cpp::test_state_parsing` PASS, including the `raw=999` unknown path | M1-001 (in_progress) |
|| STATE-002 | Receiver callbacks become typed events outside UI/RT paths | ARCHITECTURE | **verified** | `sooperlooper_receiver` trampoline queues events; `dispatch()` invokes typed callbacks from caller thread only | receiver test PASS; no user code on liblo thread; `Audio integration core` run `30774259960` | M1-002 (done) |
|| STATE-003 | Desired and observed state are structurally separate | ARCHITECTURE, D-002 | **verified** | `loop_observed_state` and `global_observed_state` are separate structs from desired state; `observed_field<T>` carries value + present + timestamp | observed-state cache test PASS (zero vs absent, snapshot immutability); `Audio integration core` run 30777160194 | M1-003 (done) |
|| STATE-004 | Observed values carry presence and freshness timestamps | SPECIFICATION | **verified** | `observed_field<T>` has `present` flag and `timestamp_us`; freshness test verifies timestamp advances; zero meter present=true vs absent | observed-state cache test PASS (freshness, zero vs absent); `Audio integration core` run 30777160194 | M1-003 (done) |
|| STATE-005 | Runtime loop indexes are scoped to engine generation | SPECIFICATION, D-005 | **verified** | `set_generation()` clears cache atomically; `apply()` generation check inside m_mutex (TOCTOU fixed); no wildcard bypass; `receiver_event.generation` stamped at receive time; `apply_event()` typed boundary | observed-state cache 27 tests PASS; M1-004A TOCTOU fix verified; `Audio integration core` run 30778778423 | M1-004/M1-004A (done) |
|| STATE-006 | Ready, stale and offline are deterministic distinct states | SPECIFICATION | **verified** | `engine_state` enum: disabled/starting/reconciling/ready/stale/engine_offline/restarting; `evaluate()` transitions based on deadlines and missed pings; startup deadline -> offline; stale threshold -> stale -> offline | engine monitor 11 tests PASS (state transitions, deadlines, recovery); `Audio integration core` run 30779063918 | M1-005 (done) |
|| STATE-007 | Operations have pending, confirmed, failed or indeterminate results | SPECIFICATION | **verified** | `confirmation_outcome` enum: pending/confirmed/failed/indeterminate/cancelled; `evaluate()` transitions expired ops; `reconcile()` verifies post-deadline | confirmation tracker 12 tests PASS; `Audio integration core` run 30779297963 | M1-006 (done) |
|| ALLOC-001 | Stable clip identity uses UUID, never persisted runtime index | ARCHITECTURE, D-005 | specified | audio model groundwork only | none | M1-004/M3 |
|| FAIL-001 | Feedback from obsolete engine generations is ignored | SPECIFICATION | **verified** | `apply()` generation check inside m_mutex; no wildcard (generation=0 requires explicit match); `apply_event()` validates receiver_event.generation; stale event test confirms no mutation after set_generation() | observed-state cache 27 tests PASS; M1-004A wildcard removal + TOCTOU fix verified; `Audio integration core` run 30778778423 | M1-004/M1-004A (done) |
|| FAIL-002 | Missed health evidence triggers bounded stale/offline transitions | SPECIFICATION | **verified** | `ping_missed()` increments counter; `evaluate()` transitions to stale after `stale_threshold` misses, then to `engine_offline` after `stale_threshold*2`; startup deadline triggers offline | engine monitor 11 tests PASS (stale threshold, deadline, recovery); `Audio integration core` run 30779063918 | M1-005 (done) |
|| FAIL-003 | Timeout is not success and triggers reconciliation | SPECIFICATION | **verified** | `evaluate()` transitions to indeterminate on deadline; `reconcile()` queries engine; timeout never classified as confirmed | confirmation tracker 12 tests PASS (deadline expiry, reconcile); `Audio integration core` run 30779297963 | M1-006 (done) |
|| FAIL-004 | Delayed, duplicated, malformed and reordered callbacks do not corrupt state | HEADLESS-TESTING | **implemented** | `sooperlooper_fault_injection_test.cpp` covers delayed, duplicate, reorder, loss, malformed, shutdown races, queue overflow; assertions on state invariants | fault injection tests compiled and run; `Audio integration core` run on PR #10 | M1-007 (done) |

## Threading

|| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
||---|---|---|---|---|---|---|
|| THREAD-001 | OSC receive/send never blocks Qt paint or RT paths | ARCHITECTURE | **verified** | `sooperlooper_receiver` liblo thread only queues events; `dispatch()` runs from caller thread; `handle()` registration uses a separate handler mutex; `sooperlooper_process_supervisor::poll()` is non-blocking | `sooperlooper_receiver` liblo thread only queues events; `dispatch()` runs from caller thread; `handle()` registration uses a separate handler mutex | receiver test PASS; liblo trampoline does not call user code; `Audio integration core` run `30774259960` | M1-002 (done) |
|| THREAD-002 | Receiver ownership and shutdown are deterministic | ARCHITECTURE | **verified** | `sooperlooper_receiver` start/stop lifecycle; `sooperlooper_process_supervisor` bounded shutdown escalation (graceful → TERM → KILL) with owned-child-only signals | `start()` creates liblo thread and sets `m_running` atomically; `stop()` sets `m_running` false, stops/frees liblo thread, notifies waiters; destructor calls `stop()` | receiver test lifecycle PASS (start/stop/restart); concurrent registration/dispatch test PASS; `Audio integration core` run `30774259960` | M1-002 (done) |
|| THREAD-003 | UI/performer consume bounded snapshots/events | ARCHITECTURE | **verified** | `snapshot()` returns immutable copy under mutex; concurrent read/write test PASS (28K writes, 50K reads in 200ms); no network calls from snapshot | observed-state cache test PASS (snapshot immutability, concurrent access); `Audio integration core` run 30777160194 | M1-003 (done) |

## Musical model

|| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
||---|---|---|---|---|---|---|
|| MUSIC-001 | Clips support arbitrary positive bar counts and time signatures | SPECIFICATION | verified | `audio_clip` | `audio_clip_test` PASS | complete for model |
|| MUSIC-002 | Free tempo policy does not follow project BPM | SPECIFICATION | verified | `audio_clip` policy calculation | model test PASS | later real audio validation |
|| MUSIC-003 | Tape policy follows BPM by rate and changes pitch | SPECIFICATION | verified at model level | `audio_clip` plus client controls | model/mock tests PASS | M3/M5 real validation |
|| MUSIC-004 | Elastic policy follows BPM while preserving pitch | SPECIFICATION | verified at model level | stretch/tempo controls | model/mock tests PASS | M3/M5 real validation |
|| MUSIC-005 | Independent pitch shift remains in tested range | OSC contract | partially_implemented | client validation | fake-engine test PASS | advanced control validation |
|| MUSIC-006 | Exact N-bar recording is verified against observed length | SPECIFICATION | specified | none | none | Phase 5 |

## Testing and operations

|| ID | Requirement | Canonical source | Status | Evidence | Work item |
||---|---|---|---|---|---|
|| TEST-001 | Fast model and fake-engine checks compile with warnings-as-errors | HEADLESS-TESTING | verified | `Audio integration core` PASS | maintain |
|| TEST-002 | Pinned real engine builds and runs without GUI over JACK dummy | HEADLESS-TESTING | verified | `Real SooperLooper headless smoke` PASS | maintain |
|| TEST-003 | Fault-injection matrix covers malformed/late/lost feedback | HEADLESS-TESTING | **implemented** | comprehensive fault injection test suite with 15+ scenarios | fault injection tests PASS; `Audio integration core` run 30779297963 | M1-007 (done) |
|| OPS-001 | Project state is recoverable from repository control files without chat | WORKFLOW, D-007 | verified | manifest, current checkpoint, work queue and `Project control plane` CI PASS | maintain |
|| OPS-002 | Every state-changing agent session leaves a checkpoint | CHECKPOINTS, AGENTS | verified for bootstrap | CP-001 plus validator-enforced pointer/schema | enforce for every material session |
|| OPS-003 | Upstream mirror and fork integration history remain separable | UPSTREAM-SYNC | implemented | `master`, `fork-main`, feature branch and PR base | manual review plus future sync rehearsal |
|| OPS-004 | PRs and proposed tasks request task IDs, evidence levels and control-file updates | WORKFLOW, AGENTS | implemented | GitHub PR and issue templates present | verify during review |

## Update rules

Update this matrix in the same coherent change when:

- a requirement is added, removed or materially changed;
- implementation status advances;
- a new test changes the strongest evidence level;
- a task is split or replaced;
- a phase gate passes or fails.

Never advance to `verified` from narrative inspection alone. Record the exact
workflow, command, fixture or target hardware that supplied the evidence.