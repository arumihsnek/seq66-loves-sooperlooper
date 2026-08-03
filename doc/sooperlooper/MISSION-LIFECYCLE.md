# Autonomous mission lifecycle

## Purpose

This document defines the complete lifecycle from a minimal user instruction to
multiple integrated tasks and phase transitions.

The intended normal instruction is:

```text
Continue the project.
```

Hermes then recovers state and runs the loop without requiring a long
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

Recover from repository evidence rather than chat memory.

Classify inconsistent control state as:

- mechanical and safely correctable: bounded autonomous control task;
- ambiguous but non-destructive: senior consultation;
- genuine L3 product/compatibility/data/licensing decision: human form;
- unrecoverable or destructive ambiguity: L4 safety stop.

## Stage 1 — claim one task

Select one ready task whose dependencies are complete. Verify stable ID,
objective, dependencies, requirement IDs, ownership, acceptance criteria,
required evidence and handoff target.

If a phase is too vague, create a decomposition/contract task before functional
implementation.

## Stage 2 — plan and classify

Create the smallest vertically complete plan. Classify each material decision:

- L1: Hermes decides;
- L2: senior review required, then Hermes decides;
- L3: bounded human selection required;
- L4: safety stop.

## Stage 3 — implement

Implement only the selected unit and necessary prerequisites. Keep behaviour,
tests, traceability and contract documentation together. Preserve all
architecture, backend, protocol, persistence and real-time invariants.

## Stage 4 — verify locally

Run the narrowest relevant matrix:

- warnings-as-errors compile;
- focused unit and negative tests;
- enabled/disabled configurations where relevant;
- project-control and autonomy validators;
- `git diff --check`;
- fake-engine, real-engine, backend or hardware evidence appropriate to the
  changed layer.

Record exact commands and outcomes.

## Stage 5 — publish and use CI

Open a draft PR early. Include task/requirement IDs, scope, exclusions,
architecture impact, exact evidence, failures, residual risks and next action.

Diagnose and repair CI autonomously. Never weaken a workflow or assertion merely
to obtain green status.

## Stage 6 — independent review

Request `codex-senior-consult` according to
`SENIOR-CONSULTATION.md`.

For a merge gate provide the exact head. Apply blockers and repeat review after
material changes.

## Stage 7 — merge task

Apply every condition in `AUTONOMOUS-MERGE.md`:

- capture expected head;
- verify exact-head checks and senior verdict;
- verify zero unresolved threads and mergeability;
- mark ready when needed;
- merge with expected-head protection;
- record merge SHA and prove ancestry.

No human permission is required for an eligible task.

## Stage 8 — checkpoint and continue

After integration update task status, traceability, changelog, manifest where
needed, immutable checkpoint, CURRENT and PR/merge evidence. Then select the
next ready task.

## Stage 9 — blocked task

Use focused diagnosis, discriminating tests, senior failure consultation and the
smallest safe correction. Only a real L3/L4 condition creates a human question.
Pause only the blocked branch when independent work can continue.

## Stage 10 — phase gate

When every required task in the current phase is integrated:

1. verify the definition of done item by item;
2. run the full required suite on the exact integrated head;
3. run real-engine, backend and hardware evidence where applicable;
4. state unavailable evidence precisely;
5. audit traceability and control consistency;
6. request a global exact-head senior phase review;
7. classify residual risks as blocking or non-blocking;
8. verify the next phase is already bounded in the approved roadmap and has a
   first real task;
9. classify whether any L3 or L4 trigger exists;
10. create the gate checkpoint and PR.

### Clear phase transition

When the senior accepts without blockers, all required evidence is green, the
next phase is preapproved and bounded, and no L3/L4 trigger exists, Hermes:

1. marks the gate PR ready;
2. re-verifies the exact head;
3. merges with expected-head protection;
4. writes the post-transition checkpoint;
5. updates manifest, roadmap, work queue and CURRENT;
6. starts the next phase's first ready task.

No human message is required.

### Phase transition with an L3 choice

When the transition changes product scope, requires subjective judgement,
introduces intentional incompatibility, data loss, irreversible migration,
licensing policy, unresolved requirement conflict or unavailable physical
acceptance, Hermes pauses only that transition and uses the selection-form
protocol in `HUMAN-ESCALATION.md`.

The phase boundary itself is not a human gate.

## Session ending

A chat or agent process may end at any point. After state changes it leaves the
mandatory checkpoint described in `CHECKPOINTS.md`. A new session resumes at
Stage 0 without private memory.

## Long-running autonomy

Hermes may complete multiple tasks and clear multiple well-defined phases in one
long-running mission.

Send an intermediate human report only when:

- an L3/L4 decision is required;
- a material cross-roadmap contract changes;
- a meaningful progress summary is useful;
- the whole approved roadmap or a major release gate is complete.

Use `templates/AUTONOMOUS-REPORT.md`.
