# CP-058 — V3.2 Envelope-Binding Correction (2026-08-05)

Immutable checkpoint. Supersedes CP-057 as current. CP-048..CP-057 remain byte-identical and immutable.

## Objective

Correct the v3.1 result-envelope binding defect (same-commit envelope/tree-OID circularity) with a
completely NEW package version v3.2 using the TWO-COMMIT model (C→R→E per leaf), re-review the exact
bytes, publish durable binding evidence, and record the stop gate — WITHOUT dispatching, creating LAB
execution worktrees, activating leases, implementing the lab, modifying the v3/v3.1 packages, or
executing D0/D1/D2.

## Completed

- Identities revalidated: PR #34 OPEN/draft/MERGEABLE; remote head == `da89654e05d7014bf0720ed5517c362b7c4beacf`
  (EXPECTED_PR_HEAD); `Project control plane` run 31028638773 SUCCESS and `Audio integration core`
  run 31028638751 SUCCESS, both `head_sha == da89654e...`.
- `9678ff50..da89654e` diff contains ONLY CP-057, CURRENT, `receipts/headless-lab-v3.1/*` and
  `receipts/headless-lab-v3.1-review-binding/*`; `receipts/headless-lab-v3/*` and
  `receipts/headless-lab-v3-review-binding/*` byte-identical across heads.
- v3.1 preserved read-only from tracked blobs (30 artifacts); 12/12 v3.1 hashes match
  `v3.1-package-manifest.json` (manifest SHA-256 `d6ebac8e...`).
- v3.1 envelope defect reproduced from exact bytes (`result-envelope-schema-v3.1.json` +
  `acceptance-verifier-v3.1.py`): envelope required in SAME result commit; `result_tree_sha`
  required to equal `result_head^{tree}`; the envelope blob is an INPUT of `HEAD^{tree}` →
  `hash_dependency_cycle=true`; schema labels `result_tree_sha` as SHA-256 while the verifier
  obtains a Git object ID via `rev-parse` → `algorithm_semantics_ambiguous=true`.
  Classified `V3_1_ENVELOPE_BINDING_DEFECT`, `blocking_for_dispatch=true`,
  `affected_control=result-envelope-binding`; `operationally_non_constructible_under_normal_git_workflow=true`,
  `cryptographic_fixed_point_would_be_required=true` (no absolute-impossibility claim).
- Controlled Git experiment (no LAB worktrees): T1=`bc726eff...` → envelope declaring T1 → T2=`5646f00e...`
  → envelope declaring T2 → T3=`6798adea...`; `T1 != T2 != T3`, `same_commit_binding_not_stable=true`.
- v3.2 package created under `receipts/headless-lab-v3.2/` (13 artifacts, NEW IDs and NEW hashes):
  batch `BATCH-20260805T224640Z-DOGFOOD004-LAB-V3_2`; leases
  `LEASE-20260805T224640Z-LAB-A-V3_2` / `LEASE-20260805T224640Z-LAB-B-V3_2` (planned, NOT activated);
  supersedes v3.1; supersession reason = same-commit envelope/tree-OID circularity.
- TWO-COMMIT MODEL: C = candidate; R = implementation result head (parent(R)=C, NO envelope);
  E = envelope container head (parent(E)=R, EXACTLY the frozen envelope); remote branch → E;
  ONLY R is integration-eligible; E is evidence-only.
- v3.2 envelope schema: non-self-referential fields `candidate_commit_oid`, `implementation_result_commit_oid`,
  `implementation_result_tree_oid`, `git_object_format`; forbidden self-binding (E commit/tree/blob OID,
  envelope SHA-256, resolved remote SHA) recorded externally after push; `_sha256` reserved for file hashes.
- `acceptance-verifier-v3.2.py --selftest`: **PASS** (15/15, real temporary git repositories, no network):
  v3.1 same-commit model rejected; C→R→E accepted; wrong candidate/result/tree OIDs rejected; E parent != R
  rejected; envelope in R rejected; implementation change in E rejected; missing envelope in E rejected;
  self container commit field rejected; self tree field rejected; envelope self-SHA256 rejected;
  remote branch mismatch rejected; correct external branch binding accepted (bare origin + `git ls-remote`);
  zero residual directories.
- Valid C→R→E repository (controller-built, contract-conformant supervisor): verifier v3.2 **PASS** (22 checks).
- Invalid fixtures: same-commit envelope **REJECT**; wrong tree OID **REJECT**; implementation change in E **REJECT**.
- Rejected-head test on clean checkout of `0769d150...` (REJECTED_LAB_B_HEAD): **REJECT** both labs (exit 1).
- v3.2 package validators: **15/15 PASS** (incl. old-ID reuse scan + self-reference field scan).
- v3.2-package-manifest validation: **PASS** 12/12 files, sizes, SHA-256; zero missing; zero extra;
  candidate exact; batch exact; supersedes v3.1 declared.
- Senior plan-review (`seq66-lab-v3-2-two-commit-envelope-package`): preflight VALID (0 model processes);
  consult **accept** — bound to PR head `da89654e...`, v3.2 package manifest SHA-256
  `441d04d4eb812b5c2738372205ae244691386cec2c2c4fe8011889486bf9ec27` and all twelve artifact hashes;
  0 blocking findings, 0 required actions; accept does NOT authorize dispatch.
- Durable evidence published under `receipts/headless-lab-v3.2-review-binding/` (indexed).
- v3/v3.1 packages under `receipts/headless-lab-v3/` and `receipts/headless-lab-v3.1/` **UNCHANGED**.

## Verification

- `git rev-parse HEAD` in evidence worktree == `da89654e...`; clean worktree before staging;
  owned-path-only diff (receipts/headless-lab-v3.2/*, receipts/headless-lab-v3.2-review-binding/*,
  CP-058, CURRENT, minimal validator-required control changes only).
- v3.2 package manifest SHA-256: `441d04d4eb812b5c2738372205ae244691386cec2c2c4fe8011889486bf9ec27`.
- 12 v3.2 artifact SHA-256 verified from package bytes (list in `tracked-v3.2-hashes.json`).
- `git diff --check` PASS; JSON parse PASS; 15/15 validators PASS; selftest PASS (15/15); valid C→R→E PASS;
  invalid fixtures REJECT; rejected-head test REJECT; `python3 contrib/scripts/validate-project-control.py` PASS.
- Senior response fingerprint: `3bf4941861cdcd4bcde356dbc0bc5c266c8993e4c911fc4c62e344b867e236cf`;
  execution id `a4a74e23-b260-48ee-ba82-9b358966ec4f`; verdict **accept**.
- Exact-head CI on the NEW PR head: Project control plane SUCCESS + Audio integration core SUCCESS.

## Current state

- **CP-057 immutable.** v3.1 exact-byte review remains valid HISTORICAL evidence; v3.1 REJECTED for
  dispatch due to the envelope/tree-OID cycle (`V3_1_ENVELOPE_BINDING_DEFECT`); v3.1 package **unmodified**.
- **v3.2 package created** with the two-commit C→R→E model; only R is integration-eligible; E is
  evidence-only; selftest PASS (15/15); invalid same-commit model REJECT; rejected LAB-B head REJECT;
  senior v3.2 verdict **accept** (0 findings, 0 required actions).
- **v3.2 frozen but NOT dispatched.** Leases v3.2 planned (not activated); **zero** LAB worktrees;
  **zero** leaves; D0/D1/D2 NOT executed.
- `active_task = P5-005` remains `in_progress`; `P5-007` remains `deferred`.
- Human redispatch decision **pending** (`REDISPATCH_AUTHORIZED=false`).

## Risks and unresolved questions

- The senior accept binds the exact v3.2 bytes; any post-freeze mutation invalidates it.
- Accept does NOT authorize dispatch; the mandatory next step is the human dispatch decision.
- The controlled Git experiment demonstrates the ordinary-procedure circularity; it is not a formal
  proof over the whole hash space.
- A future regression to the same-commit envelope model would be caught by the model checks + selftest.
- External branch-binding receipts are created at dispatch time after push (never inside the envelope).

## Next executable action

Human decision: **authorize or reject the new LAB-A/LAB-B dispatch** using EXCLUSIVELY the exact and
reviewed v3.2 package, new branches, new worktrees and new leases. Until then:
`HUMAN_DECISION_PENDING`, `v3_1_status=IMMUTABLE_REJECTED_FOR_DISPATCH`,
`v3_2_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED` — no dispatch, no leases, no D0/D1/D2.

## Open first

`doc/sooperlooper/checkpoints/CURRENT.md` → CP-058. Previous: CP-057.

## Safe reference point

- v3 package: `receipts/headless-lab-v3/` (unchanged since `66f2f0c8...`).
- v3.1 package: `receipts/headless-lab-v3.1/` (unchanged since `9678ff50...`; historical evidence).
- v3.2 package: `receipts/headless-lab-v3.2/` (frozen, reviewed, NOT dispatched).
- v3.2 review binding: `receipts/headless-lab-v3.2-review-binding/` (indexed, sha256 per file).
- CANDIDATE_HEAD `50953878...`; REJECTED_LAB_B_HEAD `0769d150...`; V3_REVIEWED_HEAD `66f2f0c8...`;
  v3.1 evidence head `9678ff50...`; EXPECTED_PR_HEAD `da89654e...`; evidence container head: resolved
  externally after push.
