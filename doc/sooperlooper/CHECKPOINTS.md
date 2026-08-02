# Checkpoint protocol

## Purpose

Checkpoints are durable handoffs between agents and human sessions. They make
project continuation possible without replaying conversations or rereading the
entire repository.

`checkpoints/CURRENT.md` is the fast entry point. It points to one immutable,
dated checkpoint containing the detailed handoff.

## When a checkpoint is mandatory

A checkpoint MUST be written before an agent/session stops when it has:

- committed or modified repository state;
- changed a task status or roadmap assumption;
- discovered durable behaviour from source, CI or a real engine;
- reached, failed or redefined a gate;
- become blocked with work left for another agent;
- changed branch, PR, upstream base, tested engine revision or backend policy.

A chat message, model summary or PR comment alone is not a checkpoint.

## File naming

Immutable checkpoints use:

```text
checkpoints/YYYY-MM-DD-CP-NNN-short-description.md
```

IDs are monotonically increasing within the project:

```text
CP-001, CP-002, CP-003, ...
```

Never rewrite an immutable checkpoint except to fix a factual typo that does not
change the recorded state. Later corrections belong in a new checkpoint.

## Required checkpoint fields

Every immutable checkpoint contains:

1. checkpoint ID and creation timestamp;
2. repository, branch and active PR;
3. primary task ID and phase;
4. short objective of the session;
5. commits or range covered;
6. exact completed work;
7. exact tests/workflows and outcomes;
8. current component/status summary;
9. changed requirements and decisions;
10. known failures, risks and unresolved questions;
11. next executable action, not merely a broad phase name;
12. files another agent should open first;
13. safe rollback/reference point;
14. whether the branch is buildable and mergeable;
15. author/agent identifier when available.

Use `none` explicitly rather than omitting required fields.

## CURRENT.md format

`checkpoints/CURRENT.md` is intentionally compact. It contains:

- current checkpoint ID and link;
- current phase and active task;
- one-paragraph state summary;
- verified CI summary;
- next action;
- top risks;
- minimal reading list.

Keep it short enough to load in a small context window. Detailed history belongs
in immutable checkpoints and the changelog.

## Checkpoint quality rules

A good checkpoint is evidence-based and continuation-oriented.

Do:

- give exact task IDs, paths, commands and workflow names;
- distinguish implemented from compiled, mock-tested, real-engine-tested and
  target-hardware-tested;
- describe failed hypotheses and what was ruled out;
- identify the smallest next action;
- state whether pending changes are committed;
- link decisions and requirement IDs.

Do not:

- paste large logs;
- repeat entire architecture documents;
- say only “continue Phase 1”;
- claim success because an OSC packet was sent;
- hide a red required workflow;
- refer to inaccessible chat context as required knowledge.

## Checkpoint and control-file consistency

Before committing the checkpoint, verify:

- manifest `current_phase` and `active_task` agree with the checkpoint;
- work queue status agrees with the checkpoint;
- traceability reflects newly verified requirements;
- changelog contains durable externally meaningful changes;
- roadmap phase status is not advanced prematurely;
- active PR describes the same gate and next task.

CI validates structural consistency, but semantic consistency remains the
agent's responsibility.

## Failed-session checkpoint template

```markdown
# CP-NNN — short title

- Created: ISO-8601 timestamp
- Branch: ...
- PR: ...
- Phase/task: ...
- Covered commits: ...
- Branch buildable: yes/no

## Objective
...

## Completed
...

## Verification
- command/workflow: PASS/FAIL

## Failure or blocker
- failing assertion:
- reproduction:
- ruled out:
- current hypothesis:

## Changed contract/decisions
...

## Next executable action
...

## Open first
1. ...

## Safe reference point
...
```

## Milestone checkpoint

When a phase gate passes, the checkpoint additionally records:

- every definition-of-done item;
- workflow run evidence;
- traceability rows advanced to `verified`;
- deferred risks accepted for the next phase;
- manifest and roadmap transition.

The manifest and roadmap move to the next phase only in the same coherent change
as the passing milestone checkpoint.
