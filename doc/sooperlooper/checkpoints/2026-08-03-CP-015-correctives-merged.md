# Checkpoint: Corrective cycle complete — M1-005B/M1-006B/M1-007B

## Checkpoint ID: `CP-015`
## Checkpoint date: 2026-08-03

### Phase
phase-1-protocol-core

### Active task
M1-008 (phase-1 integration gate) — gate verification pending

### Summary
Three corrective branches merged into gate/m1-008-phase-1:
- M1-005B: subscription wire protocol fixed (return_url, return_path, int interval, unregister)
- M1-006B: generation-aware reconciliation with immutable operation copies
- M1-007B: real reordered-event semantics with monotonic timestamps

All test suites pass:
- protocol (1593+ lines new)
- subscription (16 tests)
- receiver
- observed-state (38 tests)
- engine-monitor
- command-confirmation (25 tests)
- fault-injection
- osc-contract

### Evidence
- 16 subscription wire protocol tests pass
- 25 confirmation tracker tests pass
- 38 observed-state cache tests pass
- All 8 test suites green
- CI workflow updated with new test targets

### Next immediate action
Gate verification: build, test, real-engine smoke, CI check.

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-03-CP-015-correctives-merged.md`
