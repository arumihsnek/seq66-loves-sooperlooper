# Multi-agent working workflow

## Purpose

This document defines how humans, Codex sessions and Hermes profiles continue
work without relying on chat history or loading the entire repository into
context.

The durable project state lives in GitHub. Agent memory and conversation logs
are disposable caches.

The autonomous operating contract is defined by `/PROJECT-AUTONOMY.json` and
`AUTONOMY.md`. This workflow defines how that contract is executed with the
existing manifest, work queue, checkpoints, traceability and CI.

## Autonomous-by-default mode

Inside an approved milestone, Hermes is expected to continue through ordinary
technical tasks without repeated human permission.

Hermes owns implementation, tests, branches, PRs, CI repair, control-plane
maintenance, senior consultation and eligible ordinary merges. The human owns
product choices, destructive or irreversible changes, requirement conflicts,
subjective or unavailable physical validation and milestone close/open gates.

`codex-senior-consult` supplies independent architecture, failure and merge-gate
review. It does not replace source inspection or executable evidence.

Autonomy does not weaken architecture, required checks, checkpoint immutability,
expected-head protection or human safety authority.

## Decision classification

Before a material decision, classify it using `/PROJECT-AUTONOMY.json`:

- `L1_AUTONOMOUS`: Hermes decides and executes;
- `L2_SENIOR_REQUIRED`: Hermes consults senior, then decides and executes;
- `L3_HUMAN_REQUIRED`: Hermes asks one bounded human question;
- `L4_SAFETY_STOP`: Hermes stops the affected mutation and reports evidence.

Ordinary uncertainty is not L3. Compiler errors, naming, test design, branch
management and green ordinary task merges belong to L1 or L2.

Human questions follow `HUMAN-ESCALATION.md`. Autonomous merges follow
`AUTONOMOUS-MERGE.md`.

## Continuous task loop

The normal long-running loop is:

1. recover repository state;
2. select one ready primary task;
3. define acceptance criteria and evidence;
4. consult senior when L2 applies;
5. implement the smallest complete vertical unit;
6. run local validation;
7. open/update a PR;
8. repair CI autonomously;
9. obtain exact-head senior merge review;
10. merge when every autonomous gate passes;
11. write an immutable checkpoint and update control files;
12. continue with the next ready task inside the same milestone.

The loop stops only for an L3/L4 condition or a milestone gate. Full details are
in `MISSION-LIFECYCLE.md`.

## Five-minute recovery path

A new agent starts with this exact order:

1. read `/PROJECT-MANIFEST.json`;
2. read `/PROJECT-AUTONOMY.json` and `/AUTONOMY.md`;
3. read `checkpoints/CURRENT.md`;
4. read `WORK-QUEUE.md` and locate the active task ID;
5. inspect the active PR metadata and latest CI results;
6. read only the architecture/specification sections linked by that task;
7. inspect only the source and tests named by that task or changed by the PR.

Do not initially load every document, the complete PR diff or broad parts of
Seq66. Expand context only when evidence or dependencies require it.

## Context budget rules

- The manifest answers where the project is.
- The autonomy policy answers who may decide and merge.
- The current checkpoint answers what just happened and what remains risky.
- The work queue answers what to do next.
- The traceability matrix answers which requirement and test own the task.
- The roadmap answers phase ordering, not day-to-day task selection.
- The detailed specification is opened by section, not blindly in full.
- `TESTED-BEHAVIOUR.md` is read when assumptions about SooperLooper runtime are
  involved.

An agent must not reconstruct state from commit messages alone when the control
files above are available.

## Starting a work session

1. Verify repository, branch and active PR against the manifest.
2. Verify `CURRENT.md` points to an existing immutable checkpoint.
3. Confirm CI status for the current head.
4. Read and verify the autonomy policy.
5. Claim one ready task in `WORK-QUEUE.md` by changing its status to
   `in_progress` and adding the agent/session identifier when practical.
6. Read the task's requirement IDs, dependencies, acceptance criteria and
   expected files.
7. Classify material decisions L1-L4.
8. State any assumption that is not already backed by source or test evidence.

Only one task may be considered the session's primary task. Small prerequisite
fixes may be included, but unrelated cleanup is deferred.

## During implementation

- Work in the active feature branch or a child branch agreed in the checkpoint.
- Keep changes vertically coherent: implementation, focused tests and contract
  documentation move together.
- Record new durable runtime findings in `TESTED-BEHAVIOUR.md`.
- Record architecture/product decisions in `DECISIONS.md` before they become
  hidden implementation policy.
- Update traceability when a requirement becomes implemented or verified.
- Keep the work queue honest; blocked work is marked `blocked`, never left
  appearing active without explanation.
- Use bounded state-based waits, not arbitrary long sleeps.
- Use senior consultation at the points required by
  `SENIOR-CONSULTATION.md`.
- Do not wait for human permission for L1/L2 work.

## Senior consultation

Consultation packets and verdict handling follow
`SENIOR-CONSULTATION.md`.

The consultant must receive the exact task, head/diff, invariants, acceptance
criteria and evidence. A merge verdict becomes stale after material code, test
or contract changes.

Hermes remains responsible for verifying and applying advice.

## Pull requests and autonomous merge

Open a draft PR early enough for it to remain the review surface.

The PR template requires:

- autonomy level;
- evidence levels;
- exact senior review head and verdict;
- control-plane consistency;
- autonomous merge gate fields;
- risks and one next executable action.

Ordinary task PRs may be marked ready and merged by Hermes when every condition
in `AUTONOMOUS-MERGE.md` passes on the exact head.

Milestone close/open transitions remain a human decision.

## Mandatory handoff before stopping

Every agent session that changes repository state, changes the plan, discovers a
durable fact, or stops with incomplete work MUST leave a checkpoint before it
ends.

The exit sequence is:

1. run the narrowest relevant tests and record exact results;
2. update the active task status and next action in `WORK-QUEUE.md`;
3. update `TRACEABILITY.md` for changed requirement/test status;
4. append user-visible or architectural changes to `CHANGELOG-FORK.md`;
5. update `PROJECT-MANIFEST.json` if phase, active task, compatibility, tested
   revision, backend policy or required workflow changed;
6. create one immutable checkpoint under `checkpoints/`;
7. update `checkpoints/CURRENT.md` to point to it;
8. update the active PR body with completed scope, CI evidence, known failures,
   autonomy classification and the next task ID;
9. leave the branch buildable, or clearly record the exact failing gate and
   reproduction command.

A narrative chat handoff without these repository updates is not a valid
handoff.

## Checkpoint granularity

Create a checkpoint when any of these occurs:

- an agent/session is ending after repository changes;
- a task changes status;
- a phase gate is met or fails materially;
- a new tested runtime behaviour changes design assumptions;
- work is blocked and another agent must continue;
- branch, PR, upstream base or tested SooperLooper revision changes;
- a durable L2/L3 decision changes project behaviour or authority.

Do not create checkpoints for trivial read-only inspection that produced no new
finding and changed no project state.

## Task claiming and parallel agents

Parallel work is allowed only when tasks have disjoint expected files or an
explicit integration owner is named.

Before parallel work:

- split the work into separate task IDs;
- declare dependencies and expected files;
- identify which task owns shared documents or APIs;
- avoid two agents editing `PROJECT-MANIFEST.json`, `PROJECT-AUTONOMY.json`,
  `CURRENT.md`, the same protocol table or the same build file simultaneously.

The integration owner resolves shared-file updates and writes the final
checkpoint.

A blocked branch may wait for a human answer while independent safe work
continues on disjoint tasks.

## Status vocabulary

Tasks use only:

- `ready`;
- `in_progress`;
- `blocked`;
- `review`;
- `done`;
- `deferred`.

Requirements use only:

- `specified`;
- `partially_implemented`;
- `implemented`;
- `verified`;
- `blocked`;
- `deferred`.

A task is not `done` when code exists but required tests, senior review,
documentation or checkpoint do not.

## Failure handoff

A failed attempt is useful when the checkpoint contains:

- exact commit and task ID;
- command or workflow that failed;
- smallest relevant log excerpt or assertion;
- what was ruled out;
- current hypothesis;
- files already changed;
- safe rollback point;
- next diagnostic action;
- decision level and whether senior/human input is actually required.

Do not hide known failures by weakening tests or removing a required workflow.

Ordinary failures are diagnosed and corrected autonomously. Escalate only under
`HUMAN-ESCALATION.md`.

## Review protocol

A reviewing agent reads, in order:

1. task acceptance criteria;
2. autonomy classification;
3. requirement IDs in traceability;
4. changed tests;
5. implementation diff;
6. changed architecture/specification text;
7. CI evidence;
8. current checkpoint.

The reviewer must distinguish:

- code present;
- code compiled;
- mock protocol verified;
- real engine verified;
- target hardware verified.

These are separate confidence levels.

## Human gates

Human instruction is required only for the L3/L4 categories in
`HUMAN-ESCALATION.md`, including:

- closing a milestone and opening the next;
- destructive migration of project formats;
- intentional compatibility break;
- changing supported backend product policy;
- adding a maintained SooperLooper fork or incompatible dependency;
- abandoning upstream compatibility;
- deleting user media or replacing prior valid project data;
- unresolved requirement or senior-review conflicts;
- unavailable subjective/physical acceptance.

The human is not required for ordinary task merges whose autonomous gate passes.
