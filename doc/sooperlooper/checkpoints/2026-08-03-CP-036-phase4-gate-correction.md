# CP-036 — Phase 4 gate correction

Checkpoint ID: `CP-036`
Checkpoint date: 2026-08-03
Phase: `phase-4-native-qt-slot`
Active task: `M4-002`
Status: `in_progress`
Branch: `fork-main`

## Objective

Correct premature Phase 4 gate classification. CP-035 declared Phase 4
complete based on PR #30 (commit 8be93cf1), but PR #30 only implemented
the minimum view/model layer (M4-001), not the full Phase 4 DoD.

## Correction rationale

PR #30 implemented:
- audio_slot_model: loop_state, command_feedback, tempo_mode_selection
- audio_slot_view: thin Qt widget with labels, Start/Stop, tempo combo, signals
- audio_slot_view_test: 55 assertions (model layer only, no Qt compilation)

PR #30 did NOT implement:
- Integration into Seq66 grid abstraction
- "New MIDI pattern" / "New audio loop" slot actions
- ALSA-only backend gate with visual explanation
- Gate enforcement from UI, keyboard, MIDI and headless
- Record, Launch, Mute, Overdub controls
- Cached meter display
- Pending/stale/error differentiated rendering
- Real view → dispatcher → observed feedback connection
- First-class behavior within main window
- Qt compilation or execution in CI

## Completed

- PR #30 merged as M4-001 (minimum view/model)
- CP-035 reclassified as historical
- Phase 4 decomposition into M4-001 through M4-007

## Verification

| Gate | Evidence | Status |
|---|---|---|
| PR #30 merged | commit 8be93cf1 | pass |
| M4-001 model tests | 55 assertions | pass |
| Phase 4 DoD | NOT satisfied | blocker |

## Classification

- CP-035 is historical and immutable
- PR #30 is accepted as M4-001 (minimum view/model)
- Phase 4 remains in_progress
- Phase 4 DoD is defined in ROADMAP.md (not reduced)
- No evidence exists to mark Phase 4 complete

## Current state

- Exact head: `8be93cf1` (PR #30 merge)
- M4-001: done (model layer + thin Qt view)
- M4-002 through M4-007: pending
- Phase 4 DoD not satisfied

## Phase 4 decomposition

| Task | Description | Status |
|---|---|---|
| M4-001 | Minimum view/model (PR #30) | done |
| M4-002 | Qt compilation and real test | pending |
| M4-003 | Grid item integration | pending |
| M4-004 | Backend gate (ALSA-only) | pending |
| M4-005 | Controls and dispatcher | pending |
| M4-006 | State and meter rendering | pending |
| M4-007 | Phase 4 gate review | pending |

## Risks and unresolved questions

| Risk | Classification |
|---|---|
| No Qt rendering in CI | M4-002 must address |
| Visual design deferred | L3 decision when widget is executable |
| Grid integration complexity | M4-003 scope |

## Next action

Begin M4-002: compile sooperlooper_audio_slot_view.cpp with real Qt,
instantiate widget in headless test, verify signals and update.

## Next executable action

Begin M4-002: Qt compilation and real test.

## Open first

1. `PROJECT-MANIFEST.json`
2. `doc/sooperlooper/checkpoints/CURRENT.md`
3. this checkpoint
4. `doc/sooperlooper/WORK-QUEUE.md`
5. M4-002 task definition

## Safe reference point

Phase 3 is complete and gate-passed. Phase 4 is in_progress with M4-001
done (PR #30) and M4-002 as next task.
