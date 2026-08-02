# Current project checkpoint

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-03-CP-004-m1-001-pr3-ci-green.md`

Checkpoint ID: `CP-004`
Checkpoint date: 2026-08-03
Active branch: `feature/m1-001-typed-osc-protocol`
Integration target: `fork-main`
Active pull request: `#3` (draft)
Current phase: `phase-1-protocol-core` — Phase 1, protocol core
Completed phase: `phase-0-project-contract`
Active task: `M1-001`
Task status: `review`

## Minimal resume summary

PR #1 bootstrap is merged into `fork-main` at `cb6929c0`. PR #2, which adds
the bounded `codex-senior-consult` policy to `AGENTS.md`, is merged at
`c2999d8`. GitHub's default branch is now `fork-main`; `master` remains the
upstream mirror.

M1-001 is published in draft PR #3 from
`feature/m1-001-typed-osc-protocol` to `fork-main`. The implementation head
`1fc306e48c4d5af80cfcf9623b5a0e9b4bf10722` has green fast CI:

- `Project control plane` run `30770456444`, job `91556592918`: PASS;
- `Audio integration core` run `30770456443`, job `91556592912`: PASS;
- all three compile steps and all three focused tests passed.

The real-engine workflow did not trigger for PR #3 because none of its filtered
probe/workflow/contract paths changed. The latest separate pinned-engine PASS
remains run `30756422662`, job `91519250698`; it must not be presented as a new
run on the PR #3 head.

M1-001 is in `review`, not `done`. M1-002 has not started.

## Next executable action

1. Confirm the final control-only branch head has a green
   `Project control plane` run.
2. Review PR #3 and its exact CI evidence.
3. Do not start M1-002 on this branch.
4. Merge PR #3 only with explicit human authorization.
5. After merge, create a fresh M1-002 branch from updated `fork-main` and write
   a new immutable checkpoint.

## Read next

1. the immutable CP-004 checkpoint linked above;
2. `PROJECT-MANIFEST.json`;
3. PR #3 metadata, diff and latest checks;
4. only `M1-001` in `doc/sooperlooper/WORK-QUEUE.md`;
5. affected `TRACEABILITY.md` rows;
6. protocol source/tests only when review finds a concrete issue.

Do not treat upstream `NEWS`, `RELNOTES`, `ChangeLog`, mirror `TODO`, mirror
`ROADMAP.md` or old Seq66 planning prose as the active fork plan.

## Handoff rule

This file is a mutable pointer only. At the end of the next material session,
create a new immutable checkpoint and replace this pointer and summary. Never
edit a historical checkpoint.
