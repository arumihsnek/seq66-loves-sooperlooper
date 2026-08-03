# Autonomous merge and phase-transition policy

## Purpose

This policy allows Hermes to merge ordinary tasks and clear phase transitions
without asking the human each time, while preserving independent review,
exact-head safety and reproducible evidence.

Autonomy never authorizes destructive migration, hidden product-scope change,
incompatible licensing, force push or red-check merges.

## Eligible ordinary pull requests

A task PR is eligible when:

- it maps to one task in `WORK-QUEUE.md` or a declared integration unit;
- dependencies are satisfied;
- scope and changed files match ownership;
- unrelated cleanup is absent or separated;
- it targets `fork-main`;
- architecture and product invariants are preserved;
- no L3 or L4 decision is embedded.

## Eligible phase-gate pull requests

A phase gate is eligible for autonomous transition when:

- every required phase task is integrated;
- the definition of done is satisfied item by item;
- required CI and runtime evidence pass on the exact integrated head;
- unavailable evidence is explicit and senior-classified;
- traceability and project control are consistent;
- the next phase is already defined and bounded in the approved roadmap;
- the next phase has a real first task;
- global `codex-senior-consult` review accepts without blockers;
- residual risks are recorded and non-blocking;
- no L3 or L4 trigger exists.

A phase boundary by itself is not a human gate.

## Mandatory exact-head conditions

Every merge condition must be true on the head being merged:

1. The task or phase gate exists in the control plane.
2. Acceptance criteria or definition of done are explicitly addressed.
3. Focused positive and negative tests exist where behaviour changes.
4. Test assertions observe named properties rather than only `no crash`.
5. Every required workflow is `success` on the exact head.
6. Project-control and autonomy-policy validators pass.
7. Traceability reflects changed requirements and evidence.
8. An immutable checkpoint records the merge-ready state.
9. CURRENT, manifest, work queue and PR body are semantically consistent.
10. `git diff --check` is clean.
11. No unresolved review thread remains.
12. `codex-senior-consult` reviewed the exact head and returned `accept` or
    `accept_with_non_blocking_risks` with no blocker.
13. The PR is mergeable and targets the intended base.
14. Expected-head protection is used.
15. Merge commit is used unless a reviewed versioned policy requires another
    method.
16. No unresolved L3 or L4 trigger is crossed.

A material code, test, contract or control change after review invalidates the
verdict and requires fresh CI and senior review.

## Evidence by change type

### Documentation/control

- structural validators;
- semantic consistency review;
- exact changed paths;
- no accidental product-scope transition;
- senior review for policy or phase changes.

### Pure implementation

- warnings-as-errors compile;
- focused unit and regression tests;
- negative/failure tests;
- sanitizer evidence when practical.

### OSC/engine integration

- unit/fake-engine contract tests;
- exact pinned-source path/signature evidence;
- real-engine evidence when behaviour reaches the engine;
- generation, timeout and stale-feedback cases.

### Backend/process

- startup and shutdown outcomes;
- bounded timeout and escalation;
- crash/restart cases;
- unavailable backend cases;
- native JACK and PipeWire-JACK evidence separately;
- enabled and disabled build configurations where relevant.

### Persistence/migration

- rollback and atomicity tests;
- prior valid project preservation;
- compatibility matrix;
- L3 human selection for destructive, irreversible or intentionally
  incompatible behaviour.

## Merge sequence

Hermes:

1. fetches and verifies base and head;
2. captures `EXPECTED_HEAD`;
3. verifies required checks for that SHA;
4. verifies the senior verdict references that SHA;
5. verifies zero unresolved threads;
6. marks ready when still draft;
7. re-reads the PR head;
8. merges using expected-head protection;
9. records merge SHA and time;
10. fast-forwards local `fork-main` and proves ancestry;
11. writes the post-merge or post-phase checkpoint;
12. updates control state;
13. continues to the next task or preapproved phase.

A failed precondition is corrected, not bypassed.

## Forbidden autonomous merges

Hermes must not merge when:

- a required check is red, missing or attached to another head;
- senior review contains a blocker;
- the head changed after verification;
- a requirement or test is silently relaxed;
- review threads remain unresolved;
- mergeability is unknown/conflicting;
- shared history would be rewritten;
- user data may be deleted or irreversibly migrated without L3 authority;
- a non-FOSS/incompatible dependency is introduced without L3 authority;
- physical or subjective acceptance remains unresolved;
- a phase transition changes approved scope or otherwise hits L3/L4.

## Non-blocking risks

A risk may be accepted autonomously only when:

- senior explicitly classifies it as non-blocking;
- current acceptance criteria remain satisfied;
- it is recorded in the PR and checkpoint;
- it receives a task ID when follow-up is required;
- deferral does not falsely advance a requirement to `verified`.

## Phase-transition result

For a clear phase gate, Hermes may autonomously:

- close the completed phase;
- merge the gate PR;
- publish the post-transition checkpoint;
- open the next approved phase;
- select its first ready task;
- continue execution.

When an L3 choice exists, Hermes uses the native selection-form protocol in
`HUMAN-ESCALATION.md`.
