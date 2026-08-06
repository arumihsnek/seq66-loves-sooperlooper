# Current checkpoint — V3.4 functional-contract restoration

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-06-CP-060-v3-4-functional-contract-restoration.md`

Checkpoint ID: `CP-060`
Checkpoint date: 2026-08-06
Supersedes: `CP-059` (2026-08-06, immutable)

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task `P5-005` in_progress; P5-007 deferred.
- Headless-lab packages: v3 / v3.1 / v3.2 / v3.3 IMMUTABLE_REJECTED_FOR_DISPATCH (historical evidence);
  v3.4 EXACT_BYTES_REVIEWED_NOT_DISPATCHED.
- Batch: `BATCH-20260806T020249Z-DOGFOOD004-LAB-V3_4`; package manifest
  `63c371ba70491fa546ce4f8d980568440845c74fbb5af3a93502ff9b8c0a4da7`.
- Dual senior review: functional continuity **accept** + strict workflow **accept** (0 blocking, 0 required) —
  neither authorizes dispatch.
- Technical dispatch eligibility: **PASS**; dispatch authorized: **false** (separate fields).
- Stop gate: `HUMAN_DECISION_PENDING`; `redispatch_authorized=false`; `integration_authorized=false`;
  `D0_D1_D2_authorized=false`.
- Leases v3.4 planned only; zero LAB worktrees; zero leaves; D0/D1/D2 not executed.
- Evidence worktrees of prior runs inventoried and preserved (no destructive authorization granted).

## Next action

Human decision: authorize or reject a NEW LAB-A/LAB-B dispatch using EXCLUSIVELY the exact,
functionally continuous, doubly reviewed v3.4 package (`receipts/headless-lab-v3.4/`),
new branches, new worktrees and new leases.

## References

- `PROJECT-MANIFEST.json` (live state)
- `doc/sooperlooper/WORK-QUEUE.md` (P5-005)
- `doc/sooperlooper/checkpoints/2026-08-06-CP-059-v3-3-strict-envelope-correction.md`
- `receipts/headless-lab-v3.4/` and `receipts/headless-lab-v3.4-review-binding/`
