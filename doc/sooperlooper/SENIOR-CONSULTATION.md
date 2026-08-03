# Senior consultation contract

## Purpose

`codex-senior-consult` is the project's independent technical reviewer. It
replaces continuous external supervision at high-leverage architecture, failure,
merge and phase gates.

The consultant does not implement and its answer is not proof. Hermes owns the
outcome and verifies advice against repository contracts, pinned source and
executable evidence.

## Required consultation points

Consultation is mandatory for:

- architecture or cross-component protocol;
- concurrency, threading, lock ordering or ownership;
- process startup, shutdown, crash, restart or reconciliation;
- persistence, migration or backward compatibility;
- backend capability policy;
- security, path safety or data integrity;
- every functional merge gate;
- every phase gate.

It is also required after focused diagnosis when multiple credible root causes
remain, repeated remedies fail or a test exposes lifecycle/race ambiguity.

## Not required for

- formatting/prose cleanup;
- obvious compiler diagnostics;
- routine test reruns;
- mechanical links;
- known shell quoting mistakes;
- straightforward missing link dependencies;
- an unchanged action already prescribed by a current accepted review.

## Minimum packet

Provide:

1. task or phase ID and objective;
2. exact head SHA or diff range;
3. relevant architecture/product invariants;
4. acceptance criteria or definition of done;
5. source paths/symbols;
6. exact tests and CI evidence;
7. unavailable evidence and residual risks;
8. credible options or current hypothesis;
9. one precise question;
10. requested verdict format.

Do not send broad chat history or the entire repository.

## Planning review

Ask the senior to challenge assumptions, compare alternatives, define ownership
and lifecycle, identify hidden failure modes and negative tests, and recommend
the smallest complete unit.

## Failure review

Hermes first performs one bounded diagnosis and reports exact failing evidence,
what changed, what was ruled out, competing causes, rollback point and a proposed
discriminating test.

The senior should recommend how to distinguish hypotheses rather than merely
guess.

## Task merge gate

The senior reviews the exact PR head for:

- scope against the work-queue task;
- implementation and changed contracts;
- test oracle quality;
- concurrency/lifecycle implications;
- CI and real-engine evidence;
- traceability/checkpoint consistency;
- residual and deferred risks.

Accepted verdicts:

- `accept`;
- `accept_with_non_blocking_risks`.

Blocking verdicts:

- `request_changes`;
- `block`;
- any answer containing unresolved blockers.

A material change after review makes the verdict stale.

## Phase gate

The global phase consultation receives:

- phase definition of done;
- exact integrated head and commit range;
- every required workflow and evidence level;
- traceability/control status;
- unavailable evidence;
- blocking and non-blocking residual risks;
- the next phase's already-approved bounded scope and first task;
- an explicit L3/L4 trigger audit.

Valid phase verdicts:

- `accept`;
- `accept_with_non_blocking_risks`;
- `block`.

When the verdict accepts without blockers, evidence is green, the next phase is
already approved and bounded, and no L3/L4 trigger exists, the verdict is
operational: Hermes may merge the gate, close the current phase, open the next
phase and continue without a human message.

The senior must explicitly state whether any residual matter is genuinely L3 or
L4. A phase boundary is not itself a human decision.

When a genuine L3 matter exists, the senior explains why it belongs to the human
and recommends one option for the Hermes selection form.

## Applying advice

Hermes:

1. verifies the intended exact evidence was inspected;
2. classifies findings blocking/non-blocking;
3. implements blockers;
4. adds regression tests;
5. re-runs required validation;
6. re-consults after material changes;
7. records durable conclusions and accepted risks.

Hermes may reject advice disproved by pinned source or tests, but must record the
contradiction and request focused re-consultation before a high-risk gate.

## Conflicting consultations

When senior answers conflict:

1. compare evidence and head SHAs;
2. request one reconciliation review containing both positions;
3. run a discriminating test when possible;
4. follow the position supported by stronger evidence.

If a material contradiction remains, classify it L4 and use
`HUMAN-ESCALATION.md`.
