# Multi-agent working workflow

## Purpose

This document defines how humans, Codex sessions and Hermes profiles continue
work without relying on chat history or loading the entire repository into
context.

The durable project state lives in GitHub. Agent memory and conversation logs
are disposable caches.

## Five-minute recovery path

A new agent starts with this exact order:

1. read `/PROJECT-MANIFEST.json`;
2. read `checkpoints/CURRENT.md`;
3. read `WORK-QUEUE.md` and locate the active task ID;
4. inspect the active PR metadata and latest CI results;
5. read only the architecture/specification sections linked by that task;
6. inspect only the source and tests named by that task or changed by the PR.

Do not initially load every document, the complete PR diff or broad parts of
Seq66. Expand context only when evidence or dependencies require it.

## Context budget rules

- The manifest answers where the project is.
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
4. Claim one ready task in `WORK-QUEUE.md` by changing its status to
   `in_progress` and adding the agent/session identifier when practical.
5. Read the task's requirement IDs, dependencies, acceptance criteria and
   expected files.
6. State any assumption that is not already backed by source or test evidence.

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
8. update the active PR body with completed scope, CI evidence, known failures
   and the next task ID;
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
- branch, PR, upstream base or tested SooperLooper revision changes.

Do not create checkpoints for trivial read-only inspection that produced no new
finding and changed no project state.

## Task claiming and parallel agents

Parallel work is allowed only when tasks have disjoint expected files or an
explicit integration owner is named.

Before parallel work:

- split the work into separate task IDs;
- declare dependencies and expected files;
- identify which task owns shared documents or APIs;
- avoid two agents editing `PROJECT-MANIFEST.json`, `CURRENT.md`, the same
  protocol table or the same build file simultaneously.

The integration owner resolves shared-file updates and writes the final
checkpoint.

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

A task is not `done` when code exists but required tests or documentation do not.

## Failure handoff

A failed attempt is useful when the checkpoint contains:

- exact commit and task ID;
- command or workflow that failed;
- smallest relevant log excerpt or assertion;
- what was ruled out;
- current hypothesis;
- files already changed;
- safe rollback point;
- next diagnostic action.

Do not hide known failures by weakening tests or removing a required workflow.

## Review protocol

A reviewing agent reads, in order:

1. task acceptance criteria;
2. requirement IDs in traceability;
3. changed tests;
4. implementation diff;
5. changed architecture/specification text;
6. CI evidence;
7. current checkpoint.

The reviewer must distinguish:

- code present;
- code compiled;
- mock protocol verified;
- real engine verified;
- target hardware verified.

These are separate confidence levels.

## Human gates

Human instruction remains required for:

- merging the main integration PR;
- destructive migration of project formats;
- changing supported backend policy;
- adding a maintained SooperLooper fork or patch dependency;
- abandoning upstream compatibility;
- deleting user media or replacing prior valid project data.
