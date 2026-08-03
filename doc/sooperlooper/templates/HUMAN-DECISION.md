# Human decision — `<DECISION-ID>`

## Hermes selection form

Use the native Hermes interactive selection form whenever supported and safe.
Do not send this as an open-ended chat question unless forms are unavailable or
cannot represent the decision safely.

- Form title: `<short decision title>`
- Question: `<one concrete question>`
- Selection type: `single choice`
- Recommended option visibly marked: `<A/B/C>`
- Include `Otra opción / Other`: `yes` unless a custom answer would be unsafe
- `Otra opción / Other` behaviour: open or request free-text input
- Submit action: one confirmation action

## Exact blocked state

- Phase:
- Task:
- Branch:
- PR:
- Head SHA:
- Last passing evidence:

## Why technical resolution is insufficient

State the fact that makes this L3. A phase boundary, implementation uncertainty,
lack of time or ordinary technical disagreement is not sufficient.

## Options

### A — `<short name>`

- Form label:
- Behaviour:
- Compatibility:
- Risk:
- Scope/cost:
- Evidence required:
- Action after selection:

### B — `<short name>`

- Form label:
- Behaviour:
- Compatibility:
- Risk:
- Scope/cost:
- Evidence required:
- Action after selection:

<!-- Optional C/D; keep two to four concrete options. -->

### Otra opción / Other

Include when a safe custom answer is meaningful.

- Free-text prompt: `<what the user should specify>`
- Validation/constraints:
- Unsafe custom responses that cannot be accepted:

## Senior recommendation

- Recommended option:
- Confidence:
- Reasoning:
- Why the final choice still belongs to the human:
- Consultation execution/reference:

## Default while waiting

Pause only the blocked branch. Preserve state. Continue independent safe work
when ownership and dependencies are disjoint. Silence is not approval.

## Next action after the answer

Describe the exact bounded action Hermes will execute without asking for
repetitive confirmation.

## Chat fallback

Use only when the Hermes selection form is unavailable or cannot represent the
question safely. Preserve the same options and accept `A`, `B`, `C` or one short
custom sentence.

## Decision record

Complete after selection:

- Selected option/custom answer:
- Received at:
- Form or fallback used:
- Recorded in checkpoint/decision log:
- Authorized head/scope:
