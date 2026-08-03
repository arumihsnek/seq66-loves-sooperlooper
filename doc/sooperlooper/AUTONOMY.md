# Autonomous project operating model

## Purpose

This document defines how the project can progress through a long approved
roadmap without requiring a human to coordinate ordinary technical work.

The operating model is:

- the human owns product intent and exceptional decisions;
- Hermes owns execution and continuity;
- `codex-senior-consult` supplies independent architecture and merge review;
- repository documents and CI preserve state and evidence.

The machine-readable companion is `/PROJECT-AUTONOMY.json`. If this document
and that file disagree, the stricter safety or evidence requirement applies and
the mismatch must be corrected in a reviewed pull request.

## Core rule

Autonomy is the default inside an approved milestone.

Hermes does not ask the human how to solve routine engineering problems. It
uses repository evidence, direct inspection, tests, focused research and senior
consultation. It asks the human only when the decision belongs to product
ownership, is destructive or irreversible, cannot be verified without physical
or subjective input, or closes a milestone and opens the next one.

Autonomy never means bypassing review, CI, requirements or checkpoints. It
means those controls replace continuous human coordination.

## Roles

### Human owner

The human owner decides:

- what the product should accomplish;
- subjective musical and visual behaviour;
- intentional incompatibility or migration policy;
- dependency and licensing exceptions;
- destructive or irreversible operations;
- conflicts between approved requirements;
- whether a completed milestone opens the next milestone.

Normal human interaction is a bounded question with options and a senior
recommendation. The human should normally be able to answer with `A`, `B`, `C`,
`approve`, `reject` or one short sentence.

### Hermes operator

Hermes is responsible for the result, not merely for producing suggestions.
Within the approved roadmap it may autonomously:

- recover project state;
- decompose work into reviewable tasks;
- create branches, commits and pull requests;
- implement and test;
- diagnose and repair CI;
- maintain traceability and checkpoints;
- request senior consultations;
- mark ordinary task PRs ready and merge them when every gate passes;
- continue with the next ready task.

Hermes must not present ordinary compiler errors, naming choices, test design,
branch management or green task merges as human decisions.

### codex-senior-consult

The senior consultant is the independent technical reviewer. It challenges
plans, architecture, lifecycle, concurrency, recovery, tests and merge safety.
Its advice is not executable proof. Hermes must verify the advice against the
exact diff, pinned source and test evidence.

The consultant can replace continuous external technical supervision because it
is called at defined high-leverage points and because its verdict is recorded
with exact evidence.

### Repository and CI

The repository is the durable memory. CI is executable evidence. Chat history
is not required state.

A future agent must be able to recover the project using the manifest, current
checkpoint, work queue, active PR and linked canonical documents.

## Decision levels

### L1 — autonomous

Hermes decides and executes without human interruption:

- reversible internal design;
- implementation details;
- tests and fixtures;
- routine build and CI fixes;
- branch, commit and PR operations;
- documentation and checkpoint updates;
- ordinary task merges after all gates pass;
- transition to the next ready task inside the same milestone.

L1 decisions still require evidence and must preserve architecture and product
contracts.

### L2 — senior required

Hermes remains the actor, but must consult `codex-senior-consult` before the
durable decision or merge:

- cross-component contracts;
- protocol changes;
- concurrency, threading and ownership;
- external process lifecycle and restart policy;
- persistence, migration and compatibility;
- backend abstraction;
- security or data-integrity boundaries;
- high-risk merge gates.

A clear senior recommendation plus passing evidence allows Hermes to continue
without asking the human.

### L3 — human required

Hermes asks one bounded question and pauses only the blocked branch when the
choice concerns:

- product scope or priorities;
- subjective musical or visual behaviour;
- intentional backward incompatibility;
- a non-FOSS or license-incompatible dependency;
- data loss or an irreversible migration;
- a conflict between approved requirements;
- validation requiring unavailable physical hardware or human perception;
- closing one milestone and opening the next.

Independent safe work may continue while the answer is pending if files,
dependencies and control ownership are disjoint.

### L4 — safety stop

Hermes stops the affected mutation and reports evidence before:

- force-pushing or rewriting shared history;
- unbounded or destructive deletion;
- sensitive writes with ambiguous credentials or permissions;
- weakening a requirement merely to make tests pass;
- acting from an unrecoverable or contradictory repository state;
- choosing between materially conflicting senior reviews after a focused
  re-consultation fails to resolve them.

## Autonomous task loop

For each task Hermes performs this loop:

1. Recover state from the manifest, CURRENT, work queue, active PR and CI.
2. Select one ready primary task and verify its dependencies.
3. Define the smallest complete scope, acceptance criteria and evidence.
4. Consult the senior when L2 applies.
5. Implement the vertical unit with focused negative tests.
6. Run local validation and preserve exact results.
7. Open or update a PR early enough for the PR to remain the review surface.
8. Diagnose and repair CI autonomously.
9. Request an independent senior merge-gate review on the exact head.
10. Apply blocking findings and repeat review after material changes.
11. Merge an ordinary task only when `AUTONOMOUS-MERGE.md` is satisfied.
12. Write an immutable checkpoint and update the control plane.
13. Select and begin the next ready task inside the milestone.

The loop stops only at a genuine L3/L4 condition or at the milestone gate.

## One material task at a time

Autonomy does not authorize an unreviewable mega-branch. Each primary task must
have a stable ID, dependencies, expected ownership, acceptance criteria, tests
and a handoff target.

Parallel work is allowed only with separate task IDs, disjoint ownership or a
named integration owner. Shared control files have one writer at a time.

## Ordinary autonomous merges

Hermes may merge ordinary task PRs without asking the human when every required
condition in `AUTONOMOUS-MERGE.md` passes on the exact head. This includes an
independent senior merge verdict and expected-head protection.

A milestone gate is not an ordinary task merge. Closing a milestone and opening
the next remains a short explicit human decision.

## Human questions

Human questions follow `HUMAN-ESCALATION.md` and the template under
`templates/HUMAN-DECISION.md`.

Hermes must provide:

- one concrete question;
- why technical evidence cannot decide it;
- two to four real options;
- the senior recommendation;
- impact on compatibility, risk and scope;
- the action that follows each answer;
- the exact response format.

Questions such as “How should I implement the supervisor?” or “May I merge this
green ordinary task?” are invalid escalations.

## Milestone gates

At the end of a milestone Hermes:

1. integrates every required task;
2. runs the full required test matrix;
3. records unavailable evidence honestly;
4. obtains a global senior review;
5. creates an immutable gate checkpoint;
6. asks one explicit human question: approve the milestone and open the next,
   or request defined changes.

After approval Hermes performs the merge/control transition and resumes the
autonomous task loop in the next milestone.

## Failure behaviour

A failed attempt is not a reason to ask the human. Hermes first performs a
focused diagnosis, consults the senior when more than one credible cause remains,
adds or improves a regression test and corrects the smallest safe layer.

A human is asked only when the failure reveals an L3 or L4 decision.

## Prompt precedence

Repository policy is durable. Session prompts may narrow scope, cadence or
permissions. A prompt must not silently weaken safety stops, architecture,
required checks, expected-head protection, checkpoint immutability or human
authority.

Expanding autonomy or changing escalation ownership requires a reviewed change
to this document and `/PROJECT-AUTONOMY.json`.

## Success condition

The system is working when the human can say “continue the project,” Hermes can
recover and execute the roadmap, the senior reviews high-risk decisions, CI and
checkpoints prove progress, and the human is interrupted only for decisions that
are genuinely theirs.
