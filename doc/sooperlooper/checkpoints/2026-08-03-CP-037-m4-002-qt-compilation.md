# CP-037 — M4-002 Qt compilation and real test

Checkpoint ID: `CP-037`
Checkpoint date: 2026-08-03
Phase: `phase-4-native-qt-slot`
Active task: `M4-002`
Status: `done`
Branch: `fork-main`

## Objective

Compile sooperlooper_audio_slot_view.cpp with real Qt5, instantiate widget
in headless (offscreen) test, verify signals, snapshot updates, and absence
of synchronous OSC.

## Completed

- Qt5 installed (qtbase5-dev, libqt5test5)
- MOC generates vtable for Q_OBJECT widget
- Q_DECLARE_METATYPE for transport_action and tempo_mode_selection
- sooperlooper_audio_slot_view_qt_test.cpp: 22 assertions, all pass
- Widget instantiates in offscreen mode
- QSignalSpy verifies signal emission
- No synchronous OSC from any handler
- Model layer still passes 55 assertions

## Verification

| Gate | Evidence | Status |
|---|---|---|
| Qt compilation | g++ with Qt5Widgets + Qt5Test, -Werror | pass |
| Widget instantiation | QApplication offscreen | pass |
| Signal emission | QSignalSpy: transport + tempo | pass |
| No sync OSC | Code inspection + spy count | pass |
| Model regression | 55 assertions | pass |
| Core regression | 9 suites, 428+ assertions | pass |

## Current state

- M4-001: done (model + thin Qt view)
- M4-002: done (Qt compilation + real test)
- M4-003 through M4-007: pending
- Phase 4 DoD: not yet satisfied

## Risks and unresolved questions

| Risk | Classification |
|---|---|
| Visual design deferred | L3 decision when widget is executable |
| Grid integration complexity | M4-003 scope |
| ALSA-only gate testing | M4-004 scope |

## Next executable action

Begin M4-003: grid item integration.

## Open first

1. `PROJECT-MANIFEST.json`
2. `doc/sooperlooper/checkpoints/CURRENT.md`
3. this checkpoint
4. `doc/sooperlooper/WORK-QUEUE.md`
5. M4-003 task definition

## Safe reference point

Phase 4 is in_progress with M4-001 and M4-002 done. M4-003 (grid item
integration) is next.
