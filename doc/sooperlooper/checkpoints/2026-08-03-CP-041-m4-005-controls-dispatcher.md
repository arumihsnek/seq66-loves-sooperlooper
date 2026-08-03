# CP-041 — M4-005 controls and dispatcher implemented

Checkpoint ID: `CP-041`
Checkpoint date: 2026-08-03
Phase: `phase-4-native-qt-slot`
Active task: `M4-005`
Status: `done`
Branch: `fork-main`

## Objective

Implement audio slot controls connecting widget actions to command
dispatcher. Record, Launch, Mute, Overdub, Stop actions with
send != confirmation semantics.

## Completed

- `sooperlooper_audio_slot_controls.hpp`: Control request types, action factory
- `sooperlooper_audio_slot_controls.cpp`: Request generation with timestamps
- `sooperlooper_audio_slot_controls_test.cpp`: 31 assertions, all pass
- CI workflow updated

## Verification

| Gate | Evidence | Status |
|---|---|---|
| Record request | type, index, UUID, timestamp | pass |
| Launch request | type, index, UUID | pass |
| Mute request | type, index, UUID | pass |
| Overdub request | type, index, UUID | pass |
| Stop request | type, index, UUID | pass |
| Transport conversion | start→launch, stop→stop | pass |
| Action labels | All 5 types | pass |
| Timestamp | Monotonic, > 0 | pass |
| Core regression | 11 suites, 485+ assertions | pass |

## Current state

- M4-001 through M4-005: done
- M4-006, M4-007: pending
- Phase 4 DoD: not yet satisfied

## Risks and unresolved questions

| Risk | Classification |
|---|---|
| Controls not wired to widget signals yet | Integration in M4-007 |
| Visual design deferred | L3 decision when widget is executable |

## Next executable action

Begin M4-006: state and meter rendering.

## Open first

1. `PROJECT-MANIFEST.json`
2. `doc/sooperlooper/checkpoints/CURRENT.md`
3. this checkpoint
4. `doc/sooperlooper/WORK-QUEUE.md`
5. M4-006 task definition

## Safe reference point

Phase 4 is in_progress with M4-001 through M4-005 done. M4-006
(state and meter rendering) is next.
