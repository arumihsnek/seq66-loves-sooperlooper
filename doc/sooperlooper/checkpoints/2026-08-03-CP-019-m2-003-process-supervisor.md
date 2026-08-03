# CP-019 — M2-003 process supervisor core implemented

Checkpoint ID: `CP-019`
Checkpoint date: 2026-08-03
Phase: `phase-2-managed-engine-backend`
Active task: `M2-003`
Branch: `feature/m2-003-process-supervisor`
PR: `#19` (pending)

## Objective

Implement the managed process supervisor core for headless SooperLooper
engine lifecycle management.

## Completed

- Injectable `process_adapter` interface for testable child lifecycle.
- Typed supervisor states: idle, starting, running, stopping, failed, cooldown.
- Owned-child PID identity verification.
- Bounded shutdown escalation: graceful (OSC /quit) → SIGTERM → SIGKILL.
- Generation tracking incremented on each launch attempt.
- Non-blocking `poll()` for coordinator loop integration.
- Deterministic `build_argv()` for child process arguments.
- Config immutability while process is running.
- Destructor calls shutdown for safety.
- max_restarts limit with automatic restart support.
- 86 test assertions, all passing locally.
- CI workflow updated with compile and run steps.
- PR #19 opened targeting `fork-main`.

## Files

- `libseq66/include/audio/sooperlooper_process_supervisor.hpp` (header)
- `libseq66/src/audio/sooperlooper_process_supervisor.cpp` (implementation)
- `tests/audio/sooperlooper_process_supervisor_test.cpp` (86 assertions)
- `.github/workflows/audio-core.yml` (CI updated)

## Verification

Local:

```text
Branch: feature/m2-003-process-supervisor
Head: 1e2eb5b1

Compile:
  g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -pthread \
      -Ilibseq66/include \
      libseq66/src/audio/sooperlooper_process_supervisor.cpp \
      tests/audio/sooperlooper_process_supervisor_test.cpp \
      -o .ci/bin/sooperlooper_process_supervisor_test
  → CLEAN COMPILE

Test:
  .ci/bin/sooperlooper_process_supervisor_test
  → 86 assertions, 0 failures

Existing tests unchanged:
  .ci/bin/audio_clip_test → PASS
  .ci/bin/sooperlooper_backend_probe_test → PASS
```

CI: pending (PR #19 not yet merged).

## Architecture invariants preserved

- Seq66 owns process lifecycle and intent.
- SooperLooper remains a separate headless process.
- Owned-child-only signaling verified by test.
- No UI or real-time path blocks on process operations.
- Generation-scoped child identity.
- Backend probe (M2-002) consumed by caller, not by supervisor directly.

## Risks and unresolved questions

- Real-process integration tested only via CI (pinned SooperLooper 1.7.9 + JACK dummy).
- SIGKILL failure path exists but is nearly impossible in practice.
- max_restarts limit configurable but not yet wired to Seq66 configuration.
- M2-004 (engine launch + OSC reconciliation) is the next integration step.
- TSAN cannot currently execute in the known ARM64 environment.
- Native JACK and PipeWire-JACK capability paths need separate testing.

## Current state

- Phase 2 M2-003: implemented, PR #19 open.
- M2-004 (engine launch + OSC reconciliation) is the next task.
- All Phase 1 requirements remain verified.
- Backend capability probe (M2-002) is complete and merged.

## Open first

1. `PROJECT-MANIFEST.json`
2. `doc/sooperlooper/checkpoints/CURRENT.md`
3. this checkpoint
4. `doc/sooperlooper/WORK-QUEUE.md`
5. M2-003 task definition
6. PR #19

## Next executable action

Consult `codex-senior-consult` for merge gate review of PR #19 at head
`1e2eb5b1`, then proceed to M2-004 (engine launch and OSC reconciliation).

## Safe reference point

- Head: `1e2eb5b1`
- Base: `a43b910e` (fork-main)
- PR: #19
