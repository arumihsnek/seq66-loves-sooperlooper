# Current checkpoint — v3.8 review-binding + publication closure (CP-069)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-08-CP-069-v3-8-review-binding-publication-closure.md`

Checkpoint ID: `CP-069`
Checkpoint date: 2026-08-08
Phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — v3.8 review-binding correction + publication closure)
Status: `v3.8 EXACT_BYTES_REVIEWED_NOT_DISPATCHED — HUMAN_DECISION_PENDING`
Branch: `fork-main`
Exact head (PR #34): `3090f2e078ca29762f7654b9e5a872f58ad4ac5b`

## Verification summary

- v3.8 payload unchanged: manifest SHA-256 `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`, 19/19 payload hashes, PAYLOAD_BYTE_DRIFT=false.
- CP-068 immutable (published by a14c3f6c on fork-main; NOT modified; no history rewrite).
- Base-branch publication incident documented (PUB-F1..F6): a14c3f6c on fork-main preserved, PR source head remained 3090f2e0, de585b39 never published remotely, merge-base 3585334c.
- Original review binding audited: verdicts accept/accept/continue but three_accepts=true → RB-F1 BLOCKING, ORIGINAL_REVIEW_BINDING_VALID=false.
- Fresh adversarial acceptance gate (seq66-lab-v3-8-adversarial-final-acceptance, final-review): verdict=accept, blocking=0, required=0.
- Append-only correction-v1: corrected-review-binding-v3.8.json with effective_accept_count=3, effective_blocking_count=0, effective_required_actions=0.
- Selftest 32/32; identity negative matrix 15/15 + control PASS; cross-manifest 59/59; inherited negatives 29/29; runtime negatives 12/12; command hashes 8/8; ownership 2/2.
- CI on PR head 3090f2e0: control plane 31226194220 + audio core 31226194221 completed/success.

## Authorization scope (this checkpoint)

- v3_7_dispatch_authorized = false (REVOKED).
- v3_8_dispatch_authorized = false.
- leases = planned, leases_activated = false.
- leaves = 0, lab_execution_worktrees = 0, remote_result_branches = 0.
- integration_authorized = false, D0_D1_D2_authorized = false, PR_merge_authorized = false.
- Single-writer lease: PID `2138684` FD 9 on
  `/home/ubuntu/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`.

## Next action

HUMAN_DECISION_PENDING: authorize or reject exactly one fresh v3.8 LAB-A/LAB-B
dispatch. No leaves, no worktrees, no integration, no D0/D1/D2, no PR merge
authorized by this checkpoint. See CP-069 for full context.
