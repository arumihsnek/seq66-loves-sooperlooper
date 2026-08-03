# Checkpoint: M1-007 fault-injection matrix completed

## Checkpoint ID: `CP-012`
## Checkpoint date: 2026-08-03

### Phase
phase-1-protocol-core

### Active task
M1-008 (phase-1 integration gate) — awaiting human review

### Summary
M1-007 (negative and fault-injection matrix) is complete. PR #10 merged
to fork-main. 14 test groups with 28 checks covering queue overflow,
duplicated/reordered callbacks, NaN/Inf rejection, generation mismatch,
shutdown during flow, concurrent access, confirmation timeout, and more.

Also includes M1-006 command confirmation tracker (PR #9, merged).

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-03-CP-012-m1-007-done.md`

### Evidence
- PR #10 merged with merge commit SHA: `a19ea20c`
- Audio integration core: compile-and-test PASS
- Local verification: 28 fault-injection checks PASS
- Existing tests unchanged (no regression)
- Bug fix: apply_field() returns bool for NaN/Inf propagation

### Updated artifacts
- Modified files:
  - `libseq66/src/audio/sooperlooper_observed_state.cpp`
  - `tests/audio/sooperlooper_fault_injection_test.cpp`
  - `.github/workflows/audio-core.yml`
- New files:
  - `libseq66/include/audio/sooperlooper_command_confirmation.hpp`
  - `libseq66/src/audio/sooperlooper_command_confirmation.cpp`
  - `tests/audio/sooperlooper_command_confirmation_test.cpp`

### Next immediate action
M1-008 (phase-1 integration gate). All Phase 1 tasks (M1-001 through
M1-007) are complete. The gate requires human review of:
- Full phase CI evidence
- Traceability status
- Roadmap/manifest advancement
- Definition of done verification
