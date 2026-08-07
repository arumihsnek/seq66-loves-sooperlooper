# Current checkpoint — V3.7 control-plane conformance recovery

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-07-CP-067-v3-7-control-plane-conformance-recovery.md`

Checkpoint ID: `CP-067`
Checkpoint date: 2026-08-07
Supersedes: `CP-066` (2026-08-07, immutable)

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task `P5-005` in_progress; P5-007 deferred.
- CP-066 remains byte-identical (historical, immutable). CP-067 supersedes it
  only as the current control-plane checkpoint; the frozen v3.7 payload is
  unchanged.
- v3.7 package: `v3_7_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`; payload
  manifest SHA-256
  `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3` unchanged;
  review binding unchanged.
- `dispatch_authorized=false`; `integration_authorized=false`;
  `D0_D1_D2_authorized=false`; `PR_merge_authorized=false`; leases_activated=false;
  lab_execution_worktrees=0; leaves=0; remote_result_branches=0.
- CP-067 adds no functional/process package behavior — control-plane conformance
  recovery only.
- exact_head_ci = PENDING (previous run #166 failed on CP-066 structure; fresh
  exact-head CI observed after publication).

## Next action

Observe exact-head CI after publishing CP-067 (Project control plane, Audio
integration core on the new publication SHA). Not dispatch.

## References

- `PROJECT-MANIFEST.json`; `doc/sooperlooper/WORK-QUEUE.md` (P5-005)
- `receipts/headless-lab-v3.7/` (17 artifacts + package-payload-manifest-v3.7.json)
- `receipts/headless-lab-v3.7-validation/`; `receipts/headless-lab-v3.7-review-binding/`
- `doc/sooperlooper/checkpoints/2026-08-07-CP-066-v3-7-runtime-contract-correction.md`
