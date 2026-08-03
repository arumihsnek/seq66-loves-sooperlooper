# Checkpoint: M1-004A generation atomicity corrective

## Checkpoint ID: `CP-009`
## Checkpoint date: 2026-08-03

### Phase
phase-1-protocol-core

### Active task
M1-005 (ping, discovery and subscriptions)

### Summary
M1-004A (generation atomicity corrective) is complete. PR #8 merged
to fork-main. Fixes TOCTOU race, wildcard bypass, missing event
provenance, and NaN/Inf acceptance in M1-004 generation tracking.

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-03-CP-009-m1-004a-corrective.md`

### Evidence
- PR #8 merged with merge commit SHA: `691241dc3dc51501e9b11c32e33dc0027bfb3d1d`
- Audio integration core run `30778778423`: compile-and-test PASS (27 test groups)
- Local verification: all 27 tests pass with -Werror
- TSAN: compiled but FATAL on ARM64 kernel 6.17 (documented)
- Existing tests unchanged (no regression)

### Updated artifacts
- Modified files:
  - `libseq66/include/audio/sooperlooper_observed_state.hpp`
  - `libseq66/include/audio/sooperlooper_receiver.hpp`
  - `libseq66/src/audio/sooperlooper_observed_state.cpp`
  - `tests/audio/sooperlooper_observed_state_test.cpp`
  - `.github/workflows/audio-core.yml`

### Next immediate action
Integrate fork-main into PR #7 branch, then begin M1-005 implementation.
