# Checkpoint CP-072 — V3.8 authorized dispatch results (REJECTED)

Checkpoint ID: `CP-072`
Checkpoint date: 2026-08-08
Supersedes: `CP-071` (2026-08-08, immutable)
Phase: `phase-5-exact-recording`

## Objective

Execute EXACTLY ONE authorized fresh v3.8 LAB-A / LAB-B dispatch (one leaf
attempt per lab) against candidate C
`509538784afc2b828f2d922f65cf8ca3a39b5ee7`, using the canonical v3.8 package
from GitHub (head `4176fc85…`, manifest `bb4182…`). Then record the
fail-closed result. No replacement leaf, no second dispatch, no integration,
no D0/D1/D2, no PR merge.

## Completed

- Preflights PASS: PR #34 open/draft/mergeable=true/head `4176fc85…`; CURRENT→
  CP-071; exact-head CI runs 31281504150/31281504161 success on `4176fc85`;
  zero v3.8 remote branches/leases/leaves; canonical package recovered from
  GitHub (manifest raw SHA `bb4182…`, 19/19 hashes, publication binding
  3/0/0, blob OIDs cross-check); cross-manifest verdict PASS (59 sources);
  package selftest `SELFTEST_GLOBAL=PASS` (24/24 cases).
- Single-writer lock held throughout (flock, PID 2177972).
- DISPATCH_RUN_ID `20260808T223904Z-v3-8-authorized-lab-dispatch`; planned
  result branches absent (no collision); leases
  `LEASE-20260808T132428Z-LAB-A-V3_8` / `-LAB-B-…` activated (consumed=false).
- Two isolated leaf worktrees from C (detached, clean, push disabled,
  `leaf_can_push=false`); leaves produced:
  - R_A `9a09b5f6cd4019873886c7714276312b62c7e4f5` (tree `16e27313…`, R^==C,
    5 owned paths)
  - R_B `357fe38983784c721e25ed2c8ae59a6303e2ef5d` (tree `a1eb6cc1…`, R^==C,
    12 owned paths)
- Controller R-only validation PASS for both (ownership, forbidden, topology,
  cleanliness after restoring `build/README`).
- Controller re-execution of the 8 literal tests on detached R: 8/8 PASS
  (hash_match=true, exit=0).
- Verifier identity-coherence + functional-continuity: PASS for both.
- **Fail-closed prepublish finalizer (mechanical-finalizer-v3.8.py): FAIL for
  BOTH labs → no E, no evidence commit.**
  - LAB-A: runtime full contract structural=FAIL —
    `sooperlooper_launcher.py` import failed (relative import; module not
    standalone-loadable).
  - LAB-B: `zero tracked ELF` FAIL (`fixtures/deterministic_capture`,
    `fixtures/synthetic_source` were committed as ELF binaries; canonical
    design materializes those fixtures as directories with `.gitkeep` and
    compiles to `build/` only) + runtime full contract structural=FAIL
    (annotation mismatches: literal `str | Path`/`int`/`AnalysisResult`
    expected, evaluated objects observed) + return_shape=FAIL
    (`analyze_loop_reproduction("x.wav","y.wav",4800)` raises
    FileNotFoundError instead of returning dict).
- Remote absence verified pre-E and pre-push for both planned branches
  (ABSENT, rc=2 semantics).
- No publication performed (no E). No A. No postpublish.
- Full dispatch evidence (34 receipts) written under
  `receipts/headless-lab-v3.8-dispatch/<DISPATCH_RUN_ID>/` including
  dispatch-index (self-excluded) and dispatch-verdict.

## Verification

- PR pre-dispatch head `4176fc85cdb9d573acd77eb01c310657d8c2bc8c` verified
  before lease activation.
- Canonical manifest `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`
  verified from GitHub raw bytes; 19/19 payload hashes match manifest and
  corrected review binding (3/0/0).
- R topology: R^==C, R!=C, single parent, owned-only diff, no forbidden
  paths — PASS both.
- Literal tests: actual argv hash == declared for all 8; exit_code=0 all.
- Finalizer gate failures reproduced programmatically (structural findings,
  return-shape checks) and via CLI (prepublication_verdict=FAIL, E_created=false).
- Package selftest (real CLI, 24 adversarial cases): `SELFTEST_GLOBAL=PASS`
  (exit 0) — confirms the finalizer rejects non-conforming R and accepts the
  canonical fixture R.

## Current state

- `dispatch_verdict = DISPATCH_REJECTED`
- `v3_8_dispatch_authorized = CONSUMED`; `v3_8_dispatch_count_used = 1`
- `replacement_leaf_authorized = false`; `second_dispatch_authorized = false`
- `LAB_A_R = 9a09b5f6…`; `LAB_A_E = none`; `LAB_A_A = none`
- `LAB_B_R = 357fe389…`; `LAB_B_E = none`; `LAB_B_A = none`
- Leases terminal: `failed_validation`, `consumed=true` (never back to planned)
- `integration_authorized = false`; `D0_D1_D2_authorized = false`;
  `PR_merge_authorized = false`
- Remote result branches: 0; lab worktrees: 2 (local, not deleted —
  WORKTREE_DELETION_AUTHORIZED=false)
- Corrected review binding remains valid (3/0/0) and the canonical package is
  untouched (payload source of truth = GITHUB).

## Risks and unresolved questions

- The R commits are non-conforming; no implementation result is eligible for
  future integration.
- Future dispatch requires a NEW human authorization; the next leaf attempt
  must implement the canonical fixture semantics (standalone-loadable modules;
  LAB-B fixtures as directories + `.gitkeep`; `from __future__ import
  annotations`; `analyze_loop_reproduction` returning dict).
- Exact-head CI on the new control-plane head is required after this
  checkpoint commit.

## Next executable action

Publish controller-owned dispatch evidence + CP-072 + CURRENT to the
control-plane source branch `integration/baseline-qualification-20260805`
(fast-forward only, no fork-main writes, no force). Then human decision:
forensic review of the rejected dispatch, or a new human authorization for a
fresh dispatch. No integration.

## Open first

- PR #34 exact head (after control-plane commit)
- `receipts/headless-lab-v3.8-dispatch/20260808T223904Z-v3-8-authorized-lab-dispatch/` on GitHub
- Project control plane run (new head)
- Audio integration core run (new head)
- CP-072

## Safe reference point

- pre_dispatch_control_pr_head = `4176fc85cdb9d573acd77eb01c310657d8c2bc8c`
- candidate = `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- package_manifest_sha256 = `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`
- R_A = `9a09b5f6cd4019873886c7714276312b62c7e4f5` (non-conforming, no E)
- R_B = `357fe38983784c721e25ed2c8ae59a6303e2ef5d` (non-conforming, no E)
- dispatch_evidence_path = `receipts/headless-lab-v3.8-dispatch/20260808T223904Z-v3-8-authorized-lab-dispatch/`
