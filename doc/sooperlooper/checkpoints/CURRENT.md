# Current checkpoint — V3.4 authorized dispatch results

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-06-CP-061-v3-4-authorized-dispatch-results.md`

Checkpoint ID: `CP-061`
Checkpoint date: 2026-08-06
Supersedes: `CP-060` (2026-08-06, immutable)

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task `P5-005` in_progress; P5-007 deferred.
- RUN_ID: `20260806T134656Z-v3-4-authorized-lab-dispatch`; dispatch authorized consumed (count=1); package v3.4 manifest `63c371ba...`.
- LAB-A R=`0fd104ef2187346b8c00014829857e11ee9539ec` E=`a30e3c9d888cc55b9d4d7e92bf6250dca4f3ebb3`; LAB-B R=`608cefef3a270a2ea6859b8d48ee931c636743f8` E=`2906dc4fd77e2d7455060c3248db73aabdeb010b` — **DISPATCH RESULT: DISPATCH_RESULT_REJECTED**
  (no integration candidate set).
- `integration_authorized=false`; `D0_D1_D2_authorized=false`; no integration performed.
- Leases v3.4 consumed; leaf/evidence worktrees preserved (no destructive cleanup performed).

## Next action

Human decision: review the exact failure evidence (envelope/binding rejection for both leaves;
implementations pass the frozen literal tests 8/8). Decide a future correction. No integration
candidate set.

## References

- `PROJECT-MANIFEST.json`; `doc/sooperlooper/WORK-QUEUE.md` (P5-005)
- `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/` (evidence)
- `doc/sooperlooper/checkpoints/2026-08-06-CP-060-v3-4-functional-contract-restoration.md`
