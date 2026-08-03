# Autonomous merge policy

## Purpose

This policy allows Hermes to merge ordinary task pull requests without asking
the human each time, while preserving independent review, exact-head safety and
reproducible evidence.

Autonomous merge applies only inside an already approved milestone. It does not
close a milestone, open the next milestone, authorize destructive migration or
change product scope.

## Eligible pull requests

A pull request is eligible only when:

- it maps to one task in `WORK-QUEUE.md`, or to a declared integration unit;
- its dependencies are satisfied;
- its scope and changed files match the task ownership;
- unrelated cleanup is absent or split into another task;
- it targets `fork-main`, not `master`;
- it preserves repository architecture and product invariants;
- no L3 or L4 decision is embedded in the change.

Documentation-only control PRs are eligible when they do not change milestone
ownership or bypass a human gate.

## Mandatory merge conditions

Every condition must be true on the exact head being merged:

1. The task exists and has status `review`.
2. Acceptance criteria are explicitly addressed in the PR.
3. Focused positive and negative tests exist where behaviour changes.
4. Test assertions observe the named property rather than only “no crash.”
5. Every required workflow is `success` on the exact head.
6. Project-control and autonomy-policy validators pass.
7. Traceability reflects changed requirements and evidence.
8. A new immutable checkpoint records the task result or merge-ready handoff.
9. `CURRENT.md`, manifest, work queue and PR body are semantically consistent.
10. `git diff --check` is clean.
11. No unresolved review thread remains.
12. `codex-senior-consult` reviewed the exact head and returned `accept` or
    `accept_with_non_blocking_risks` with no blocking finding.
13. The PR is mergeable.
14. The base is the intended integration branch.
15. Expected-head protection is used for the merge.
16. Merge commit is used unless a reviewed versioned policy explicitly requires
    another method.

If a material code, test, contract or control change occurs after senior review,
that review is stale and must be repeated.

## Required evidence by task type

### Documentation/control task

- structural validator;
- semantic consistency review;
- exact changed paths;
- no accidental product or phase transition;
- senior review when policy or milestone scope changes.

### Pure implementation task

- warnings-as-errors compile;
- focused unit tests;
- regression tests;
- negative/failure tests;
- sanitizer evidence when practical and relevant.

### OSC or engine integration task

- unit/fake-engine contract tests;
- exact paths and signatures verified against pinned source;
- pinned real-engine evidence when the behaviour can reach the engine;
- generation, timeout and stale-feedback cases.

### Backend/process task

- startup and shutdown outcomes;
- bounded timeout and escalation;
- crash/restart cases;
- unavailable backend cases;
- native JACK and PipeWire-JACK evidence named separately;
- enabled and disabled build configurations where relevant.

### Persistence/migration task

- rollback/atomicity tests;
- prior valid project preservation;
- compatibility matrix;
- explicit human gate for destructive or irreversible behaviour.

## Merge sequence

Hermes performs:

1. fetch and verify base and head;
2. capture `EXPECTED_HEAD`;
3. inspect required checks for that exact SHA;
4. verify senior verdict references that SHA;
5. verify unresolved review threads count is zero;
6. mark ready when still draft;
7. re-read PR head immediately;
8. merge using expected-head protection;
9. record merge SHA and time;
10. fetch and fast-forward local `fork-main`;
11. prove the task head is an ancestor of `fork-main`;
12. write/update the post-merge checkpoint and control state;
13. continue to the next ready task.

A failed merge precondition is diagnosed and corrected; it is not bypassed.

## Forbidden autonomous merges

Hermes must not autonomously merge when:

- a required check is red, missing or attached to a different head;
- the senior review contains a blocker;
- the head changed after verification;
- the PR silently relaxes a requirement or test;
- unresolved review threads remain;
- mergeability is unknown or conflicting;
- the change rewrites shared history;
- the change deletes user data or performs irreversible migration;
- the change introduces a non-FOSS or incompatible dependency;
- the change closes a milestone or opens the next one;
- physical or subjective acceptance remains unresolved.

These become a correction, L3 question or L4 stop according to
`HUMAN-ESCALATION.md`.

## Non-blocking risk handling

Non-blocking risks may be accepted autonomously only when:

- the senior explicitly classifies them as non-blocking;
- they do not violate current acceptance criteria;
- they are recorded in the PR and checkpoint;
- they receive a task ID when follow-up is required;
- deferral does not falsely advance a requirement to `verified`.

## Milestone exception

A milestone integration PR may be technically prepared and reviewed
autonomously, but the final close/open transition requires an explicit human
answer. After that answer Hermes may perform the authorized merge and control
updates without another permission round.
