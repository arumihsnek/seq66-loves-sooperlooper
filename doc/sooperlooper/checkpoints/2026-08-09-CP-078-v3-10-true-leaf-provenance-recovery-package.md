# Checkpoint CP-078 — V3.10 true-leaf provenance recovery package

Checkpoint ID: `CP-078`
Checkpoint date: 2026-08-09
Supersedes: `CP-077` (CP-077 remains immutable historical evidence)
Phase: `phase-5-exact-recording`
Current phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — V3.10 provenance package)
Status: `EXACT_BYTES_REVIEWED_NOT_DISPATCHED — HUMAN_DECISION_PENDING`

## Objective

Prepare, byte-freeze, independently review, and publish a V3.10 package that
makes true-leaf actor provenance fail-closed without executing leaves, activating
leases, changing product implementation, integrating R, deleting result branches,
or authorizing dispatch. Functional semantics remain those of the V3.9 package.

## Completed

- PR #34 and PR #35 remain historical physical merges. fork-main was verified at
  `c965340f6be7655e2f4d796582e5acd7dc163a82` before package
  preparation; PR #34 merge is `2fbd2ba5ca9b5f4c599d0c6c3c61771732043f5d` and
  PR #35 merge is `c965340f6be7655e2f4d796582e5acd7dc163a82`.
- CURRENT@fork-main was CP-077. The two V3.9 result branches were preserved and
  not modified: LAB-A E=`7db1a1b7572eabf6913ad8219a7647adca4f15c8`, LAB-B
  E=`71a630659310782940705f484104465ad1c2308a`.
- V3.9 historical classification was preserved: functional R validity PASS;
  workflow actor provenance FAIL; product integration FAIL; result publication
  PASS; finding `WF-F1_ACTOR_PROVENANCE_NOT_ENFORCED`. R_A/R_B/E_A/E_B remain
  evidence only and are not V3.10 inputs.
- V3.9 manifest `71596b8472b459940ee79fe9df8d3687dd3dbbdf5bf4696d897a9e4a1bdb3bba`
  and all listed historical evidence were preserved byte-exactly. Old V3.9
  batch/leaf/lease identities are already used and fresh lease reuse is unsafe.
- V3.10 fresh identities were generated: batch
  `BATCH-20260809T205317Z-DOGFOOD004-LAB-V3_10`; leaves
  `LEAF-LAB-A-V3_10` / `LEAF-LAB-B-V3_10`; leases
  `LEASE-20260809T205317Z-LAB-A-V3_10` /
  `LEASE-20260809T205317Z-LAB-B-V3_10`; future result paths are
  `receipts/headless-lab-results-v3.10/lab-{a,b}-controller-evidence.json` and
  future branches are `result/<future-run-id>-lab-{a,b}-v3_10`.
- Leases are `planned`, `activated=false`, `authorization=false`,
  `consumed=false`; leaves executed=0; V3.10 result branches=0.
- Scope freeze contains only fresh identities, true-leaf provenance contract and
  evidence/finalizer gates, controller non-intervention, and the historical V3.9
  invalid-dispatch binding. No product or functional implementation drift.
- Normative principle added: **THE LEAF MUST CREATE THE FINAL IMPLEMENTATION TREE
  AND COMMIT R. THE CONTROLLER MAY VALIDATE R BUT MAY NOT COMPLETE R.** Controller
  implementation edits, git add, commit/amend, cherry-pick/fixup/rebase are
  forbidden in a leaf worktree.
- Fail-closed termination rule added: max_iterations, timeout, crash, tool
  failure, no final report, no commit, or dirty worktree yields
  `LEAF_FAILED_TO_PRODUCE_R`; lease consumed=true and result FAIL; controller
  rescue and same-authorization replacement are forbidden.
- Added transcript-bound verifier, provenance finalizer, receipt schema,
  controller non-intervention schema, AP-01..AP-08 traceability, real temporary
  Git selftests P1 and N1-N8, and CP-077 controller-as-leaf replay. P1 PASS;
  N1-N8 all REJECT for intended provenance failures.
- Functional semantic diff was verified as zero: literal ft-a1..ft-a4 and
  ft-b1..ft-b4 argv/hashes, ownership, canonical sources/modules, compile
  commands, silence semantics, controlled build, runtime and publication gates
  remain unchanged. `functional_semantic_drift=0`.
- Payload freeze: 27 files, raw manifest SHA-256
  `4ca2f2686f36e5b3455d2ec4bb6b5edb4e22099e1e1b13f0cfbb4899160c550d`;
  `PAYLOAD_FROZEN=true`. Any later payload byte change is
  `V3_10_FROZEN_PACKAGE_REJECTED` and STOP; next generation would be V3.11/CP-079+.
- Triple exact-byte senior reviews all accepted with blocking=0 and required=0:
  - functional continuity: exec `bf45c5a4-5865-497b-860a-2e50eaf9d29b`;
  - workflow/actor provenance: exec `dfc78070-cba5-4275-b7cc-ca612901aba7`;
  - adversarial validity: exec `bd215327-45e3-4edf-b292-b8371e554a21`.
- Non-circular review binding created after all three reviews:
  `created_after_reviews=true`, `self_reviewed=false`, accept_count=3,
  blocking=0, required=0, technical_dispatch_eligibility=PASS,
  dispatch_authorized=false.

## Verification

- V3.9 preservation receipt: 57 historical files hashed from the exact fork-main
  tree; R_A/R_B/E_A/E_B preserved as evidence only.
- Fresh identity and old-lease non-reuse receipts PASS; no V3.9 lease or result
  branch was reused or modified.
- Actor verifier was exercised through its executable CLI against real temporary
  Git repositories: P1 true leaf commit PASS; N1 controller-only commit, N2
  rescue, N3 amend, N4 dirty/no commit, N5 fake LEAF identity, N6 transcript
  hash mismatch, N7 wrong R, and N8 post-termination R all REJECT.
- Gate registry validation PASS: AP-01..AP-08 each bind normative requirement,
  leaf-visible requirement, verification method, positive fixture, and negative
  fixture; orphan_gates=[]
- Python syntax validation passed for package scripts. Payload manifest verified
  27/27 file hashes and sizes, zero missing, zero extra.
- Triple senior responses were `COMPLETED/VALID_ADVISORY_VERDICT`, verdict
  `accept`, blocking=0, required=0, safe_to_merge=true, each bound to the exact
  frozen manifest and all individual payload hashes.

## Current state

- `CURRENT -> CP-078`.
- `V3_10_STATUS=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`.
- `V3_10_TECHNICAL_DISPATCH_ELIGIBILITY=PASS` but
  `V3_10_DISPATCH_AUTHORIZED=false`.
- `V3_9_STATUS=FUNCTIONALLY_PASS_WORKFLOW_PROVENANCE_INVALID_NOT_INTEGRATED`.
- `V3_9_EXECUTION_IDENTITIES_ALREADY_USED=true`;
  `V3_9_FRESH_LEASE_REUSE_SAFE=false`.
- `V3_9_SECOND_DISPATCH_AUTHORIZED=false`;
  `V3_9_LEASE_REUSE_AUTHORIZED=false`.
- `fresh_leases=planned`; `fresh_leases_activated=false`; `leaves=0`;
  `v3_10_result_branches=0`.
- `integration_authorized=false`; `D0_D1_D2_authorized=false`;
  `PR_merge_authorized=false`; `RESULT_BRANCH_DELETION_AUTHORIZED=false`;
  `FORCE_PUSH_AUTHORIZED=false`; `FORK_MAIN_REWIND_AUTHORIZED=false`.
- Planned publication branch: `recovery/v3-10-true-leaf-provenance-20260809T205317Z`
  from exact fork-main; planned PR is draft toward fork-main and contains only
  V3.10 receipts, CP-078, CURRENT, and strictly necessary validator compatibility.

## Risks and unresolved questions

- No leaf execution, lease activation, R creation, result publication, product
  integration, D0/D1/D2 action, or merge was authorized or performed.
- A true-leaf dispatch must independently produce R; the controller cannot rescue
  a failed leaf. Any future provenance violation must fail before E publication.
- Exact-head CI for the future recovery PR is still pending because the branch
  has not yet been published.
- The only unresolved decision is whether to authorize exactly one fresh V3.10
  LAB-A/LAB-B dispatch with true isolated leaves. That authorization excludes
  integration.

## Next executable action

Publish the frozen V3.10 package, validation receipts, review binding, CP-078 and
CURRENT on a new recovery branch from exact fork-main; open a draft PR; require
Project control plane and Audio integration core exact-head CI. Do not dispatch
leaves or integrate implementation.

## Open first

- `receipts/headless-lab-v3.10/**` — frozen 27-file payload and manifest
- `receipts/headless-lab-v3.10-validation/**` — validation evidence
- `receipts/headless-lab-v3.10-review-binding/**` — post-review non-circular binding
- `doc/sooperlooper/checkpoints/2026-08-09-CP-078-v3-10-true-leaf-provenance-recovery-package.md`
- `doc/sooperlooper/checkpoints/CURRENT.md`

## Safe reference point

- Exact fork-main base: `c965340f6be7655e2f4d796582e5acd7dc163a82`.
- Candidate C: `509538784afc2b828f2d922f65cf8ca3a39b5ee7`.
- V3.9 manifest: `71596b8472b459940ee79fe9df8d3687dd3dbbdf5bf4696d897a9e4a1bdb3bba`.
- V3.10 payload manifest: `4ca2f2686f36e5b3455d2ec4bb6b5edb4e22099e1e1b13f0cfbb4899160c550d`.
- CP-077 remains immutable and records the V3.9 provenance/integration failure.
- No V3.10 result branch exists; no V3.9 result branch was modified.
