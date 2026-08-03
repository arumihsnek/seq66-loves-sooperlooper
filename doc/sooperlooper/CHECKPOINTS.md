# Checkpoint protocol

## Purpose

Checkpoints are durable handoffs between agents and human sessions. They make
project continuation possible without replaying conversations or rereading the
entire repository.

`checkpoints/CURRENT.md` is the fast entry point. It points to one immutable,
dated checkpoint containing the detailed handoff.

Autonomous execution does not reduce checkpoint requirements. Checkpoints are
what allow a long-running Hermes mission to continue across sessions without a
human coordinator.

## When a checkpoint is mandatory

A checkpoint MUST be written before an agent/session stops when it has:

- committed or modified repository state;
- changed a task status or roadmap assumption;
- discovered durable behaviour from source, CI or a real engine;
- reached, failed or redefined a gate;
- become blocked with work left for another agent;
- changed branch, PR, upstream base, tested engine revision or backend policy;
- accepted a durable L2 senior-reviewed decision;
- received and applied an L3 human decision;
- performed an autonomous merge or milestone transition.

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

## Required headings

Every immutable checkpoint contains exactly these canonical sections at minimum:

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

Additional sections may record decisions, failures or merge evidence, but must
not replace the required headings.

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
15. author/agent identifier when available;
16. autonomy decision level for material decisions;
17. senior consultation reference and exact reviewed head when L2 applies;
18. human decision ID and answer when L3 applies;
19. expected-head and merge SHA when an autonomous merge occurred.

Use `none` explicitly rather than omitting required fields.

## Autonomy decision record

For a material L1/L2/L3 decision, record a compact section such as:

```markdown
## Decision authority

- Level: `L2_SENIOR_REQUIRED`
- Decision: use bounded SIGTERM -> timeout -> SIGKILL supervisor shutdown
- Senior consultation: `<reference>`
- Exact head reviewed: `<sha>`
- Human decision: `none`
- Evidence: `<tests/source>`
```

Do not paste full senior transcripts. Record the durable decision, rationale,
accepted/rejected recommendations and evidence.

For L3 decisions record:

- decision ID;
- human answer;
- time received;
- authorized scope/head;
- document or PR where the decision is preserved.

A timeout or missing answer is never recorded as approval.

## Autonomous merge record

A checkpoint following an ordinary autonomous merge additionally records:

- task ID and PR;
- exact task head;
- senior verdict and reviewed head;
- required workflow runs;
- expected-head used;
- merge method;
- merge commit;
- ancestry verification;
- residual risks;
- next ready task.

## CURRENT.md format

`checkpoints/CURRENT.md` is intentionally compact. It contains:

- current checkpoint ID and link;
- current phase and active task;
- one-paragraph state summary;
- verified CI summary;
- autonomy state when relevant (`running`, `human_decision_pending`,
  `safety_stopped`, `milestone_gate`);
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
- link decisions and requirement IDs;
- state decision authority and review evidence;
- make it possible for a new session to resume at Stage 0 of
  `MISSION-LIFECYCLE.md`.

Do not:

- paste large logs;
- repeat entire architecture documents;
- say only “continue Phase 1”;
- claim success because an OSC packet was sent;
- hide a red required workflow;
- refer to inaccessible chat context as required knowledge;
- claim human approval from silence;
- claim senior review for a head the consultant did not inspect.

## Checkpoint and control-file consistency

Before committing the checkpoint, verify:

- manifest `current_phase` and `active_task` agree with the checkpoint;
- work queue status agrees with the checkpoint;
- traceability reflects newly verified requirements;
- changelog contains durable externally meaningful changes;
- roadmap phase status is not advanced prematurely;
- active PR describes the same gate and next task;
- autonomy classification agrees with `PROJECT-AUTONOMY.json`;
- human/senior authority is not misrepresented;
- an autonomous merge satisfies `AUTONOMOUS-MERGE.md`.

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
- failing assertion:
- reproduction:
- ruled out:
- current hypothesis:
- senior reference:
- human decision ID, when applicable:

## Risks and unresolved questions
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
- global senior milestone verdict;
- explicit human milestone decision;
- manifest and roadmap transition;
- exact gate head and post-merge commit.

The manifest and roadmap move to the next phase only in the same coherent change
as the passing milestone checkpoint and after the explicit human gate.

## Long-running mission checkpoints

Hermes may integrate multiple ordinary tasks in one long-running mission. It
writes a checkpoint at each task transition/merge even when the chat session
continues. This keeps repository state recoverable if the process stops
unexpectedly.

Human-facing reports remain sparse and follow
`templates/AUTONOMOUS-REPORT.md`; repository checkpoints remain complete.
