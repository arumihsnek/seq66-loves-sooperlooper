# Current checkpoint — V3.5 process hardening package

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-06-CP-064-v3-5-process-hardening-package.md`

Checkpoint ID: `CP-064`
Checkpoint date: 2026-08-06
Supersedes: `CP-063` (2026-08-06, immutable)

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task `P5-005` in_progress; P5-007 deferred.
- `v3_5_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`; `technical_dispatch_eligibility=PASS`
- `dispatch_authorized=false`; `integration_authorized=false`; `D0_D1_D2_authorized=false`;
  `PR_merge_authorized=false`; leases=planned (activated=false); lab_worktrees=0; leaves=0
- `forensic_closure_final=PASS` (CI runs 31128838866/31128838865 success on 129e5b50)
- Package payload manifest SHA-256: `31b3fd5518153727bc51785553b0dd3ab9223f957ec5a2d270eca90186e946f6`
- Dual senior reviews: functional accept 0/0 (`8411b59c`), process accept 0/0 (`c5021990`); accept
  does NOT authorize dispatch.
- Exact-head CI on the v3.5 evidence commit: observed after push.

## Next action

Human decision: authorize or reject ONE fresh v3.5 LAB-A/LAB-B dispatch using exclusively the
v3.5 package (new branches, new worktrees, new leases). NOT authorized yet; not executed.

## References

- `PROJECT-MANIFEST.json`; `doc/sooperlooper/WORK-QUEUE.md` (P5-005)
- `receipts/headless-lab-v3.5/` (17 artifacts + package-payload-manifest-v3.5.json)
- `receipts/headless-lab-v3.5-validation/`; `receipts/headless-lab-v3.5-review-binding/`
- `doc/sooperlooper/checkpoints/2026-08-06-CP-063-v3-4-forensic-review-binding-correction.md`
