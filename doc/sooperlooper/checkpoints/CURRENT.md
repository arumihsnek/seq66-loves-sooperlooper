# Current checkpoint — V3.8 rejected dispatch contract forensics (CP-073)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-08-CP-073-v3-8-rejected-dispatch-contract-forensics.md`

Checkpoint ID: `CP-073`
Checkpoint date: 2026-08-08
Phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — v3.8 forensic review of rejected dispatch)
Status: `FORENSIC_COMPLETE — v3.8 ELIGIBILITY REVOKED — HUMAN_DECISION_PENDING`
Branch: `integration/baseline-qualification-20260805`

Published source head: resolved externally after publication.
Exact-head CI: required on resulting PR head (Project control plane +
Audio integration core, head_sha == new PR head).

## Verification summary

- Forensic review complete: F-A1 FINALIZER_OVERCONSTRAINT, F-B1
  LEAF_CONTRACT_VIOLATION, F-B2 FINALIZER_REPRESENTATION_OVERCONSTRAINT,
  F-B3 FINALIZER_OVERCONSTRAINT (controlled experiments + Layer-1 scan).
- R_A `9a09b5f6…` / R_B `357fe389…` preserved byte-exact (manifests + git
  bundles with C prerequisite, verified recoverable) — forensic evidence only.
- SELFTEST_GLOBAL=PASS proves internal consistency, NOT normative alignment.
- Senior forensic verdict (changes_required, execution `0631a805…`):
  Alternative B confirmed — **v3_8_technical_dispatch_eligibility = REVOKED**;
  v3.9 package correction recommended; no failed R integration-eligible.
- Evidence published under
  `receipts/headless-lab-v3.8-dispatch-forensic/20260808T223904Z-v3-8-authorized-lab-dispatch/`.

## Authorization scope (this checkpoint)

- v3_8_forensic_review_authorized = true (CONSUMED).
- failed_R_preservation_authorized = true (done).
- NEW_V3_8_DISPATCH_AUTHORIZED = false; REPLACEMENT_LEAF = false;
  SECOND_DISPATCH = false.
- V3_9_PACKAGE_PREPARATION_AUTHORIZED = false (recommended, not authorized).
- integration = false; D0/D1/D2 = false; PR_merge = false; force_push = false;
  worktree_deletion = false.
- Single-writer lock held for this session
  (`~/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`).

## Next action

Human decision required (exactly one):
1. **Authorize v3.9 package correction preparation** (recommended by senior
   forensic review) — aligns finalizer gates with Layer 1 (loader context,
   annotation semantics, missing-file behavior) while retaining the
   tracked-ELF prohibition, or
2. Reject / alternative direction.

A new v3.8 dispatch MUST NOT be authorized (eligibility REVOKED).
No integration, no D0/D1/D2, no PR merge authorized by this checkpoint.
