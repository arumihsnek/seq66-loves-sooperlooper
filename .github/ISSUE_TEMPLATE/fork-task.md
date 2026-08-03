---
name: Fork task proposal
about: Propose a traceable Seq66 Loves SooperLooper task
labels: ''
assignees: ''
---

## Proposed task

Suggested task ID: <!-- leave blank when proposing a new queue item -->
Roadmap phase:
Requirement IDs:

## Problem and user value

Describe the observable problem or capability. Do not start from an
implementation preference.

## Preconditions and dependencies

- Dependencies:
- Existing decisions/specification sections:
- Upstream Seq66/SooperLooper evidence:

## Proposed ownership

Expected files/components:

Potential shared-file conflicts:

Integration owner when shared files are unavoidable:

## Autonomy classification

Expected highest decision level:

- [ ] `L1_AUTONOMOUS`
- [ ] `L2_SENIOR_REQUIRED`
- [ ] `L3_HUMAN_REQUIRED`
- [ ] `L4_SAFETY_STOP`

Explain the classification. Do not classify ordinary implementation uncertainty
as a human decision.

Required senior consultation points:

Potential human decision ID/question, when genuinely required:

## Acceptance criteria

- [ ] criterion 1
- [ ] criterion 2
- [ ] failure/negative criterion
- [ ] no silent relaxation of existing requirement

## Required evidence

- [ ] compile/unit
- [ ] fake-engine protocol
- [ ] pinned real engine
- [ ] native JACK
- [ ] PipeWire-JACK
- [ ] Raspberry Pi/target hardware
- [ ] documentation/traceability/checkpoint
- [ ] project-control and autonomy-policy validators

State unavailable evidence explicitly and identify the task or gate where it
will be produced.

## Architecture and backend impact

State whether this changes authority, process boundaries, protocol, backend
support, persistence, threading or `backend_unavailable` semantics.

## Merge and handoff

- Eligible for ordinary autonomous merge: yes/no
- Conditions that would make it a milestone/human gate:
- Expected next task/handoff:

## Decision needed

List only decisions that cannot be resolved from repository evidence, tests and
required senior consultation. Use `doc/sooperlooper/HUMAN-ESCALATION.md`.

---

This issue does not become authorized work until it receives a stable ID in
`doc/sooperlooper/WORK-QUEUE.md`, dependencies, requirement links and acceptance
criteria. Upstream TODO text alone is not authorization.
