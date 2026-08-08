# Current checkpoint — V3.8 canonical payload publication (CP-071)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-08-CP-071-v3-8-canonical-payload-publication.md`

Checkpoint ID: `CP-071`
Checkpoint date: 2026-08-08
Phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — v3.8 canonical payload publication)
Status: `v3.8 EXACT_BYTES_REVIEWED_NOT_DISPATCHED — HUMAN_DECISION_PENDING`
Branch: `integration/baseline-qualification-20260805`

Published source head: resolved externally after publication.
Exact-head CI: required on resulting PR head (Project control plane +
Audio integration core, head_sha == new PR head).

## Verification summary

- v3.8 reviewed frozen payload published byte-exact to GitHub
  `receipts/headless-lab-v3.8/` (operational count=19) + canonical manifest
  (self-excluded, `operational_file_count=19`, `manifest_self_excluded=true`).
- Manifest raw SHA-256 `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`;
  19/19 payload hashes match manifest AND corrected senior binding.
- Publication binding:
  `receipts/headless-lab-v3.8-publication-closure/payload-publication-binding.json`
  (canonical paths, SHA-256, expected Git blob OIDs; future commit SHA not embedded).
- Corrected review binding remains valid: effective accepts=3, blocking=0,
  required=0, dispatch_authorized=false.
- Zero payload modification; byte_exact_copy=true.
- Pre-push validation PASS (byte compare 19/19 + manifest, manifest SHA,
  `git diff --check`, `validate-project-control.py`, JSON parse, py_compile).
- Post-push GitHub verification, PR mergeability and exact-head CI: PENDING.

## Authorization scope (this checkpoint)

- v3_8_payload_modification_authorized = false (payload untouched, byte-exact copy only).
- v3_8_review_binding_modification_authorized = false.
- v3_8_new_generation_authorized = false.
- v3_8_dispatch_authorized = false.
- leases = planned, leases_activated = false.
- leaves = 0, lab_execution_worktrees = 0, remote_result_branches = 0.
- integration_authorized = false, D0_D1_D2_authorized = false,
  PR_merge_authorized = false.
- Single-writer lock held for this session
  (`~/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`).

## Next action

Push the canonical payload publication commit (fast-forward only, no force, no
fork-main write). Verify canonically from GitHub (manifest SHA + 19/19 hashes +
blob OIDs). Observe exact-head CI on the resulting PR head. Then
HUMAN_DECISION_PENDING: authorize or reject exactly one fresh v3.8 LAB-A/LAB-B
dispatch. No leaves, no worktrees, no integration, no D0/D1/D2, no PR merge
authorized by this checkpoint.
