# CP-017 — Phase 1 gate governance correction

Checkpoint ID: `CP-017`
Checkpoint date: 2026-08-03
Phase: `phase-1-protocol-core`
Active task: `M1-008`
Branch: `gate/m1-008-phase-1`
Draft PR: `#14`

## Objective

Record the successful technical Phase 1 gate while preserving the explicit
human-approval boundary required by PR #14.

This checkpoint supersedes the active-control interpretation of CP-016.
CP-016 remains immutable historical evidence and is not modified.

## Completed

- The Phase 1 implementation and corrective cycle are published.
- The final senior consultation returned `accept` with no blocking findings.
- Audio integration core passed on head
  `1ff326a7b5e57cd04f2b99ec8a4a228217281642`.
- The pinned real-SooperLooper/JACK-dummy smoke passed on the same head.
- CP-016 was published but its control-state transition was rejected by
  the project-control validator.
- The project remains at the human Phase 1 gate.
- Phase 2 has not begun.

## Verification

Evidence on head
`1ff326a7b5e57cd04f2b99ec8a4a228217281642`:

- Audio integration core:
  - run `30818190320`
  - job `91701103380`
  - result: PASS
- Real SooperLooper headless smoke:
  - run `30818190722`
  - job `91701106486`
  - result: PASS
- Project control plane:
  - run `30818190756`
  - job `91701104433`
  - result: FAIL

The control-plane failure was caused by:

- missing mandatory CP-016 headings;
- manifest active task `none yet` not existing in WORK-QUEUE;
- CURRENT.md not matching manifest current phase and active task;
- unsupported phase token `phase-2-managed-engine`.

The failure was not a shell-quoting artifact.

## Current state

- Current phase: `phase-1-protocol-core`.
- Active task: `M1-008`.
- Active-task status: `review`.
- PR #14 remains draft.
- Technical gate verdict: PASS.
- Senior consultation verdict: `accept`.
- Human approval: pending.
- Phase 2 authorization: pending human approval and merge of PR #14.
- No Phase 2 implementation work is authorized by this checkpoint.

## Risks and unresolved questions

- TSAN cannot currently execute in the known ARM64 environment.
- The real-engine CI smoke is non-realtime and uses JACK dummy.
- Subscription intervals remain untuned on target hardware.
- CP-016 contains a premature Phase 2 transition and is superseded by CP-017
  for current control-state purposes.
- Human review may still request changes despite the technical gate passing.

## Next executable action

1. Correct manifest, CURRENT.md and WORK-QUEUE to represent M1-008 review.
2. Run project-control validation locally.
3. Push the CP-017 correction.
4. Require all three workflows to pass on the resulting exact head.
5. Present PR #14 for explicit human approval.
6. Merge only after that approval.
7. Create a post-merge checkpoint before opening the first Phase 2 task.

## Open first

1. `PROJECT-MANIFEST.json`
2. `doc/sooperlooper/checkpoints/CURRENT.md`
3. this checkpoint
4. `doc/sooperlooper/WORK-QUEUE.md`
5. PR #14 and checks for its exact head
6. CP-016 as historical evidence only

## Safe reference point

Published pre-correction head:

`1ff326a7b5e57cd04f2b99ec8a4a228217281642`

PR:

`https://github.com/arumihsnek/seq66-loves-sooperlooper/pull/14`

Audio core and real-engine smoke pass on that head. The remaining failure is
limited to the project-control state transition.
