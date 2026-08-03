# Current checkpoint — M2-008 in progress

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-03-CP-025-m2-008-ci-closure.md`

Checkpoint ID: `CP-025`
Checkpoint date: 2026-08-03
Phase: `phase-2-managed-engine-backend`
Active task: `M2-008`
Status: `in_progress`
Branch: `feature/m2-008-ci-closure`

## Completed

- M2-001 through M2-007 merged (7 PRs).
- Crash reconciler (45 assertions, 19 cases) merged in PR #23.
- Lifecycle smoke test (21 assertions, 6 cases) implemented.
- TESTED-BEHAVIOUR.md updated with M2-008 evidence.
- CI updated with lifecycle smoke test step.

## In progress

- M2-008: lifecycle smoke test committed, CI updated, pending PR.

## Next action

- Commit M2-008 changes, open PR, verify CI, merge.
- Complete Phase 2 gate with senior review.

## Verification

- Lifecycle smoke: 21/21 assertions pass.
- Regression: 282 total assertions across 6 suites, all pass.
- validate-project-control.py: pass.
- validate-autonomy-policy.py: pass.
