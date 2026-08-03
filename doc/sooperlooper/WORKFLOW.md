# Multi-agent working workflow

## Purpose

This document defines how Hermes, Codex sessions and humans continue work
without relying on chat history.

Durable state lives in GitHub. `/PROJECT-AUTONOMY.json` and `/AUTONOMY.md`
define authority; this workflow applies them to the manifest, queue,
checkpoints, traceability and CI.

## Autonomous-by-default mode

Hermes is expected to progress through the approved roadmap without repeated
human permission.

Hermes owns implementation, tests, branches, PRs, CI repair, control-plane
maintenance, senior consultation, eligible task merges and clear phase
transitions.

The human owns only genuine L3/L4 decisions: product scope, subjective musical
or visual behaviour, intentional incompatibility, licensing, data loss,
irreversible migration, unresolved requirement conflict, unavailable physical
or subjective acceptance and safety stops.

A phase boundary is not automatically a human gate.

## Decision classification

- `L1_AUTONOMOUS`: Hermes decides and executes;
- `L2_SENIOR_REQUIRED`: Hermes consults senior, then decides and executes;
- `L3_HUMAN_REQUIRED`: one bounded human selection form;
- `L4_SAFETY_STOP`: stop the affected mutation and report evidence.

Human questions follow `HUMAN-ESCALATION.md`. Merges and phase transitions
follow `AUTONOMOUS-MERGE.md`.

## Continuous task loop

1. recover repository state;
2. select one ready task;
3. define acceptance criteria and evidence;
4. consult senior when L2 applies;
5. implement the smallest complete vertical unit;
6. run local validation;
7. open/update a PR;
8. repair CI autonomously;
9. obtain exact-head senior merge review;
10. merge with `expected-head` protection when all gates pass;
11. write an immutable checkpoint and update control files;
12. continue with the next ready task.

The loop stops only for a genuine L3/L4 condition.

## Phase loop

When a phase is complete Hermes:

1. verifies definition of done item by item;
2. runs the full required matrix on the exact integrated head;
3. records unavailable evidence;
4. audits traceability and project control;
5. obtains global exact-head senior phase review;
6. records blocking/non-blocking risks;
7. verifies the next phase is already approved, bounded and has a first task;
8. confirms no L3/L4 trigger exists;
9. publishes the phase gate checkpoint and PR;
10. merges with `expected-head` protection;
11. writes the post-transition checkpoint;
12. opens the next phase and continues.

No human message is required when all conditions are true.

If the transition changes approved scope or hits L3/L4, Hermes uses the native
selection-form protocol in `HUMAN-ESCALATION.md`.

## Five-minute recovery path

1. read `/PROJECT-MANIFEST.json`;
2. read `/PROJECT-AUTONOMY.json` and `/AUTONOMY.md`;
3. read `checkpoints/CURRENT.md`;
4. locate the active `WORK-QUEUE.md` task;
5. inspect active PR and exact-head CI;
6. read only linked specification/architecture sections;
7. inspect only owned source and tests.

Do not reconstruct state from chat or commit messages alone.

## Context rules

- manifest: live repository and task state;
- autonomy policy: authority and gates;
- CURRENT/checkpoint: latest handoff and risks;
- work queue: next executable work;
- traceability: requirement/evidence ownership;
- roadmap: phase ordering and approved boundaries;
- PR/CI: review surface and executable proof.

## Starting a session

1. verify repository, branch and PR base;
2. verify CURRENT and checkpoint;
3. verify current exact-head CI;
4. read autonomy policy;
5. claim one ready task;
6. read dependencies, criteria and expected files;
7. classify material decisions L1-L4;
8. state unsupported assumptions.

## During implementation

- keep implementation, tests and contract docs together;
- record durable runtime findings;
- record architecture/product decisions;
- keep queue and traceability honest;
- use bounded state-based waits;
- use senior consultation at required points;
- do not wait for human permission for L1/L2 work.

## Senior consultation

Packets and verdicts follow `SENIOR-CONSULTATION.md`. The senior receives exact
head/diff, invariants, criteria and evidence. A material change makes the verdict
stale. Hermes remains responsible for verification.

## Pull requests and merge

Open draft PRs early. The PR records:

- autonomy level;
- evidence levels;
- exact senior review head/verdict;
- control-plane consistency;
- task or phase gate fields;
- risks and one next action.

Hermes may mark ready and merge any eligible task or clear phase gate when every
condition in `AUTONOMOUS-MERGE.md` passes.

## Human interaction

When L3/L4 applies and Hermes supports interactive forms, Hermes MUST use a
native selection form with two to four concrete choices, senior recommendation
and `Otra opción / Other` with free-text input when safe.

Plain chat is fallback only when the form is unavailable or unsafe for the
decision.

## Mandatory handoff

Every state-changing session:

1. records exact tests/results;
2. updates queue status and next action;
3. updates traceability;
4. updates changelog/decisions where durable;
5. updates manifest when live state changes;
6. writes one immutable checkpoint;
7. updates CURRENT;
8. updates the PR with exact evidence, risks, autonomy class and next task;
9. leaves the branch buildable or records the exact red gate.

## Parallel work

Parallel tasks require separate IDs, dependencies, disjoint ownership or one
integration owner. Shared control files have one writer at a time.

A branch waiting for L3 may pause while disjoint safe work continues.

## Failure handoff

Record exact commit/task, failing command, relevant diagnostics, ruled-out
causes, hypothesis, changed files, rollback point, decision level and next
diagnostic action.

Ordinary failures are diagnosed and corrected autonomously. Never weaken tests
or required workflows to hide failure.

## Review protocol

Reviewers inspect task/phase criteria, autonomy classification, traceability,
tests, implementation, contracts, CI and checkpoint. They distinguish code
present, compiled, unit tested, fake-engine tested, real-engine tested and target
hardware tested.

## Human gates

Human instruction is required only for genuine L3/L4 categories. A completed
phase with a senior-accepted exact-head gate and a preapproved bounded next phase
is not a human gate.
