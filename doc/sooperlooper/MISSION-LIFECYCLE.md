# Autonomous mission lifecycle

## Purpose

This document defines the complete lifecycle from a minimal user instruction to
multiple integrated tasks, milestone completion and the next human decision.

The intended normal instruction is simply:

```text
Continue the project.
```

Hermes then recovers state and runs the loop below without requiring a long
session-specific prompt.

## Stage 0 — recover

Read in this order:

1. `/PROJECT-MANIFEST.json`;
2. `/PROJECT-AUTONOMY.json`;
3. `/AUTONOMY.md`;
4. `checkpoints/CURRENT.md`;
5. the active `WORK-QUEUE.md` task;
6. active PR metadata and CI;
7. linked traceability, architecture, specification and tested-behaviour
   sections;
8. exact source and tests owned by the task.

Verify repository, branch, PR base, active task and checkpoint consistency.
Recover from repository evidence rather than chat memory.

If control files disagree, classify the discrepancy:

- mechanical and safely correctable: create a bounded control task;
- ambiguous but non-destructive: consult senior;
- phase/product authority conflict: ask the human;
- unrecoverable or destructive ambiguity: safety stop.

## Stage 1 — claim one task

Select one ready task whose dependencies are complete.

Before editing, verify it has:

- stable ID;
- objective;
- dependencies;
- requirement IDs;
- expected ownership;
- deliverables;
- acceptance criteria;
- required tests;
- handoff target.

If the active milestone is insufficiently decomposed, create a documentation
and contract task first. Do not begin a broad implementation from a vague phase
heading.

Move the task to `in_progress` and establish branch/PR ownership according to
the existing workflow.

## Stage 2 — plan and classify decisions

Create the smallest vertically complete implementation plan.

For each material decision classify it:

- L1: Hermes decides;
- L2: senior review required, Hermes decides after review;
- L3: human decision required;
- L4: safety stop.

A plan should identify:

- changed contracts;
- source and test paths;
- ownership and lifecycle;
- expected failure cases;
- evidence levels achievable in the environment;
- rollback/reference point;
- deferred scope.

## Stage 3 — implement

Implement only the selected unit and necessary prerequisites.

Keep behaviour, tests, traceability and contract documentation together.
Do not mix unrelated refactors.

Use bounded waits and explicit state transitions. Preserve architecture,
backend, process, protocol and persistence invariants from `AGENTS.md`.

## Stage 4 — verify locally

Run the narrowest relevant matrix before relying on remote CI:

- warnings-as-errors compile;
- focused unit tests;
- negative/fault tests;
- configuration enabled/disabled tests where relevant;
- validator and `git diff --check`;
- fake-engine or real-engine tests according to the changed layer.

Record exact commands and outcomes. Do not write “all tests pass” without naming
the evidence level.

## Stage 5 — publish and use CI

Open a draft PR early. The PR is the review surface and contains:

- task and requirement IDs;
- scope and exclusions;
- architecture impact;
- exact evidence;
- known failures and residual risks;
- next executable action.

Use CI failures as concrete evidence. Diagnose and repair them autonomously.
Never weaken a required workflow or assertion merely to make the PR green.

## Stage 6 — independent review

Request `codex-senior-consult` according to
`SENIOR-CONSULTATION.md`.

For a merge gate, provide the exact head SHA and request blocking/non-blocking
classification.

Apply blockers, add regression tests and repeat review after material changes.

## Stage 7 — merge ordinary task

Apply every condition in `AUTONOMOUS-MERGE.md`.

When all conditions pass:

- capture expected head;
- mark ready when needed;
- reverify checks and mergeability;
- merge using expected-head protection and the required method;
- capture merge SHA;
- fast-forward local integration branch;
- prove ancestry.

No human permission is required for an ordinary eligible task.

## Stage 8 — checkpoint and continue

After integration:

- update task status and next ready task;
- update traceability and changelog;
- record durable decisions;
- update manifest when active branch/task/phase or compatibility changes;
- write one immutable checkpoint;
- update CURRENT;
- update PR/merge evidence;
- select the next ready task.

Continue automatically inside the same milestone.

## Stage 9 — blocked task

A blocked ordinary task follows this order:

1. focused self-diagnosis;
2. discriminating test or inspection;
3. senior failure consultation when needed;
4. smallest safe correction;
5. regression evidence;
6. resume task.

Only L3/L4 conditions create a human question. Use
`HUMAN-ESCALATION.md`.

When possible, pause only the blocked branch and continue independent tasks.

## Stage 10 — milestone gate

When every milestone task is integrated:

1. verify the roadmap definition of done item by item;
2. run the full required suite;
3. run pinned real-engine and backend/hardware evidence where applicable;
4. state unavailable evidence precisely;
5. audit traceability and control consistency;
6. request global senior milestone review;
7. create the gate checkpoint and PR;
8. ask one explicit human approval question.

The human answer is the only normal interruption between milestones.

After approval Hermes:

- updates the gate PR body;
- marks ready and merges with expected-head protection;
- writes the post-merge checkpoint;
- changes manifest/roadmap/work queue to the next milestone;
- begins its first ready task.

## Session ending

A particular chat or agent process may end at any point. Before ending after any
state change it must leave the mandatory checkpoint described in
`CHECKPOINTS.md`.

A new session resumes at Stage 0. No private chain of thought, hidden memory or
conversation transcript is required.

## Long-running autonomy

Hermes may complete multiple ordinary tasks in one long-running mission. It
should not produce a human report after every command.

Send an intermediate report only when:

- an L3/L4 human decision is required;
- a task materially changes an approved cross-phase contract;
- a meaningful set of tasks is integrated;
- the milestone is complete.

Use `templates/AUTONOMOUS-REPORT.md`.
