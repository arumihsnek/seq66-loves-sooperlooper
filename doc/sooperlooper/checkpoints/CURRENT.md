# Current checkpoint — V3.8 post-publication control-plane closure (CP-070)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-08-CP-070-v3-8-post-publication-control-plane-closure.md`

Checkpoint ID: `CP-070`
Checkpoint date: 2026-08-08
Phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — v3.8 post-publication control-plane closure)
Status: `v3.8 EXACT_BYTES_REVIEWED_NOT_DISPATCHED — HUMAN_DECISION_PENDING`
Branch: `integration/baseline-qualification-20260805`

Published source head: resolved externally after this checkpoint commit.
Exact-head CI: required on the resulting PR head (Project control plane +
Audio integration core, head_sha == new PR head).

## Verification summary

- v3.8 payload unchanged: manifest SHA-256
  `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`,
  19/19 payload hashes, `PAYLOAD_BYTE_DRIFT=false`.
- Corrected review binding valid: effective accepts=3, blocking=0, required=0,
  `dispatch_authorized=false`.
- CP-069 immutable; CP-068 byte-identical across branches; base-branch incident
  (a14c3f6c on fork-main) preserved, no fork-main rewrite.
- Topology reconciliation: merge commit `f3fafd3c` (parents 5649ca36 +
  a14c3f6c); single conflict CURRENT.md resolved source-side; zero payload /
  validation / review-binding / checkpoint drift.
- Staleness defects recorded: CP-F1_CURRENT_BRANCH_STALE=true,
  CP-F2_CURRENT_HEAD_STALE=true (fixed by this checkpoint).
- PR #34 mergeability and exact-head CI: PENDING after publication.

## Authorization scope (this checkpoint)

- v3_8_payload_modification_authorized = false (payload untouched).
- v3_8_review_binding_modification_authorized = false.
- v3_8_dispatch_authorized = false.
- leases = planned, leases_activated = false.
- leaves = 0, lab_execution_worktrees = 0, remote_result_branches = 0.
- integration_authorized = false, D0_D1_D2_authorized = false,
  PR_merge_authorized = false.
- Single-writer lock held for this session
  (`~/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`).

## Next action

Push the reconciled source branch (fast-forward only). Observe exact-head CI on
the resulting PR head. Then HUMAN_DECISION_PENDING: authorize or reject exactly
one fresh v3.8 LAB-A/LAB-B dispatch. No leaves, no worktrees, no integration,
no D0/D1/D2, no PR merge authorized by this checkpoint.
