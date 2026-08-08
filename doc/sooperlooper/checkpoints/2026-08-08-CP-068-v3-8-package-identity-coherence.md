# Checkpoint CP-068 — v3.8 package identity coherence

Checkpoint ID: `CP-068`
Checkpoint date: 2026-08-08
Supersedes: `CP-067` (2026-08-07, immutable)
Phase: `phase-5-exact-recording` (control-plane; package correction, no dispatch)
RUN_ID: `20260808T132428Z-v3-8-identity-coherence-package`
PR: #34 (head `3090f2e078ca29762f7654b9e5a872f58ad4ac5b`)

## Objective

Correct the v3.7 package identity incoherence (ID-F1..ID-F4) as a NEW
generation v3.8, preserving the v3.7 functional surface byte-for-byte, adding
fail-closed cross-manifest identity binding, single-writer control-plane
ownership, and obtaining exact-byte senior review WITHOUT authorizing dispatch,
integration, D0/D1/D2 or PR merge.

## Completed

- CP-067 confirmed immutable; v3.7 exact bytes preserved
  (manifest SHA-256 `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3` MATCH).
- v3.7 identity blocking audit (from exact bytes, receipt
  `v3.7-identity-blocking-audit.json`): ID-F1 scope-batch mismatch,
  ID-F2 supersedes identity drift, ID-F3 finalizer cross-manifest identity not
  enforced, ID-F4 verifier identity binding incomplete — all BLOCKING.
- v3.7 dispatch eligibility REVOKED
  (`v3_7_status=EXACT_BYTES_REVIEWED_REJECTED_FOR_DISPATCH`,
  `v3_7_technical_dispatch_eligibility=REVOKED`).
- v3.8 new generation: single authoritative identity
  (`package-identity-v3.8.json`); batch `BATCH-20260808T132428Z-DOGFOOD004-LAB-V3_8`;
  candidate HEAD `509538784afc2b828f2d922f65cf8ca3a39b5ee7`; leaves `LEAF-LAB-A-V3_8` /
  `LEAF-LAB-B-V3_8`; leases `LEASE-20260808T132428Z-LAB-A-V3_8` /
  `LEASE-20260808T132428Z-LAB-B-V3_8`; supersedes v3.7
  (manifest `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3`, batch `BATCH-20260807T161935Z-DOGFOOD004-LAB-V3_7`).
- Operational payload v3.8 frozen: 19 artifacts
  (17 inherited/transformed v3.7 + `package-identity-v3.8.json` +
  `cross-manifest-coherence-validator.py`); manifest SHA-256
  `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`; `zero_missing=True`, `zero_extra=True`.
- Cross-manifest coherence validator: PASS
  (`59` sources checked, 0 mismatches).
- Finalizer v3.8 fail-closed identity gate (exit 3 `IDENTITY_MISMATCH` with
  named check; no checkout/evidence/E/remote side effects; no fail-open mode).
- Verifier v3.8 independent cross-check (does not trust `evidence.batch`
  alone); full-completion requires identity coherence PASS.
- Negative identity matrix (real temp git repos, real CLI): 15/15 mutations
  REJECT with named check + named exit code 3 (or 1 for cross-manifest); 1
  valid identity control PASSES and creates E. v3.7 mixed-batch reproduction
  REJECT (case N15: scope `20260806T234554Z` vs package `20260807T161935Z`).
- Functional regression: selftest 32/32
  (declared=executed=passed=32,
  failed=0, skipped=0);
  fail-closed 8/8; negative matrix
  29/29; command hashes 8/8 (4 LAB-A + 4 LAB-B);
  ownership diff LAB-A + LAB-B PASS; runtime contract negatives 12/12;
  controlled build PASS; publication create-only PASS; postpublish binding
  PASS; full completion PASS; package validation 19/19; static source PASS.
- Triple exact-byte senior review (frozen manifest + 19 payload hashes):
  - functional-continuity (final-review): verdict=`accept`,
    0 blocking, 0 required (exec_id `c8469792-5d75-45bd-9690-23fd4143cb00`,
    fingerprint `d89ae94526e61772…`).
  - package-identity-coherence (integrated-review): verdict=`accept`,
    0 blocking, 0 required (exec_id `8d738e09-ea1c-4350-af24-844f4bdc2188`,
    fingerprint `dd509260e7982db8…`).
  - adversarial-identity-validity (risk-audit): verdict=`continue`,
    0 blocking, 0 required (exec_id `576c2d81-b0de-46af-8ac1-2e6ee0bc86bc`,
    fingerprint `b98697762f5bd74b…`).
- Review binding (`package-review-binding-v3.8.json`): `three_accepts=True`,
  `blocking=0`, `required_actions_total=0`, all 3 verdict hashes + 3 execution
  ids + 3 fingerprints, bound to manifest + 19 hashes.
- Publication index (`package-publication-index-v3.8.json`): self-exclusion
  (manifest NEVER inside operational payload) and review-exclusion (preflight /
  bundle / verdict / binding / publication-index NEVER inside operational
  payload) rules declared.

## Verification

- `python3 contrib/scripts/validate-project-control.py` (pending publication).
- Receipts persisted under
  `receipts/headless-lab-v3.8/` and `receipts/headless-lab-v3.8-validation/`
  and `receipts/headless-lab-v3.8-review-binding/`.
- Local RUN revalidation from bytes (not from summary): selftest 32/32,
  identity negative matrix 15/15 + control, S1-S11 harness PASS, cross-manifest
  59/59, package validation 19/19, command hashes 8/8, ownership diff 2/2.
- Source state remote: PR #34 open/draft/MERGEABLE; head `3090f2e078ca29762f7654b9e5a872f58ad4ac5b` MATCH.
  CI runs 31226194220 (control plane) and 31226194221 (audio integration core)
  success, exact head.

## Current state

- `v3_7_status = EXACT_BYTES_REVIEWED_REJECTED_FOR_DISPATCH`.
- `v3_7_technical_dispatch_eligibility = REVOKED`.
- `v3_7_dispatch_authorized = false`.
- `v3_8_status = EXACT_BYTES_REVIEWED_NOT_DISPATCHED`.
- `v3_8_technical_dispatch_eligibility = PASS`.
- `v3_8_dispatch_authorized = false`.
- `leases = planned`, `leases_activated = false`.
- `leaves = 0`, `lab_execution_worktrees = 0`, `remote_result_branches = 0`.
- `integration_authorized = false`, `D0_D1_D2_authorized = false`,
  `PR_merge_authorized = false`.
- Single-writer lease: PID `1915047` holds FD 9 on
  `/home/ubuntu/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`
  (reacquired 2026-08-08T20:27:33Z after killing stale PID 407855; no
  concurrent writer detected; remote re-read pre-push required).
- Frozen payload manifest SHA-256: `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`.

## Risks and unresolved questions

- V3.8 dispatch is **NOT** authorized. The next human decision must be
  either authorize or reject **exactly one** fresh v3.8 LAB-A/LAB-B dispatch
  (no integration, no D0/D1/D2, no merge).
- Any change to the frozen payload after this checkpoint forces
  `V3_8_FROZEN_PACKAGE_REJECTED` and a new generation (v3.9 / CP-069+).
- Adversarial review verdict is `continue` (NOT `changes_required`); it carries
  0 blocking findings and 0 required actions, and its `controls` enumerate
  the binding contract: exact manifest SHA, exact 19 hashes, v3.7 preservation,
  fail-closed finalizer/verifier, self-exclusion rules, single-writer lease,
  and the no-dispatch-without-human-authorization rule. Treated as a valid
  advisory verdict (status=COMPLETED, detailed_status=VALID_ADVISORY_VERDICT).
- Working tree currently has uncommitted changes on `fork-main` from non-v3.8
  work (stashed); they are not part of this publication.

## Next executable action

- The human authorizes or rejects **exactly one** fresh v3.8 LAB-A/LAB-B
  dispatch.
- This checkpoint does **not** start the dispatch, integration, D0/D1/D2 or
  PR merge.
- After human authorization, a fresh controller session will:
  (1) re-read PR head; (2) re-acquire single-writer lock;
  (3) activate LEASE-20260808T132428Z-LAB-A-V3_8 and
  LEASE-20260808T132428Z-LAB-B-V3_8 in a fresh worktree;
  (4) execute the leaf plan; (5) collect evidence; (6) accept A; (7) request
  exact-head senior merge review; (8) merge via `AUTONOMOUS-MERGE.md`.
- Until that authorization, this session remains in STOP gate.

## Open first

- Re-read this checkpoint end-to-end and confirm:
  (a) all 8 canonical headings present and accurate;
  (b) v3.7 bytes are byte-exact preserved (manifest SHA `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3`);
  (c) v3.8 payload manifest SHA `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18` matches the bytes;
  (d) cross-manifest 59 sources check = 0 mismatches;
  (e) three senior verdicts: accept / accept / continue, all VALID_ADVISORY_VERDICT,
  0 blocking each.
- Re-verify the single-writer lock is still held by PID 1915047 before any
  subsequent action.
- Re-verify PR #34 head is still `3090f2e078ca29762f7654b9e5a872f58ad4ac5b` before any subsequent action.

## Safe reference point

- V3.7 preserved exactly: `/home/ubuntu/.hermes/runs/20260807T161935Z-v3-7-runtime-contract-package/receipts/headless-lab-v3.7/` (manifest
  SHA-256 `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3`).
- V3.8 frozen payload: `/home/ubuntu/.hermes/runs/20260808T132428Z-v3-8-identity-coherence-package/frozen-payload/` (manifest SHA-256
  `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`).
- V3.8 receipts: `/home/ubuntu/.hermes/runs/20260808T132428Z-v3-8-identity-coherence-package/receipts/headless-lab-v3.8-validation/`,
  `/home/ubuntu/.hermes/runs/20260808T132428Z-v3-8-identity-coherence-package/receipts/headless-lab-v3.8-review-binding/`.
- V3.8 RUN: `/home/ubuntu/.hermes/runs/20260808T132428Z-v3-8-identity-coherence-package`.
- Previous checkpoint: `CP-067` (immutable).
- PR: #34, head `3090f2e078ca29762f7654b9e5a872f58ad4ac5b`.
