# Current checkpoint — V3.7 runtime contract correction package

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-07-CP-066-v3-7-runtime-contract-correction.md`

Checkpoint ID: `CP-066`
Checkpoint date: 2026-08-07
Supersedes: `CP-064` (2026-08-06, immutable)

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task `P5-005` in_progress; P5-007 deferred.
- v3.6 package (20260806T234554Z) REJECTED for dispatch (fc-f1/fc-f2/fc-f3); preserved frozen; NOT unfrozen/patched/published.
- v3.7 correction package prepared: `v3_7_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`;
  `v3_7_technical_dispatch_eligibility=PASS`
- `dispatch_authorized=false`; `integration_authorized=false`; `D0_D1_D2_authorized=false`;
  `PR_merge_authorized=false`; leases=planned (activated=false); lab_worktrees=0; leaves=0;
  remote_result_branches=0
- Package payload manifest SHA-256:
  `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3` (supersedes v3.6)
- Triple senior review: functional-continuity accept 0/0 (`eedfae43`), fail-closed-process
  accept 0/0 (`6b4b5dad`), adversarial-validity accept 0/0 (`0e3bdc1d`); accept does NOT
  authorize dispatch.
- Selftest 32/32 PASS; static validator PASS (production_findings=0, debug_markers=0);
  fail-closed 8/8; command-hash 8/8; ownership diff clean both labs; runtime-contract
  negative matrix 12/12.

## Next action

Human decision: authorize or reject ONE fresh v3.7 LAB-A/LAB-B dispatch using exclusively the
frozen v3.7 package (new branches, new worktrees, new leases). NOT authorized yet; not executed.

## References

- `PROJECT-MANIFEST.json`; `doc/sooperlooper/WORK-QUEUE.md` (P5-005)
- `receipts/headless-lab-v3.7/` (17 artifacts + package-payload-manifest-v3.7.json)
- `receipts/headless-lab-v3.7-validation/`; `receipts/headless-lab-v3.7-review-binding/`
- `doc/sooperlooper/checkpoints/2026-08-06-CP-064-v3-5-process-hardening-package.md`
