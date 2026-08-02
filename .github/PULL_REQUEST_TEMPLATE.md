## Task and scope

Task ID: <!-- e.g. M1-001 -->
Requirement IDs: <!-- e.g. OSC-001, STATE-001 -->
Target branch: `fork-main`

Describe the smallest coherent vertical change and explicitly list anything out
of scope.

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
<!-- command/workflow, revision, result -->
```

## Architecture and safety

- [ ] Seq66 remains authority for intent, project, lifecycle, routing and UI.
- [ ] desired and observed state remain separate.
- [ ] outbound OSC delivery is not treated as completion.
- [ ] no UI or real-time path performs blocking OSC/process/file operations.
- [ ] runtime loop indexes are not persisted as clip identity.
- [ ] ALSA-only audio remains hard `backend_unavailable`, not mute.
- [ ] existing MIDI-only behaviour is preserved.

## Protocol changes

- [ ] no protocol change
- [ ] paths/signatures/ranges are verified against pinned SooperLooper source
- [ ] fake-engine tests cover valid and invalid forms
- [ ] `OSC-CONTROL-AND-FEEDBACK.md` and traceability are updated

## Project-control updates

- [ ] `WORK-QUEUE.md` task status/next action updated
- [ ] `TRACEABILITY.md` updated when requirements/evidence changed
- [ ] `DECISIONS.md` updated for durable choices
- [ ] `CHANGELOG-FORK.md` updated for notable durable changes
- [ ] `PROJECT-MANIFEST.json` updated when phase/task/branch/revisions changed
- [ ] new immutable checkpoint written
- [ ] `checkpoints/CURRENT.md` points to the new checkpoint
- [ ] PR body states exact failures, risks and next task ID

## Upstream compatibility

- [ ] branch targets `fork-main`, not `master`
- [ ] unrelated upstream refactors are excluded
- [ ] conflicts with upstream Seq66 are described
- [ ] upstream/tested revisions are recorded when changed

## Remaining risks and next executable action

<!-- Give one concrete next action; do not write a vague “continue work”. -->
