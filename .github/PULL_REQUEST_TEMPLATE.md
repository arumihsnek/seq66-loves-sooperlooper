## Task and scope

Task ID: <!-- e.g. M2-002 -->
Requirement IDs: <!-- e.g. BACKEND-001, FAIL-005 -->
Target branch: `fork-main`

Describe the smallest coherent vertical change and explicitly list anything out
of scope.

## Autonomy classification

Decision level:

- [ ] `L1_AUTONOMOUS`
- [ ] `L2_SENIOR_REQUIRED`
- [ ] `L3_HUMAN_REQUIRED`
- [ ] `L4_SAFETY_STOP`

Human decision ID, when applicable: <!-- none or DECISION-ID -->

Explain why the selected level is correct. An ordinary technical uncertainty is
not a valid L3 escalation.

## Evidence level

Check only evidence actually produced:

- [ ] code inspected/implemented
- [ ] compiled
- [ ] pure/unit tests passed
- [ ] fake-engine OSC tests passed
- [ ] pinned real SooperLooper test passed
- [ ] native JACK tested
- [ ] PipeWire-JACK tested
- [ ] Raspberry Pi/target hardware tested
- [ ] soak/performance tested

Exact commands/workflows and results:

```text
<!-- command/workflow, exact revision, result -->
```

Unavailable evidence and reason:

```text
<!-- explicit none / not applicable / unavailable -->
```

## Architecture and safety

- [ ] Seq66 remains authority for intent, project, lifecycle, routing and UI.
- [ ] desired and observed state remain separate.
- [ ] outbound OSC delivery is not treated as completion.
- [ ] no UI or real-time path performs blocking OSC/process/file operations.
- [ ] runtime loop indexes are not persisted as clip identity.
- [ ] ALSA-only audio remains hard `backend_unavailable`, not mute.
- [ ] existing MIDI-only behaviour is preserved.
- [ ] no destructive operation, shared-history rewrite or silent requirement
      relaxation is included.

## Protocol changes

- [ ] no protocol change
- [ ] paths/signatures/ranges are verified against pinned SooperLooper source
- [ ] fake-engine tests cover valid and invalid forms
- [ ] `OSC-CONTROL-AND-FEEDBACK.md` and traceability are updated

## Senior consultation

- [ ] not required by policy
- [ ] planning consultation completed
- [ ] failure consultation completed
- [ ] exact-head merge-gate consultation completed

Consultation reference/execution:
Exact head reviewed:
Verdict:
Blocking findings:
Non-blocking risks:

A material code, test or contract change after the recorded review invalidates
the merge verdict.

## Project-control updates

- [ ] `WORK-QUEUE.md` task status/next action updated
- [ ] `TRACEABILITY.md` updated when requirements/evidence changed
- [ ] `DECISIONS.md` updated for durable choices
- [ ] `CHANGELOG-FORK.md` updated for notable durable changes
- [ ] `PROJECT-MANIFEST.json` updated when phase/task/branch/revisions changed
- [ ] new immutable checkpoint written
- [ ] `checkpoints/CURRENT.md` points to the new checkpoint
- [ ] PR body states exact failures, risks and next task ID
- [ ] `PROJECT-AUTONOMY.json` and autonomy documents remain consistent

## Autonomous merge gate

Complete this section for an ordinary autonomous merge. Milestone transitions
still require a human decision.

- [ ] task exists in `WORK-QUEUE.md` and dependencies are satisfied
- [ ] scope matches one task or a declared integration unit
- [ ] all required checks pass on the exact head below
- [ ] focused negative tests are present where behaviour changes
- [ ] project-control validator passes
- [ ] autonomy-policy validator passes
- [ ] traceability and immutable checkpoint are current
- [ ] senior merge gate accepts the exact head without blockers
- [ ] no unresolved review thread remains
- [ ] PR is mergeable and targets the intended base
- [ ] expected-head protection will be used
- [ ] merge commit will be used unless a versioned policy says otherwise
- [ ] this PR does not close a milestone or open the next one

Expected head:
Expected base:
Required workflow runs:
Planned merge method: `merge`

## Upstream compatibility

- [ ] branch targets `fork-main`, not `master`
- [ ] unrelated upstream refactors are excluded
- [ ] conflicts with upstream Seq66 are described
- [ ] upstream/tested revisions are recorded when changed

## Remaining risks and next executable action

List blocking and non-blocking risks separately. Give one concrete next action;
do not write a vague “continue work.”
