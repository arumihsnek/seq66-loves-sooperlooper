# Senior consultation contract

## Purpose

`codex-senior-consult` is the project's independent technical reviewer. It
replaces continuous external supervision at the moments where architecture,
risk or merge safety benefits from a second high-capability model.

The consultant does not own implementation and its answer is not proof. Hermes
owns the outcome and must verify all advice against repository contracts,
pinned source and executable evidence.

## Required consultation points

Consultation is mandatory before a durable decision involving:

- architecture or cross-component protocol;
- concurrency, threading, lock ordering or ownership;
- external process startup, shutdown, crash, restart or reconciliation;
- persistence, migration or backward compatibility;
- backend capability policy;
- security, path safety or data integrity;
- an ordinary functional merge gate;
- a milestone gate.

Consultation is also required after focused self-diagnosis when:

- more than one credible root cause remains;
- the proposed correction changes a contract;
- repeated attempts fail for materially different reasons;
- a test exposes a race, deadlock, lifecycle or recovery ambiguity.

## Consultation is not required for

- formatting and prose cleanup;
- obvious compiler diagnostics;
- routine test reruns;
- mechanical path/link updates;
- known shell quoting mistakes;
- straightforward missing link dependencies;
- changes already prescribed by an unchanged accepted review.

Avoid ceremonial calls. A consultation must answer a real review or decision
question.

## Minimum consultation packet

Provide the smallest sufficient packet:

1. task ID, phase and objective;
2. exact head SHA or exact diff range;
3. relevant architecture and product invariants;
4. acceptance criteria;
5. source paths and symbols under review;
6. exact tests and CI evidence;
7. credible options or current hypothesis;
8. one precise question;
9. requested verdict format.

Do not paste the entire repository or unrelated chat history.

## Planning consultation

Ask the consultant to:

- challenge assumptions;
- compare credible alternatives;
- identify hidden failure modes;
- define ownership and lifecycle;
- identify required negative tests;
- recommend the smallest complete implementation unit;
- separate Phase-current work from deferred scope.

Hermes records the durable decision and rationale in the appropriate canonical
document, not the full consultation transcript.

## Failure consultation

Before consulting, Hermes performs one bounded diagnosis and reports:

- exact failing command/run/job;
- smallest relevant log excerpt;
- what changed;
- what was ruled out;
- competing root causes;
- safe rollback point;
- proposed next diagnostic or correction.

The consultant must recommend how to discriminate between hypotheses, not merely
guess a cause.

## Merge-gate consultation

The merge request must identify the exact PR head and ask the consultant to
review:

- scope against the work-queue task;
- implementation and affected contracts;
- changed tests and quality of their oracles;
- concurrency/lifecycle implications;
- CI and real-engine evidence;
- documentation, traceability and checkpoint consistency;
- residual risks and deferred work.

Accepted merge verdicts are:

- `accept`;
- `accept_with_non_blocking_risks`.

Blocking verdicts include:

- `request_changes`;
- `block`;
- any answer with unresolved blocking findings even if its prose sounds
  positive.

A valid verdict must identify the exact head reviewed. A verdict for an earlier
head is stale after material code, test or contract changes.

## Applying advice

Hermes must:

1. verify the consultant inspected the intended evidence;
2. classify findings as blocking or non-blocking;
3. implement blocking corrections;
4. add regression tests where applicable;
5. re-run required validation;
6. re-consult after material changes;
7. record only durable conclusions and accepted risks.

Hermes may reject advice when pinned source or executable evidence disproves it,
but must record the contradiction and request a focused re-consultation before a
high-risk merge.

## Conflicting consultations

When two senior answers conflict:

1. compare exact evidence and head SHAs;
2. ask one focused reconciliation question containing both positions;
3. run a discriminating test when possible;
4. follow the position supported by stronger source and evidence.

If a material contradiction remains after this process, it becomes an L4 safety
stop and a bounded human decision.

## Milestone review

The global milestone consultation receives:

- milestone definition of done;
- integrated commit range;
- every required workflow and evidence level;
- traceability status;
- known unavailable evidence;
- accepted/deferred risks;
- proposed next milestone boundary.

It must return one of:

- `approve_milestone_gate`;
- `approve_with_non_blocking_risks`;
- `block_milestone_gate`.

The senior verdict informs the human gate; it does not replace the explicit
human decision to close one milestone and open the next.
