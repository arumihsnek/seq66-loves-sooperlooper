# Work queue

## Rules

This is the executable task queue. `ROADMAP.md` defines phase order; this file
defines the next reviewable units of work.

Each active task has:

- a stable ID;
- one status;
- dependencies;
- requirement IDs;
- expected files or ownership area;
- acceptance criteria;
- required tests;
- a concrete handoff target.

Agents claim at most one primary task at a time. Status changes require a
checkpoint.

## Current milestone

Milestone: **M3 — performer integration**

Goal: One headless audio clip can be allocated, recorded, launched, muted and
overdubbed through Seq66 control paths.  Displayed state derives from feedback.
Restart rebuilds mapping without persisting raw indexes.

Active task: **M3-003** (status: `ready`)
## Tasks

### M1-001 — typed protocol identifiers

Status: `done`
Agent: `hermes` session `0bc5d91d41b9`
Branch: `feature/m1-001-typed-osc-protocol` (base `fork-main` @ `c2999d8`)
Draft PR: `#3`
Published implementation head: `1fc306e48c4d5af80cfcf9623b5a0e9b4bf10722`
Started: 2026-08-02
Review entered: 2026-08-03

Dependencies: Phase 0 complete.

Requirement IDs: `OSC-001`, `OSC-002`, `OSC-003`, `STATE-001`.

Expected ownership:

- `libseq66/include/audio/` protocol types;
- `libseq66/src/audio/` mapping/validation;
- `tests/audio/` focused protocol tests;
- protocol documentation and traceability entries.

Deliverables:

- typed command, loop-control, global-control and observed-output identifiers;
- one canonical string mapping in each direction;
- range/type metadata where known;
- preservation of unknown inbound state integers without inventing identifiers;
- removal of new scattered protocol literals from integration code.

Acceptance criteria:

- every identifier used by the current client maps deterministically;
- invalid/unknown outbound identifiers cannot be sent;
- inbound unknown controls are rejected or preserved according to the
  specification;
- fake-engine tests cover valid and invalid mappings;
- audio-disabled build remains possible.

Required tests:

- compile with warnings-as-errors;
- protocol mapping unit tests;
- existing fake-engine OSC contract test;
- metadata consistency test.

Review evidence:

- `Audio integration core` run `30770456443`, job `91556592912`: PASS on
  implementation head `1fc306e48c4d5af80cfcf9623b5a0e9b4bf10722`;
- all three compile steps and all three test executions passed;
- `Project control plane` run `30770456444`, job `91556592918`: PASS on the
  same implementation head;
- the real-engine workflow did not trigger for this diff because its PR path
  filter covers only the real-engine probe, its workflow and related contract
  documentation; the latest verified pinned-engine smoke remains run
  `30756422662`, job `91519250698` from the bootstrap line.

Handoff target: M1-002 can build a receiver without adding untyped protocol
strings, after PR #3 is reviewed and merged.

### M1-002 — receiver lifecycle and strict parser

Status: `done`
Agent: `hermes`
Branch: `feature/m1-002-osc-receiver` (merged to `fork-main`)
Draft PR: `#4` (merged, merge commit `1131a33d2e2e81eb63f2f408f727ee41db42a65`)
Started: 2026-08-02
Completed: 2026-08-03

Dependencies: M1-001.

Requirement IDs: `OSC-004`, `THREAD-001`, `THREAD-002`, `STATE-002`.

Expected ownership:

- local `liblo` server lifecycle;
- callback namespace and signature allow-list;
- typed inbound events;
- receiver shutdown tests.

Acceptance criteria:

- start/stop is deterministic and leak-free;
- malformed path, signature, index and non-finite values are rejected safely;
- callbacks never call Qt or performer code directly;
- late callbacks after shutdown are harmless;
- all waits are bounded and diagnostic.

Required tests:

- fake feedback callbacks;
- malformed payload matrix;
- repeated receiver start/stop;
- sanitizer run when practical.

Review evidence:
- `Audio integration core` run `30774259960` on PR #4 branch: PASS (compile + all 5 receiver test groups).
- Local: `.ci/bin/sooperlooper_receiver_test` PASS; existing audio tests unchanged.
- Atomic `m_dropped_count` + `memory_order_relaxed` load documented; no race.

Handoff target: M1-003 builds an immutable snapshot cache from the typed
receiver events without calling the network.

### M1-003 — observed-state cache

Status: `done`
Agent: `hermes`
Branch: `feature/m1-003-observed-state-cache` (merged to `fork-main`)
Draft PR: `#5` (merged, merge commit `da8ec5cb1a6afc392e17848154af5c4d8bf038f5`)
Started: 2026-08-03
Completed: 2026-08-03

Dependencies: M1-002.

Requirement IDs: `STATE-003`, `STATE-004`, `THREAD-003`.

Expected ownership:

- immutable/snapshot state representation;
- timestamps and freshness;
- loop/global observed values;
- coalescing policy for meters and position.

Acceptance criteria:

- desired and observed state are structurally separate;
- zero is distinguishable from absent/stale;
- state transitions/errors are not intentionally dropped;
- meter/position updates can be coalesced;
- consumers read snapshots without network calls.

### M1-004 — engine generation and stale feedback

Status: `done`
Agent: `hermes`
Branch: `feature/m1-004-engine-generation` (merged to `fork-main`)
Draft PR: `#6` (merged, merge commit `bf0300072a23d7e1efa57565c4abb4643577d81b`)
Started: 2026-08-03
Completed: 2026-08-03

Dependencies: M1-003.

Requirement IDs: `STATE-005`, `ALLOC-001`, `FAIL-001`.

Expected ownership:

- generation token lifecycle;
- UUID/runtime-index mapping boundary;
- stale callback rejection;
- restart/reconciliation state.

Acceptance criteria:

- restart invalidates all runtime indexes;
- old-generation callbacks cannot mutate current state;
- clips retain stable UUIDs;
- tests cover delayed packets from the previous generation.

### M1-005 — ping, discovery and subscriptions

Status: `done`
Agent: `hermes`
Branch: `feature/m1-005-discovery-subscriptions` (merged to `fork-main`)
Draft PR: `#7` (merged, merge commit `2228cfd088f24552c2aa51aae670adb06904051d`)
Started: 2026-08-03
Completed: 2026-08-03

Dependencies: M1-002, M1-003, M1-004.

Requirement IDs: `OSC-005`, `STATE-006`, `FAIL-002`.

Expected ownership:

- ping/version/topology discovery;
- required change and auto-update subscriptions;
- freshness deadlines;
- ready/stale/offline transitions.

Acceptance criteria:

- reachability and readiness remain distinct;
- missing pings and stale feedback produce deterministic transitions;
- subscription intervals are tested and bounded;
- readiness requires compatible version, topology and subscriptions.

### M1-006 — command confirmation contracts

Status: `done`
Agent: `hermes`
Branch: `feature/m1-006-command-confirmation` (merged to `fork-main`)
Draft PR: `#9` (merged, merge commit `d56f8edb`)
Started: 2026-08-03
Completed: 2026-08-03

Dependencies: M1-003, M1-005.

Requirement IDs: `OSC-006`, `STATE-007`, `FAIL-003`.

Expected ownership:

- pending operation records;
- expected feedback transitions;
- deadlines and reconciliation queries;
- indeterminate/error outcomes.

Acceptance criteria:

- OSC send success is never user-visible completion;
- `/set` confirmation tolerates eventual consistency demonstrated by the real
  engine test;
- record/mute/tempo-policy operations can declare expected transitions;
- timeout is not success.

### M1-007 — negative and fault-injection matrix

Status: `done`
Agent: `hermes`
Branch: `feature/m1-007-fault-injection` (merged to `fork-main`)
Draft PR: `#10` (merged, merge commit `a19ea20cda074512884cce029aed9783913b944e`)
Started: 2026-08-03
Completed: 2026-08-03

Dependencies: M1-002 through M1-006.

Requirement IDs: `TEST-003`, `FAIL-004`.

Expected ownership:

- delayed, duplicated and reordered callbacks;
- malformed values;
- process disappearance;
- callback loss and recovery;
- receiver shutdown races.

Acceptance criteria:

- no crash, deadlock or false confirmed state;
- diagnostic error codes are stable;
- tests reproduce failures without long fixed sleeps.

### M1-008 — phase-1 integration gate

Status: `review`
Agent: `hermes`
Branch: `gate/m1-008-phase-1`
Draft PR: `#14`
Completed: 2026-08-03

Blocked by: M1-001 through M1-007.

Requirement IDs: all Phase 1 IDs.

Deliverables:

- full phase CI evidence;
- traceability status updated;
- checkpoint declaring the gate result;
- roadmap and manifest advanced only when all mandatory criteria pass.

Acceptance criteria:

- Phase 1 definition of done in `ROADMAP.md` is met;
- no unresolved required-test failure;
- PR body identifies exact verified scope and remaining Phase 2 risks.

Review evidence:
- `Audio integration core` run `30814274582`: PASS on gate head `50d39dfb`;
- `Project control plane` run `30814274616`: PASS;
- `Real SooperLooper headless smoke` run `30814274591`: PASS;
- `codex-senior-consult` merge-gate: `VALID_ADVISORY_VERDICT`, verdict `accept`,
  execution `efaabd1a-f018-4d53-9ff7-b5138d9005e5`;
- checkpoint `CP-016` records gate PASS.

Handoff target: human approves PR #14; Phase 2 work begins on `fork-main`.

## Deferred queue

These tasks are intentionally not ready before the Phase 1 gate:

- M2 managed process supervisor;
- M2 JACK/PipeWire routing;
- M3 performer integration;
- M4 Qt audio slot;
- M5 exact N-bar recording;
- M6 transactional persistence.

Agents must not bypass Phase 1 to implement UI or persistence prototypes in the
main integration branch.

### M2-001 — Phase 2 decomposition and contract

Status: `done`
Agent: `hermes`
Branch: `feature/m2-001-phase2-contract`
Completed: 2026-08-03
Senior consult: `c335b6d5-2ea3-425b-ba3f-60b67378efb7` (verdict: `continue`)

Dependencies: Phase 1 complete (CP-018).

Requirement IDs: `BACKEND-001`, `BACKEND-002`, `BACKEND-003`, `ARCH-001`.

Acceptance criteria:

- every M2 deliverable from ROADMAP has a corresponding task ID;
- backend detection, ALSA hard-block and supervisor have specified behaviour;
- test strategy covers enabled/disabled configurations;
- no Phase 2 implementation code is included.

Handoff target: M2-002 (backend capability probe) can begin.

### M2-002 — backend capability probe

Status: `done`
Agent: `hermes`
Branch: `feature/m2-002-backend-capability-probe`
Completed: 2026-08-03
PR: `#17` (pending)

Dependencies: none (no M2 dependencies).

Requirement IDs: `BACKEND-001`, `BACKEND-002`, `BACKEND-003`.

Expected ownership:

- `libseq66/include/audio/` backend capability types;
- `libseq66/src/audio/` probe implementation;
- `tests/audio/` probe tests.

Deliverables:

- typed backend capability probe (native JACK, PipeWire-JACK, backend_unavailable, probe_error);
- side-effect-free classification using JACK API capability probe;
- ALSA-only produces backend_unavailable without starting JACK;
- evidence: selected client library, reachable server, implementation metadata.

Acceptance criteria:

- probe compiles with `-Wall -Wextra -Wpedantic -Werror`;
- fake-JACK tests cover: native JACK, PipeWire-JACK, ALSA-only, probe error;
- ALSA-only never starts JACK, PipeWire or SooperLooper;
- probe is independently testable without launching processes.

Required tests:

- unit tests with fake JACK API stubs;
- ALSA-only negative test (no process launch);
- backend_unavailable state correctly produced.

Handoff target: M2-003 (supervisor) consumes typed probe result.

### M2-003 — managed process supervisor core

Status: `done`
Agent: `hermes`
Branch: `feature/m2-003-process-supervisor` (merged to `fork-main`)
PR: `#19` (merged, merge commit `e09930f7`)
Completed: 2026-08-03

Dependencies: M2-002.

Requirement IDs: `ARCH-001`, `THREAD-001`, `THREAD-002`.

Expected ownership:

- `libseq66/include/audio/` supervisor interface;
- `libseq66/src/audio/` supervisor implementation;
- `tests/audio/` supervisor tests.

Deliverables:

- owned-child lifecycle (PID + identity verification);
- serialized event loop on one supervisor context;
- deterministic executable arguments and names;
- bounded shutdown (protocol graceful → TERM → KILL);
- generation-tagged observations;
- never kills externally discovered processes.

Acceptance criteria:

- supervisor compiles with warnings-as-errors;
- fake-process tests cover: start, stop, restart, owned-child verification;
- signals only sent to verified owned child;
- generation increments on each launch attempt;
- no synchronous waits on UI/RT paths.

Required tests:

- lifecycle unit tests with fake process adapter;
- owned-child-only signaling verification;
- generation tagging tests;
- shutdown escalation tests (graceful → TERM → KILL).

Handoff target: M2-004 (engine launch) integrates supervisor with Phase 1 OSC.

### M2-004 — engine launch and OSC reconciliation

Status: `done`
Agent: `hermes`
Branch: `feature/m2-004-engine-launch` (merged to `fork-main`)
PR: `#20` (merged, merge commit `5b8e2ba7`)
Completed: 2026-08-03

Dependencies: M2-003, Phase 1 complete.

Requirement IDs: `OSC-005`, `STATE-005`, `STATE-006`.

Expected ownership:

- `libseq66/src/audio/` launch orchestration;
- `tests/audio/` launch tests.

Deliverables:

- launch SooperLooper only when backend is usable;
- assign deterministic OSC/JACK names;
- connect supervisor generation to Phase 1 monitor;
- reject stale callbacks from old generations.

Acceptance criteria:

- launch gated on usable backend probe;
- deterministic names derived from instance identity + generation;
- Phase 1 engine_monitor receives generation-tagged lifecycle events;
- stale event rejection verified.

Required tests:

- fake-engine launch tests with backend gate;
- deterministic naming verification;
- generation-to-monitor integration tests;
- stale callback rejection tests.

Handoff target: M2-005 (JACK discovery) adds port discovery and routing.

### M2-005 — JACK discovery and Seq66-owned routing

Status: `done`
Agent: `hermes`
Branch: `feature/m2-005-jack-discovery` (merged to `fork-main`)
PR: `#21` (merged, merge commit `2b115706`)
Completed: 2026-08-03

Dependencies: M2-002, M2-004.

Requirement IDs: `BACKEND-001`.

Expected ownership:

- `libseq66/include/audio/` JACK discovery types;
- `libseq66/src/audio/` routing implementation;
- `tests/audio/` routing tests.

Deliverables:

- discover expected JACK clients and ports;
- establish idempotent Seq66-owned connections;
- report incomplete topology;
- reconnect after graph changes.

Acceptance criteria:

- fake-JACK graph tests for discovery and connection;
- idempotent routing (repeated calls produce same result);
- incomplete topology detected and reported;
- JACK-dummy integration test when environment allows.

Required tests:

- fake-JACK graph simulation tests;
- idempotent routing verification;
- topology incompleteness detection;
- JACK-dummy integration (CI environment permitting).

Handoff target: M2-006 (readiness gate) combines all evidence sources.

### M2-006 — readiness gate

Status: `done`
Agent: `hermes`
Branch: `feature/m2-006-readiness-gate` (merged to `fork-main`)
PR: `#22` (merged, merge commit `82e6aa24`)
Completed: 2026-08-03

Dependencies: M2-004, M2-005.

Requirement IDs: `STATE-006`, `FAIL-002`.

Expected ownership:

- `libseq66/include/audio/` readiness types;
- `libseq66/src/audio/` readiness derivation;
- `tests/audio/` readiness tests.

Deliverables:

- derive readiness from: child health, OSC confirmation, JACK topology, routing, backend capability;
- backend_unavailable cannot become ready;
- degraded states properly classified.

Acceptance criteria:

- deterministic unit tests for all readiness combinations;
- backend_unavailable → not ready (verified);
- child exit → not ready;
- OSC loss → not ready;
- routing loss → not ready;
- generation change → readiness re-evaluated.

Required tests:

- unit tests for each readiness condition;
- negative tests (backend_unavailable, child exit, OSC loss);
- generation-change re-evaluation tests.

Handoff target: M2-007 (crash/restart) tests recovery paths.

### M2-007 — shutdown, crash, and restart reconciliation

Status: `done`
Completed: 2026-08-03
Branch: `feature/m2-008-ci-closure`
Merged PR: `#24`
Agent: `hermes`

Dependencies: M2-003 through M2-006.

Requirement IDs: `FAIL-001`, `FAIL-004`, `STATE-005`.

Expected ownership:

- `libseq66/src/audio/` restart policy;
- `tests/audio/` fault injection tests.

Deliverables:

- bounded graceful shutdown with escalation;
- generation rollover on crash;
- stale-state invalidation;
- routing restoration after restart;
- capped exponential backoff with terminal state.

Acceptance criteria:

- crash invalidates generation and all runtime indexes;
- pending operations cancelled or marked indeterminate;
- restart loop has cap and backoff;
- backoff resets after stable interval;
- shutdown during startup handled safely.

Required tests:

- crash injection tests;
- restart backoff verification;
- shutdown-during-startup tests;
- pending-operation cancellation tests.

Handoff target: M2-008 (CI closure) provides real-engine evidence.

### M2-008 — real-engine CI and regression closure

Status: `done`
Completed: 2026-08-03
Branch: `feature/m2-008-ci-closure`
Merged PR: `#24`
Agent: `hermes`

Dependencies: M2-007.

Requirement IDs: `TEST-001`, `TEST-002`, `BACKEND-001`.

Expected ownership:

- `.github/workflows/` CI updates;
- `tests/audio/` integration tests;
- `TESTED-BEHAVIOUR.md` updates.

Deliverables:

- JACK-dummy lifecycle/crash/restart smoke;
- deterministic names and ports verification;
- routing and readiness assertions;
- ALSA-only no-launch test;
- warnings-as-errors build;
- existing MIDI-only regression suite.

Acceptance criteria:

- all three CI workflows pass on gate head;
- real-engine smoke covers lifecycle, crash, restart;
- ALSA-only test proves no JACK launch;
- MIDI-only regression passes;
- TESTED-BEHAVIOUR.md updated.

Required tests:

- real-engine smoke with crash/restart;
- ALSA-only no-launch test;
- MIDI-only regression;
- build with support enabled and disabled.

Handoff target: Phase 2 gate review.

### M1-005A — real OSC ping and subscription transport

Status: `done`
Agent: `hermes`
Branch: `fix/m1-005-real-osc-transport`
Draft PR: `#11` (merged, merge commit `5f30a3b0`)
Completed: 2026-08-03

Dependencies: M1-005.

Requirement IDs: `OSC-005`.

Expected ownership:
- real OSC ping implementation in sooperlooper_client;
- real subscribe/unsubscribe implementation;
- connection to receiver and monitor;
- readiness only after version+topology+subscriptions.

Acceptance criteria:
- ping sends real OSC and parses reply;
- subscribe registers real callbacks;
- readiness requires all prerequisites;
- no synchronous blocking on UI/RT paths.

### M3-001 — clip UUID/runtime-index mapper

Status: `done`
Completed: 2026-08-03
Branch: `feature/m3-001-clip-mapper`
Merged PR: `#25`
Agent: `hermes`

Dependencies: M2-008 (Phase 2 complete).

Requirement IDs: `ALLOC-001`, `STATE-006`.

Expected ownership:

- `libseq66/src/audio/` clip allocation and UUID/index mapping;
- `tests/audio/` allocation tests.

Deliverables:

- stable clip UUID to runtime-index mapper;
- pool with append/remove-last or controlled rebuild policy;
- generation-scoped index validity;
- index invalidation on crash/topology change.

Acceptance criteria:

- UUID is stable across restarts;
- runtime index is generation-scoped;
- crash invalidates all runtime indexes;
- pool append/remove-last semantics verified;
- no index persists as clip identity.

Required tests:

- UUID generation and stability;
- index allocation and invalidation;
- pool append/remove-last;
- crash invalidation.

Handoff target: M3-002 (performer command dispatch).

### M3-002 — performer audio command dispatch

Status: `done`
Completed: 2026-08-03
Branch: `feature/m3-002-command-dispatch`
Merged PR: `#26`
Agent: `hermes`

Dependencies: M3-001.

Requirement IDs: `DISP-001`, `STATE-007`.

Expected ownership:

- `libseq66/src/audio/` command dispatch and operation model;
- `tests/audio/` dispatch tests.

Deliverables:

- desired/pending/observed operation model for audio commands;
- command deadlines and verification queries;
- performer dispatch integrating clip mapper and supervisor.

Acceptance criteria:

- audio command goes through desired → pending → observed lifecycle;
- deadline expiry classified as timeout/indeterminate;
- dispatch integrates with clip mapper for index resolution;
- concurrent commands handled safely.

Required tests:

- command lifecycle (desired → pending → observed);
- deadline expiry handling;
- dispatch with clip mapper integration;
- concurrent command safety.

Handoff target: M3-003 (transport and tempo policy).

### M3-003 — transport and tempo policy integration

Status: `ready`
Agent: `hermes`

Dependencies: M3-002.

Requirement IDs: `TRAN-001`, `TEMPO-001`.

Expected ownership:
- `libseq66/src/audio/` transport policy and tempo application;
- `tests/audio/` transport and tempo tests.

Deliverables:
- tape, free and elastic tempo modes applied to audio loops;
- transport start/stop follows Seq66 global transport;
- tempo changes propagated to SooperLooper via OSC;
- round-trip latency compensation.

Acceptance criteria:
- tape mode: loop plays at recording speed;
- free mode: loop plays at independent speed;
- elastic mode: loop follows global tempo;
- transport sync: start/stop follows Seq66;
- tempo changes propagate within 1 cycle;
- no audio glitch on tempo change.

Required tests:
- tempo mode unit tests;
- transport sync tests;
- tempo propagation timing tests;
- negative tests (unsupported modes rejected).

Handoff target: M3-004 (native Qt audio slots).
