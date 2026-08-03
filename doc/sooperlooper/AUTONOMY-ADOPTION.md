# Autonomous governance adoption

## Purpose

This document describes safe activation without interrupting implementation work
already in progress.

## Review sequence

1. Target the current `fork-main` and incorporate its latest integrated state.
2. Run project-control and autonomy validators on the exact head.
3. Review L1-L4 role boundaries.
4. Review task and phase autonomous merge gates.
5. Review native Hermes selection-form interaction and chat fallback.
6. Review senior exact-head task and phase requirements.
7. Confirm no live task, phase or CURRENT state was changed by adoption.
8. Apply blocking findings and repeat CI after material changes.
9. Merge as one documentation/governance unit with expected-head protection.

## Activation

The policy becomes active only after its PR is merged into `fork-main`.

An in-progress task may finish under its previous authorization until a safe task
or checkpoint boundary. At the next boundary Hermes records that the repository
uses `seq66-sl-autonomy-v2` and follows the continuous task and phase loops.

A pre-existing implementation PR is not invalidated solely because it was opened
before adoption, but its merge must satisfy the policy active at merge time.

## First autonomous mission

The normal instruction is:

```text
Continue the project under the repository autonomy policy.
```

Hermes recovers canonical state and continues across tasks and clear phases until:

- a genuine L3 human selection is required;
- an L4 safety stop occurs;
- the approved roadmap is exhausted or a major release/product gate needs a new
  scope decision;
- the operator chooses to provide a sparse progress report without pausing work.

A phase boundary is not itself a stopping condition.

## Human interaction

When L3/L4 applies and Hermes supports native controls, use a single-choice form
with two to four options, senior recommendation and `Otra opción / Other` with
free-text input when safe. Plain chat is fallback only when the form is
unavailable or cannot represent the decision safely.

## Rollback

Do not silently ignore individual rules. Create a governance task and reviewed
PR that updates `PROJECT-AUTONOMY.json`, human-readable contracts, validator and
templates together.

Previously merged implementation remains intact; policy rollback changes future
coordination authority, not product data or Git history.
