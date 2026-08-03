# Autonomous governance adoption

## Purpose

This document describes how to integrate the autonomous governance system
without interrupting an implementation milestone already in progress.

## Stacked introduction

The governance branch is intentionally based on the CP-018 documentation head
rather than claiming the live M2 implementation task. It does not modify
`CURRENT.md`, `WORK-QUEUE.md` task status or implementation source.

The initial pull request may target the CP-018 branch while PR #15 is open. After
PR #15 merges, retarget the governance PR to `fork-main` and resolve only
mechanical documentation/control conflicts.

## Review sequence

1. Confirm PR #15/CP-018 is merged or remains the declared base.
2. Run both project-control validators on the governance exact head.
3. Review role boundaries and L1-L4 classification.
4. Review the autonomous ordinary-task merge conditions.
5. Review human escalation triggers and question format.
6. Review senior-consultation exact-head requirements.
7. Confirm no active task, phase or CURRENT checkpoint was changed by adoption.
8. Obtain an independent senior policy/merge review.
9. Apply blocking findings and repeat CI/review after material changes.
10. Merge as a documentation/governance unit.

## Activation

The policy becomes active only after its PR is merged into `fork-main`.

An in-progress task may continue under its previously authorized workflow until
a safe task boundary. At the next checkpoint, Hermes records that the repository
is operating under `seq66-sl-autonomy-v1` and follows the continuous task loop.

No existing green implementation PR is invalidated solely because it was opened
before adoption. Its merge must, however, satisfy the active policy at the time
of merge.

## First autonomous mission

After activation, the normal user instruction is:

```text
Continue the project under the repository autonomy policy.
```

Hermes recovers state from canonical files and continues until:

- a valid L3 human question;
- an L4 safety stop;
- a meaningful integrated-task report;
- the milestone gate.

## Rollback

If the policy causes an operational problem, do not silently ignore individual
rules. Create a governance task and reviewed PR that updates
`PROJECT-AUTONOMY.json` and all affected human-readable contracts together.

Previously merged implementation remains intact; policy rollback changes future
coordination authority, not product data or Git history.
