# Current project checkpoint

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-02-CP-003-m1-001-typed-protocol.md`

Checkpoint ID: `CP-003`
Checkpoint date: 2026-08-02
Active branch: `feature/m1-001-typed-osc-protocol`
Integration target: `fork-main`
Active pull request: not opened yet
Current phase: `phase-1-protocol-core` — Phase 1, protocol core
Completed phase: `phase-0-project-contract`
Active task: `M1-001`
Task status: `in_progress`

## Minimal resume summary

`master` remains the clean upstream mirror; `fork-main` is the fork integration
line. PR #1 (bootstrap) has been merged into `fork-main` (commit `cb6929c0`).

M1-001 is in progress on `feature/m1-001-typed-osc-protocol`.  A new typed
protocol module (`libseq66/include/audio/sooperlooper_protocol.hpp` and
`libseq66/src/audio/sooperlooper_protocol.cpp`) centralises SooperLooper OSC
command, loop-control and global-control identifiers with bidirectional string
mappings, per-control range/type metadata and bounded observed-state integer
parsing.  A focused unit test
(`tests/audio/sooperlooper_protocol_test.cpp`) covers every mapped identifier
and the canonical SooperLooper state integers.

The previous outbound client (`sooperlooper_client.cpp`) has been migrated to
the new typed accessors; no raw OSC string literals remain in integration code.

The new module, the migrated client and the focused unit test all compile
with warnings-as-errors and the unit test passes locally against
`-Wall -Wextra -Werror -pedantic -std=c++17`.  The existing
`audio_clip_test` and `sooperlooper_osc_contract_test` still pass against the
migrated client.

## Next executable action

Commit the M1-001 work on `feature/m1-001-typed-osc-protocol`, push the branch
and open a draft pull request against `fork-main`.  Then update the immutable
checkpoint with the PR number and the first required-workflow runs.

## Read next

1. the immutable CP-003 checkpoint linked above;
2. `PROJECT-MANIFEST.json`;
3. only `M1-001` in `doc/sooperlooper/WORK-QUEUE.md`;
4. `OSC-CONTROL-AND-FEEDBACK.md` for the canonical contract;
5. affected client code/tests and `TRACEABILITY.md` rows.

Do not treat upstream `NEWS`, `RELNOTES`, `ChangeLog`, mirror `TODO`, mirror
`ROADMAP.md` or old Seq66 planning prose as the active fork plan.

## Handoff rule

This file is a mutable pointer only. At the end of the next material session,
create a new immutable checkpoint and replace this pointer and summary. Never
edit a historical checkpoint.
