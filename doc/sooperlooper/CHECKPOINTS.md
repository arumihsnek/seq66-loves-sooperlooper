# Checkpoint protocol

## Purpose

Checkpoints are durable handoffs between agents and human sessions. They make
continuation possible without replaying conversations.

`checkpoints/CURRENT.md` is the compact entry point and points to one immutable,
dated checkpoint.

Autonomy does not reduce checkpoint requirements; checkpoints are what let
Hermes operate across tasks and phases without a human coordinator.

## When mandatory

Write a checkpoint before stopping after any state change, including:

- repository or task-status changes;
- durable source/CI/runtime discoveries;
- gate pass/failure/redefinition;
- blockers and handoffs;
- branch, PR, upstream, tested-engine or backend changes;
- durable L2 decisions;
- applied L3 decisions;
- autonomous task merges;
- autonomous phase transitions.

A chat summary, PR comment or model handoff alone is not a checkpoint.

## File naming and immutability

```text
checkpoints/YYYY-MM-DD-CP-NNN-short-description.md
```

Use monotonically increasing IDs. Never rewrite history to reinterpret state;
write a new checkpoint that supersedes the prior interpretation.

## Required headings

Every immutable checkpoint contains:

```markdown
## Objective
## Completed
## Verification
## Current state
## Risks and unresolved questions
## Next executable action
## Open first
## Safe reference point
```

## Required fields

Record:

1. checkpoint ID and timestamp;
2. repository, branch and PR;
3. task/phase ID;
4. objective;
5. commits/range covered;
6. exact completed work;
7. exact tests/workflows and outcomes;
8. current status;
9. changed requirements/decisions;
10. failures and risks;
11. one executable next action;
12. files to open first;
13. rollback/reference point;
14. buildable/mergeable state;
15. agent identifier when available;
16. L1-L4 classification;
17. senior reference and exact reviewed head for L2;
18. human decision ID, selected option/custom answer and interaction method for
    L3;
19. expected-head, merge method and merge SHA after merge;
20. phase-gate audit when a phase transition occurred.

Use `none` explicitly rather than omitting required fields.

## Decision authority record

Example:

```markdown
## Decision authority

- Level: `L2_SENIOR_REQUIRED`
- Decision: bounded SIGTERM -> timeout -> SIGKILL shutdown
- Senior consultation: `<reference>`
- Exact head reviewed: `<sha>`
- Human decision: `none`
- Evidence: `<tests/source>`
```

For L3 also record:

- decision ID;
- native Hermes selection form or chat fallback;
- options presented;
- senior recommendation;
- selected option or `Otra opción / Other` free text;
- time received;
- authorized scope/head.

Silence is never approval.

## Autonomous task merge record

Record task ID/PR, exact head, senior verdict, workflow runs, expected-head,
merge method, merge commit, ancestry verification, residual risks and next task.

## Autonomous phase-transition record

When a clear phase gate passes, record:

- every definition-of-done item;
- exact integrated head and commit range;
- full workflow/runtime evidence;
- unavailable evidence;
- traceability/control consistency;
- global senior phase verdict and exact reviewed head;
- blocking/non-blocking residual risks;
- next phase's approved bounded scope and first task;
- explicit audit that no L3/L4 trigger exists;
- expected-head and gate merge commit;
- manifest/roadmap/work-queue transition;
- post-transition checkpoint;
- first next-phase action started or ready.

No human decision field is required when no L3/L4 trigger exists. Record
`Human decision: none — senior phase gate authorized transition`.

If the phase transition contains L3, record the Hermes selection-form decision
instead and do not advance until answered.

## CURRENT.md

Keep CURRENT compact and include:

- checkpoint ID/link;
- current phase and task;
- state summary;
- verified CI;
- autonomy state (`running`, `human_decision_pending`, `safety_stopped`,
  `phase_gate_review`, `phase_transition_complete`);
- next action;
- top risks;
- minimal reading list.

## Quality rules

Do:

- give exact IDs, paths, commands, SHAs and workflow runs;
- distinguish specified, compiled, unit-tested, fake-engine, real-engine and
  hardware evidence;
- record failed hypotheses;
- identify one next action;
- state whether changes are committed;
- link decisions and requirements;
- record exact authority/review evidence.

Do not:

- paste large logs;
- duplicate architecture documents;
- use vague next actions;
- hide red required checks;
- depend on inaccessible chat context;
- infer human approval from silence;
- claim senior review for another head.

## Consistency checks

Before committing verify:

- manifest phase/task agree;
- work queue agrees;
- traceability reflects evidence;
- changelog records durable changes;
- roadmap is not advanced before its exact gate;
- PR describes the same gate and next task;
- autonomy classification matches policy;
- human/senior authority is represented honestly;
- autonomous merge/phase transition satisfies `AUTONOMOUS-MERGE.md`.

## Failed-session template

```markdown
# CP-NNN — short title

- Created: ISO-8601
- Branch/PR:
- Phase/task:
- Covered commits:
- Buildable: yes/no
- Decision level: L1/L2/L3/L4

## Objective
...

## Completed
...

## Verification
- command/workflow: PASS/FAIL

## Current state
...

## Failure or blocker
- assertion/reproduction:
- ruled out:
- hypothesis:
- senior reference:
- human decision/form, when applicable:

## Risks and unresolved questions
...

## Next executable action
...

## Open first
1. ...

## Safe reference point
...
```

## Long-running missions

Hermes writes a checkpoint at each task merge and phase transition even when the
same chat continues. Human-facing reports may remain sparse; repository
checkpoints remain complete.
