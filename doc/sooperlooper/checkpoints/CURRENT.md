# Current checkpoint — V3.8 authorized dispatch results (CP-072)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-08-CP-072-v3-8-authorized-dispatch-results.md`

Checkpoint ID: `CP-072`
Checkpoint date: 2026-08-08
Phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — v3.8 authorized dispatch execution + results)
Status: `DISPATCH_EXECUTED — REJECTED — HUMAN_DECISION_PENDING`
Branch: `integration/baseline-qualification-20260805`

Published source head: resolved externally after publication.
Exact-head CI: required on resulting PR head (Project control plane +
Audio integration core, head_sha == new PR head).

## Verification summary

- EXACTLY ONE authorized dispatch executed: LAB-A leaf + LAB-B leaf against C
  (`50953878…`), canonical package from GitHub (manifest `bb4182…`).
- R_A `9a09b5f6…`, R_B `357fe389…` (R^==C, owned-only) — controller R-only
  validation PASS; 8/8 literal tests PASS (controller re-run).
- Fail-closed prepublish finalizer: **FAIL both labs** (LAB-A
  sooperlooper_launcher relative import; LAB-B tracked ELF + annotation
  mismatches + return_shape FAIL) → **no E, no publication**.
- Package selftest SELFTEST_GLOBAL=PASS (24/24) confirms finalizer
  consistency; R commits non-conforming.
- **GLOBAL DISPATCH VERDICT = DISPATCH_REJECTED**.
- v3_8_dispatch_authorized = CONSUMED; count_used = 1.
- Leases terminal failed_validation/consumed=true (never planned again).
- replacement_leaf=false, second_dispatch=false.
- integration=false, D0/D1/D2=false, PR merge=false.
- Full dispatch evidence (34 receipts, index self-excluded) published under
  `receipts/headless-lab-v3.8-dispatch/20260808T223904Z-v3-8-authorized-lab-dispatch/`.
- Corrected review binding remains valid (3/0/0); canonical package untouched.

## Authorization scope (this checkpoint)

- v3_8_dispatch_authorized = CONSUMED (count=1, not renewable).
- integration_authorized = false; D0_D1_D2_authorized = false;
  PR_merge_authorized = false; force_push = false.
- Leases: failed_validation/consumed=true both.
- Leaves/worktrees: 2 local (not deleted; deletion not authorized).

## Next action

Human decision required:
1. Forensic review of the rejected dispatch (R commits, finalizer findings,
   evidence receipts), or
2. NEW human authorization for a fresh v3.8 dispatch — the next leaf attempt
   must implement canonical fixture semantics (standalone-loadable modules;
   LAB-B fixtures as dirs + `.gitkeep`; `from __future__ import annotations`;
   `analyze_loop_reproduction` returning dict).

No integration, no D0/D1/D2, no PR merge authorized by this checkpoint.
