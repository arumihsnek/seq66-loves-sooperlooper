# Human escalation protocol

## Purpose

This contract ensures the human is asked only for decisions that genuinely
belong to product ownership or safety, and that each interaction is as easy as
possible.

Hermes must not escalate ordinary engineering uncertainty or a clear phase
transition before using repository evidence, tests, CI and
`codex-senior-consult`.

## Valid L3 escalation triggers

### Product and experience

- changing the approved product objective or roadmap outcome;
- choosing subjective musical behaviour;
- choosing subjective visual or interaction behaviour;
- changing user-facing priorities when approved goals conflict.

### Compatibility, data and licensing

- intentional backward incompatibility;
- irreversible project-format migration;
- deletion or replacement of user media/data;
- a non-FOSS or license-incompatible dependency;
- abandoning an approved upstream compatibility constraint.

### Requirement and evidence limits

- two approved requirements conflict and cannot both be satisfied;
- green tests would require weakening an approved requirement;
- acceptance requires unavailable physical hardware;
- acceptance requires human hearing, performance or visual judgement;
- materially conflicting senior reviews remain after focused reconciliation.

### Phase transitions

A phase transition is L3 only when it introduces one of the triggers above, when
the next phase is not already bounded in the approved roadmap, or when its
acceptance requires a human subjective decision.

A completed, fully verified phase with a preapproved next phase is L2 and uses a
senior gate without interrupting the human.

## L4 safety stops

- force push or shared-history rewrite;
- unbounded or destructive deletion;
- unrecoverable repository/control-plane contradiction;
- sensitive mutation with ambiguous authority or credentials;
- unresolved materially conflicting senior reviews.

## Invalid escalations

Do not ask the human:

- which class or file name to use;
- whether to use a mutex, queue or atomic;
- how to repair a compiler, linker or CI error;
- whether to rerun tests;
- whether to create a branch, commit or PR;
- whether to merge an ordinary PR whose gate is complete;
- whether to close a clearly complete phase whose senior phase gate passes and
  whose next phase is already approved and bounded;
- what to do next when `WORK-QUEUE.md` has a ready task;
- to repeat information present in repository evidence.

## One decision at a time

Each escalation contains exactly one decision. Provide two to four concrete,
implementable options with consequences.

An `Otra opción / Other` choice is encouraged when a custom answer is safe. It
must open or request free-text input and must not be used to bypass safety,
licensing, compatibility or destructive-action constraints.

## Native Hermes selection forms

When the Hermes interface supports native interactive selection forms, Hermes
MUST use them for human decisions whenever the decision can be represented
safely.

The preferred interaction is:

1. concise title and decision ID;
2. one-sentence question;
3. two to four selectable options;
4. the recommended option visibly marked;
5. short impact text for each option;
6. `Otra opción / Other` when safe;
7. a free-text field or immediate free-text follow-up for `Otra opción / Other`;
8. one submit action.

For a binary decision use a two-choice form, not an open-ended chat question.
For a multiple-choice product decision use radio/select controls. Do not require
the human to copy an option label manually when Hermes can render it.

Plain chat is permitted only when:

- the form capability is unavailable;
- the decision cannot be represented safely by selection controls;
- the user explicitly requests plain text;
- an L4 report must present evidence before any selectable action.

When falling back to chat, preserve the same bounded structure and allow answers
such as `A`, `B`, `C` or one short custom sentence.

## Required decision packet

Every human interaction includes:

- decision ID;
- one concrete question;
- blocked task/branch/PR/head when applicable;
- why technical evidence and senior consultation cannot decide it;
- two to four options;
- senior recommendation and confidence;
- impact on compatibility, risk, scope and evidence;
- default behaviour while waiting;
- exact next action after the selected answer.

Use `templates/HUMAN-DECISION.md`.

## Senior recommendation

A valid L3 question normally includes a senior recommendation. The senior must
receive the exact evidence and explain why the final choice belongs to the
human.

If consultation is unavailable, Hermes states that limitation and gives its own
evidence-based recommendation. It must not pretend a senior verdict exists.

## Behaviour while waiting

Hermes:

- pauses only the blocked branch;
- preserves state and evidence;
- does not infer approval from silence;
- continues independent safe tasks with disjoint ownership;
- avoids shared control files owned by the blocked task;
- updates the checkpoint if the session ends.

A timeout never becomes implicit approval.

## Answer handling

After a selection Hermes:

1. records the decision ID and selected/custom answer;
2. updates `DECISIONS.md` when durable policy changed;
3. verifies scope and exact head before mutation;
4. executes without repetitive confirmation;
5. adds required tests and compatibility evidence;
6. resumes the autonomous loop.

## Phase-gate handling

A phase gate produces no human question when:

- definition of done is satisfied;
- full required checks pass on the exact integrated head;
- senior global verdict is `accept` or
  `accept_with_non_blocking_risks` without blockers;
- residual risks are recorded and non-blocking;
- the next phase is already approved, bounded and has a first real task;
- no L3/L4 trigger exists.

Hermes then closes the phase, opens the next, writes the post-transition
checkpoint and continues.

When a phase gate contains a genuine L3 choice, Hermes uses the native selection
form described above rather than asking a vague approve/reject question.

## Safety-stop report

For L4, Hermes does not offer an unsafe `continue anyway` option unless a
reviewed safe mechanism exists. It reports:

- observed fact;
- exact SHA and commands;
- potential damage;
- safe alternatives;
- senior recommendation;
- minimal human decision, rendered as a selection form when safe.
