## Task or phase gate and scope

Task/phase ID: <!-- e.g. M2-003 or Phase 2 gate -->
Requirement IDs:
Target branch: `fork-main`

Describe the smallest coherent unit and explicitly list out-of-scope work.

## Autonomy classification

Highest decision level:

- [ ] `L1_AUTONOMOUS`
- [ ] `L2_SENIOR_REQUIRED`
- [ ] `L3_HUMAN_REQUIRED`
- [ ] `L4_SAFETY_STOP`

Human decision ID when applicable: <!-- none or DECISION-ID -->
Hermes selection form used when applicable: <!-- yes/no/not applicable -->

Explain the classification. A phase boundary or ordinary technical uncertainty
is not by itself L3.

## Evidence level

- [ ] code inspected/implemented
- [ ] compiled
- [ ] unit tests passed
- [ ] fake-engine tests passed
- [ ] pinned real-engine test passed
- [ ] native JACK tested
- [ ] PipeWire-JACK tested
- [ ] target hardware tested
- [ ] soak/performance tested

Exact commands/workflows, exact revision and results:

```text
<!-- evidence -->
```

Unavailable evidence and reason:

```text
<!-- none / not applicable / unavailable -->
```

## Architecture and safety

- [ ] Seq66 remains authority for intent, project, lifecycle, routing and UI.
- [ ] desired and observed state remain separate.
- [ ] outbound delivery is not treated as completion.
- [ ] no UI or real-time path blocks on OSC/process/file work.
- [ ] runtime indexes are not persisted as identity.
- [ ] ALSA-only remains hard `backend_unavailable`.
- [ ] MIDI-only behaviour is preserved.
- [ ] no destructive operation, history rewrite or silent requirement relaxation.

## Senior consultation

- [ ] planning consultation completed when required
- [ ] failure consultation completed when required
- [ ] exact-head task merge review completed
- [ ] exact-head phase review completed when this is a phase gate

Consultation reference:
Exact head reviewed:
Verdict:
Blocking findings:
Non-blocking risks:

A material change after review invalidates the verdict.

## Project-control updates

- [ ] `WORK-QUEUE.md` updated
- [ ] `TRACEABILITY.md` updated
- [ ] `DECISIONS.md` updated for durable choices
- [ ] `CHANGELOG-FORK.md` updated for durable notable changes
- [ ] `PROJECT-MANIFEST.json` updated when live state changed
- [ ] immutable checkpoint written
- [ ] CURRENT points to the correct checkpoint
- [ ] autonomy policy and documents remain consistent

## Autonomous merge and phase gate

Complete for every autonomous merge.

- [ ] task or phase gate exists in the control plane
- [ ] dependencies and definition of done are satisfied
- [ ] scope matches the declared unit
- [ ] all required checks pass on the exact head below
- [ ] focused negative tests are present where applicable
- [ ] project-control validator passes
- [ ] autonomy-policy validator passes
- [ ] traceability and checkpoint are current
- [ ] senior exact-head gate accepts without blockers
- [ ] no unresolved review thread remains
- [ ] PR is mergeable and targets the intended base
- [ ] expected-head protection will be used
- [ ] merge commit will be used unless versioned policy says otherwise
- [ ] no unresolved L3 or L4 trigger is crossed

For a phase transition additionally:

- [ ] current phase definition of done is satisfied
- [ ] full phase matrix passes on the integrated exact head
- [ ] next phase is already approved and bounded in the roadmap
- [ ] next phase has a first real task
- [ ] residual risks are recorded and senior-classified non-blocking
- [ ] no product, subjective, incompatibility, licensing, data-loss or other L3
      choice remains

Expected head:
Expected base:
Required workflow runs:
Planned merge method: `merge`

## Human interaction when L3/L4 applies

- [ ] native Hermes selection form used when supported
- [ ] two to four concrete options provided
- [ ] senior recommendation visibly marked
- [ ] `Otra opción / Other` with free text included when safe
- [ ] plain chat fallback reason recorded when a form was not used

## Upstream compatibility

- [ ] targets `fork-main`, not `master`
- [ ] unrelated upstream refactors excluded
- [ ] upstream conflicts described
- [ ] tested/upstream revisions recorded when changed

## Remaining risks and next executable action

Separate blocking and non-blocking risks. Give one bounded next action.
