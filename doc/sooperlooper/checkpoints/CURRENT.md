# Current checkpoint — V3.9 normative gate alignment package (CP-074)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-08-CP-074-v3-9-normative-gate-alignment-package.md`

Checkpoint ID: `CP-074`
Checkpoint date: 2026-08-08
Phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — v3.9 normative gate alignment package)
Status: `V3.9 EXACT_BYTES_REVIEWED_NOT_DISPATCHED — HUMAN_DECISION_PENDING`
Branch: `integration/baseline-qualification-20260805`

Published source head: resolved externally after publication.
Exact-head CI: required on resulting PR head (Project control plane +
Audio integration core, head_sha == new PR head).

## Verification summary

- v3.9 package frozen and triple-reviewed (3 × accept, blocking=0,
  required=0): manifest `71596b8472b459940ee79fe9df8d3687dd3dbbdf5bf4696d897a9e4a1bdb3bba`,
  21 operational files, supersedes v3.8 (`bb4182…`, REJECTED_AND_FORENSICALLY_CLOSED).
- F-A1 corrected (package import context, NA1 PASS); F-B2 corrected (semantic
  annotations, NA2/NA3 PASS, I2/I3 REJECT); F-B3 corrected (valid WAV return
  shape, NA4/C1 PASS, C2 REJECT, C3 no-block); F-B1 retained fail-closed and
  leaf-visible (NA5/D1 REJECT, D2 PASS).
- SELFTEST_INTERNAL_CONSISTENCY=PASS, SELFTEST_NORMATIVE_ALIGNMENT=PASS,
  SELFTEST_GLOBAL=PASS.
- Gate traceability: 25 gates with normative authority; orphan gate REJECT
  (NA6); leaf-requirements-v3.9.json LAB-A 10 / LAB-B 12.
- Review binding created (3 exec IDs: 5e86f9c8, 753f1500, 55904bee;
  accept_count=3; blocking=0; required=0; non-circular).
- Replay: R_A no longer rejected for F-A1; R_B still rejected (tracked ELF +
  diff_snapshots drift); both forensic-only.
- v3_8_status=REJECTED_AND_FORENSICALLY_CLOSED;
  v3_8_technical_dispatch_eligibility=REVOKED;
  v3_9_technical_dispatch_eligibility=PASS; v3_9_dispatch_authorized=false.

## Authorization scope (this checkpoint)

- v3_9_package_preparation_authorized = true (CONSUMED).
- v3_9_dispatch_authorized = false; leaves = 0; leases = planned,
  leases_activated = false; remote_result_branches = 0.
- integration_authorized = false; D0_D1_D2_authorized = false;
  PR_merge_authorized = false; force_push = false; worktree_deletion = false.
- Single-writer lock held for this session
  (`~/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`).

## Next action

Human decision required (exactly one):
1. **Authorize exactly ONE fresh v3.9 LAB-A/LAB-B dispatch** (technical
   eligibility PASS, binding 3×accept), or
2. Reject.

No integration, no D0/D1/D2, no PR merge authorized by this checkpoint.
