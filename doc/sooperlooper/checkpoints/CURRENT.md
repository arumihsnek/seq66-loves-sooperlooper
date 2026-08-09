# Current checkpoint — V3.9 authorized lab dispatch completed (CP-075)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-09-CP-075-v3-9-authorized-lab-dispatch.md`

Checkpoint ID: `CP-075`
Checkpoint date: 2026-08-09
Phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — v3.9 authorized lab dispatch)
Status: `V3.9 DISPATCH COMPLETED — exact-head senior merge review pending`
Branch: `integration/baseline-qualification-20260805`

Published source head: resolved externally after publication.
Exact-head CI: required on resulting PR head (Project control plane +
Audio integration core, head_sha == new PR head).

## Verification summary

- v3.9 dispatch executed with frozen package `71596b84…` (3×accept binding):
  leaves LAB-A/LAB-B → R_A `46bd5f52…` / R_B `122303ee…` (both R^==C, clean,
  zero tracked ELF, 8/8 frozen literal tests exit 0).
- Finalizer v3.9: prepublication_verdict PASS for both (zero gate failures).
- Result branches published (create-only) at E commits
  `7db1a1b7…` (lab-a) / `71a63065…` (lab-b); postpublish binding PASS.
- D0/D1/D2 complete: dispatch, postpublish binding + evidence publication,
  checkpoint/PR/CI.
- v3_9_dispatch_authorized=true (human) — consumed; leases active; leaves=2;
  remote_result_branches=2.

## Authorization scope (this checkpoint)

- integration_authorized=true, D0_D1_D2_authorized=true (consumed),
  PR_merge_authorized=true (pending exact-head senior merge review).
- force_push=false; worktree_deletion=false.
- Single-writer lock held for this session.

## Next action

1. Exact-head senior merge review (merge-gate) on the new control-plane head.
2. On accept (blocking=0): merge PR #34 to fork-main with expected-head
   protection per AUTONOMOUS-MERGE.md.
3. Post-transition checkpoint.
