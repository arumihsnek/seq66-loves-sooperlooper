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

| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
|---|---|---|---|---|---|---|
| ARCH-001 | SooperLooper remains a separate headless process | ARCHITECTURE, D-001 | specified | none yet | real-engine fixture proves headless execution | M2 supervisor |
| ARCH-002 | Seq66 owns desired state and lifecycle | ARCHITECTURE, D-002 | specified | initial client only | documentation review | M1/M2 |
| ARCH-003 | Runtime truth comes from observed feedback | SPECIFICATION, D-002 | partially_implemented | real-engine probe only | real-engine `/get` feedback PASS | M1-003, M1-006 |
| BACKEND-001 | Native JACK and PipeWire-JACK are supported audio environments | SPECIFICATION, D-003 | specified | none in application | JACK dummy fixture PASS | M2 backend gate |
| BACKEND-002 | ALSA-only audio yields `backend_unavailable` | SPECIFICATION, D-003, D-004 | specified | none in application | no application test yet | M2 backend gate |
| BACKEND-003 | `backend_unavailable` is not mute and cannot be bypassed | SPECIFICATION, D-004 | specified | none in application | no application test yet | M2 negative gate |

## Protocol

| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
|---|---|---|---|---|---|---|
| OSC-001 | Protocol commands and controls use canonical typed identifiers | OSC contract | partially_implemented | string-based client with centralized methods | fake-engine contract PASS | M1-001 |
| OSC-002 | Outbound indexes, ranges and finite values are validated | OSC contract, SPECIFICATION | partially_implemented | initial client validation | fake-engine invalid-input assertions PASS | M1-001 |
| OSC-003 | Unknown outbound identifiers cannot be emitted | OSC contract | specified | methods restrict current command set | partial mock evidence | M1-001 |
| OSC-004 | Inbound paths/signatures/values are strictly validated | OSC contract | specified | test probe only; no production receiver | none | M1-002 |
| OSC-005 | Ping/version/topology and subscriptions establish readiness | OSC contract | partially_implemented | smoke probe ping only | pinned real-engine ping PASS | M1-005 |
| OSC-006 | Send success is not operation confirmation | SPECIFICATION, D-006 | partially_implemented | real-engine test uses bounded verification | pinned real-engine eventual-confirmation PASS | M1-006 |

## State, identity and failure

| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
|---|---|---|---|---|---|---|
| STATE-001 | Known protocol/state values have deterministic typed mappings | OSC contract | specified | basic audio enums only | none | M1-001 |
| STATE-002 | Receiver callbacks become typed events outside UI/RT paths | ARCHITECTURE | specified | none | none | M1-002 |
| STATE-003 | Desired and observed state are structurally separate | ARCHITECTURE, D-002 | specified | audio clip desired model only | none | M1-003 |
| STATE-004 | Observed values carry presence and freshness timestamps | SPECIFICATION | specified | none | none | M1-003 |
| STATE-005 | Runtime loop indexes are scoped to engine generation | SPECIFICATION, D-005 | specified | none | none | M1-004 |
| STATE-006 | Ready, stale and offline are deterministic distinct states | SPECIFICATION | specified | none | none | M1-005 |
| STATE-007 | Operations have pending, confirmed, failed or indeterminate results | SPECIFICATION | specified | test-only pending logic | real-engine confirmation test PASS | M1-006 |
| ALLOC-001 | Stable clip identity uses UUID, never persisted runtime index | ARCHITECTURE, D-005 | specified | audio model groundwork only | none | M1-004/M3 |
| FAIL-001 | Feedback from obsolete engine generations is ignored | SPECIFICATION | specified | none | none | M1-004 |
| FAIL-002 | Missed health evidence triggers bounded stale/offline transitions | SPECIFICATION | specified | none | none | M1-005 |
| FAIL-003 | Timeout is not success and triggers reconciliation | SPECIFICATION | specified | real-engine test asserts deadline | partial real-engine evidence | M1-006 |
| FAIL-004 | Delayed, duplicated, malformed and reordered callbacks do not corrupt state | HEADLESS-TESTING | specified | none | none | M1-007 |

## Threading

| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
|---|---|---|---|---|---|---|
| THREAD-001 | OSC receive/send never blocks Qt paint or RT paths | ARCHITECTURE | specified | outbound adapter is non-UI and non-RT only | focused compile | M1-002 |
| THREAD-002 | Receiver ownership and shutdown are deterministic | ARCHITECTURE | specified | none | none | M1-002 |
| THREAD-003 | UI/performer consume bounded snapshots/events | ARCHITECTURE | specified | none | none | M1-003 |

## Musical model

| ID | Requirement | Canonical source | Status | Implementation | Evidence | Work item |
|---|---|---|---|---|---|---|
| MUSIC-001 | Clips support arbitrary positive bar counts and time signatures | SPECIFICATION | verified | `audio_clip` | `audio_clip_test` PASS | complete for model |
| MUSIC-002 | Free tempo policy does not follow project BPM | SPECIFICATION | verified | `audio_clip` policy calculation | model test PASS | later real audio validation |
| MUSIC-003 | Tape policy follows BPM by rate and changes pitch | SPECIFICATION | verified at model level | `audio_clip` plus client controls | model/mock tests PASS | M3/M5 real validation |
| MUSIC-004 | Elastic policy follows BPM while preserving pitch | SPECIFICATION | verified at model level | stretch/tempo controls | model/mock tests PASS | M3/M5 real validation |
| MUSIC-005 | Independent pitch shift remains in tested range | OSC contract | partially_implemented | client validation | fake-engine test PASS | advanced control validation |
| MUSIC-006 | Exact N-bar recording is verified against observed length | SPECIFICATION | specified | none | none | Phase 5 |

## Testing and operations

| ID | Requirement | Canonical source | Status | Evidence | Work item |
|---|---|---|---|---|---|
| TEST-001 | Fast model and fake-engine checks compile with warnings-as-errors | HEADLESS-TESTING | verified | `Audio integration core` PASS | maintain |
| TEST-002 | Pinned real engine builds and runs without GUI over JACK dummy | HEADLESS-TESTING | verified | `Real SooperLooper headless smoke` PASS | maintain |
| TEST-003 | Fault-injection matrix covers malformed/late/lost feedback | HEADLESS-TESTING | specified | none | M1-007 |
| OPS-001 | Project state is recoverable from repository control files without chat | WORKFLOW, D-007 | verified | manifest, current checkpoint, work queue and `Project control plane` CI PASS | maintain |
| OPS-002 | Every state-changing agent session leaves a checkpoint | CHECKPOINTS, AGENTS | verified for bootstrap | CP-001 plus validator-enforced pointer/schema | enforce for every material session |
| OPS-003 | Upstream mirror and fork integration history remain separable | UPSTREAM-SYNC | implemented | `master`, `fork-main`, feature branch and PR base | manual review plus future sync rehearsal |
| OPS-004 | PRs and proposed tasks request task IDs, evidence levels and control-file updates | WORKFLOW, AGENTS | implemented | GitHub PR and issue templates present | verify during review |

## Update rules

Update this matrix in the same coherent change when:

- a requirement is added, removed or materially changed;
- implementation status advances;
- a new test changes the strongest evidence level;
- a task is split or replaced;
- a phase gate passes or fails.

Never advance to `verified` from narrative inspection alone. Record the exact
workflow, command, fixture or target hardware that supplied the evidence.
