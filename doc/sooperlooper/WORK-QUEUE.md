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

Milestone: **M1 — bidirectional protocol core**

Goal: Seq66 can discover a headless SooperLooper engine, receive and validate
feedback, maintain generation-scoped observed state and confirm commands without
blocking UI or real-time paths.

Active task: **M1-001**

## Tasks

### M1-001 — typed protocol identifiers

Status: `ready`

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

Handoff target: M1-002 can build a receiver without adding untyped protocol
strings.

### M1-002 — receiver lifecycle and strict parser

Status: `ready`

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

### M1-003 — observed-state cache

Status: `ready`

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

Status: `ready`

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

Status: `ready`

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

Status: `ready`

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

Status: `ready`

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

Status: `blocked`

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

## Deferred queue

These tasks are intentionally not ready before the Phase 1 gate:

- M2 backend capability detector;
- M2 managed process supervisor;
- M2 JACK/PipeWire routing;
- M3 performer integration;
- M4 Qt audio slot;
- M5 exact N-bar recording;
- M6 transactional persistence.

Agents must not bypass Phase 1 to implement UI or persistence prototypes in the
main integration branch.
