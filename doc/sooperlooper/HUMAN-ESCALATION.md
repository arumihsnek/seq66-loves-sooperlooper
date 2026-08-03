# Human escalation protocol

## Purpose

This contract ensures the human is asked only for decisions that genuinely
belong to product ownership or safety, and that each question is easy to answer
without becoming the project coordinator.

Hermes must not escalate ordinary engineering uncertainty before using source,
tests, focused diagnosis and `codex-senior-consult` where required.

## Valid escalation triggers

### Product and user experience

- changing the product objective or roadmap outcome;
- choosing subjective musical behaviour;
- choosing subjective visual/interaction behaviour;
- changing user-facing priorities when alternatives cannot all be delivered in
  the approved milestone.

### Compatibility, data and licensing

- intentional backward incompatibility;
- irreversible project-format migration;
- deletion or replacement of user media/data;
- a non-FOSS or license-incompatible dependency;
- abandoning an approved upstream compatibility constraint.

### Requirement and evidence limits

- two approved requirements conflict and no implementation can satisfy both;
- the only path to green tests would weaken an approved requirement;
- acceptance requires unavailable physical hardware;
- acceptance requires human hearing, performance or visual judgement;
- materially conflicting senior reviews remain after focused reconciliation.

### Governance

- closing a milestone and opening the next;
- force push or shared-history rewrite;
- an unrecoverable repository/control-plane contradiction;
- sensitive mutation with ambiguous authority or credentials.

## Invalid escalations

Do not ask the human:

- which class or file name to use;
- whether to use a mutex, queue or atomic before senior consultation and tests;
- how to repair an ordinary compiler or linker error;
- whether to rerun CI;
- whether to add a regression test;
- whether to create a branch, commit or PR;
- whether to merge an ordinary PR whose autonomous gate is complete;
- what to do next when `WORK-QUEUE.md` has a ready task;
- whether a red check may be ignored;
- to repeat information already present in repository evidence.

## One decision at a time

Each escalation contains exactly one decision. Do not combine product scope,
compatibility and schedule choices into one question.

Provide two to four real options. An option must be implementable and must state
its consequences. Do not use vague options such as “do it better” or “other.”

## Required question format

Use the template `templates/HUMAN-DECISION.md` and include:

- decision ID;
- one-sentence question;
- exact blocked task/branch/PR/head;
- why technical evidence and senior consultation cannot decide it;
- two to four options;
- senior recommendation and confidence;
- impact on compatibility, risk, scope and evidence;
- default behaviour while waiting;
- exact next action after each answer;
- expected answer format.

The normal expected answer is `A`, `B`, `C`, `approve`, `reject` or one short
sentence.

## Senior recommendation

A valid human question normally includes a senior recommendation. The senior
must receive the exact evidence and explain why the final choice still belongs
to the human.

No senior recommendation is required when consultation is impossible because of
an outage, but Hermes must state that limitation and provide its own evidence-
based recommendation.

## Behaviour while waiting

Default behaviour is:

- pause only the blocked branch;
- preserve all state and evidence;
- do not guess the human preference;
- continue independent safe tasks when ownership and dependencies are disjoint;
- avoid editing shared control files owned by the blocked task;
- update the checkpoint if the session ends while waiting.

A timeout never becomes implicit approval.

## Human answer handling

After receiving the answer Hermes:

1. records the decision ID and answer in the relevant PR/checkpoint;
2. updates `DECISIONS.md` when the decision is durable architecture or product
   policy;
3. verifies the authorized scope and exact head before mutation;
4. executes without asking for repetitive confirmation;
5. adds tests and compatibility evidence required by the chosen option;
6. resumes the autonomous task loop.

## Milestone gate question

The standard milestone question is:

> The milestone definition of done is satisfied on `<SHA>`, required workflows
> pass, senior verdict is `<verdict>`, and residual risks are listed below.
> Approve closing this milestone and opening `<next milestone>`, or request
> specific changes?

The packet includes exact runs, unavailable evidence, accepted risks and the
next milestone's bounded scope.

After `approve`, Hermes may mark ready, merge with expected-head protection,
write the post-merge checkpoint and begin the first ready task of the next
milestone.

## Safety-stop report

For L4 stops, Hermes does not offer an unsafe “continue anyway” option unless a
reviewed safe mechanism exists. It reports:

- observed fact;
- exact SHA and commands;
- potential damage;
- safe alternatives;
- senior recommendation;
- minimal human decision.
