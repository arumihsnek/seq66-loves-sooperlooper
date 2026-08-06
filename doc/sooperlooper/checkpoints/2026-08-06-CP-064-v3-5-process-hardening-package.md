# CP-064 — V3.5 Process Hardening Package (2026-08-06)

Immutable checkpoint. Supersedes CP-063 as current. CP-048..CP-063 remain byte-identical and immutable.

## Objective

Prepare (NOT execute) the v3.5 process-hardening package: R-only leaf model, mechanical controller
finalization, non-circular exact-byte package review. Human authorization covered ONLY package
preparation; dispatch/integration/D0-D2 remain forbidden.

## Completed

- Revalidated GitHub state: PR #34 open/draft/mergeable, head `129e5b50` == EXPECTED_PR_HEAD;
  CURRENT -> CP-063; CP-048..CP-063 immutable; packages v3..v3.4 immutable (v3.4-dispatch leaf
  reports differ only by the documented CP-061 amendment, both generations preserved).
- Forensic closure: runs 31128838866 (Project control plane) and 31128838865 (Audio integration
  core) both completed+success on `129e5b50` -> **FORENSIC_CLOSURE_FINAL=PASS**.
- Failure model reconstructed from CP-061 receipts (LAB-A: `exit code` key drift, -2 placeholders,
  invalid envelope, publication pre-PASS, force-push; LAB-B: R misidentified, two envelope commits,
  E^!=R, out-of-scope LAB-A path, owned fixtures absent, untracked .c, dirty worktree, branch not
  published, invalid build claim). No fraud/malice attributed.
- New IDs frozen: `BATCH-20260806T222325Z-DOGFOOD004-LAB-V3_5`; leases
  `LEASE-20260806T222325Z-LAB-A-V3_5` / `LEASE-20260806T222325Z-LAB-B-V3_5` (status=planned,
  activated=false, authorization=false); result paths
  `receipts/headless-lab-results-v3.5/lab-a-controller-evidence.json` /
  `.../lab-b-controller-evidence.json`; planned branches `result/<future-run-id>-lab-{a,b}-v3_5`.
- 17 operational artifacts in `receipts/headless-lab-v3.5/` + `package-payload-manifest-v3.5.json`
  (directory=18; manifest self-excludes; review artifacts excluded; zero missing/extra).
- **V3_5_PACKAGE_PAYLOAD_MANIFEST_SHA256=`31b3fd5518153727bc51785553b0dd3ab9223f957ec5a2d270eca90186e946f6`**.
- Functional continuity vs v3.4: modules/functions/classes/methods/signatures/returns/silence
  semantics (verification=FAIL)/ownership/literal IDs/canonical sources/gcc commands all
  DEEP-EQUAL; `functional-continuity.json` all_deep_equal=true.
- Mechanical finalizer `mechanical-finalizer-v3.5.py`: sole mechanism creating E; verifies
  R^==C, owned-only, required paths, evidence absent, clean detached checkout, zero untracked,
  zero tracked ELF; runs literal tests + functional gates + artifact hashing + LAB-B controlled
  build; creates exactly one E (E^==R, diff R..E evidence-only); `--reproduce` regenerates
  byte-identical evidence (no commit).
- Verifier `acceptance-verifier-v3.5.py`: phases functional-continuity / r-only-leaf-result /
  prepublish-finalization / postpublish-binding / full-completion.
- Selftest: 20/20 PASS in real temp git repos (valid R-only A/B, finalization A/B with real gcc,
  create-only publication; REJECT for empty R, wrong/multi parents, out-of-scope, missing paths,
  evidence-in-R, tracked ELF, leaf key `exit code`, evidence forbidden/missing fields, silent
  substitution, wrong test ID, E^!=R, extra envelope, preexisting branch, force-push,
  acceptance-in-E, remote!=E).
- Negative matrix: all negative cases real REJECT (not declarative); reproductions of every v3.4
  failure anchored. Evidence under `receipts/headless-lab-v3.5-validation/` (selftest-results,
  negative-matrix, functional-continuity, finalizer-reproducibility, controlled-build,
  publication-create-only, package-validation).
- Controlled build LAB-B: gcc -std=c11 -O2 from EMPTY build dir, 2/2 exit 0, real ELF
  (synthetic_source 71864 B, deterministic_capture 72408 B), ldd libs captured, zero tracked ELF.
- Dual senior reviews: functional `seq66-lab-v3-5-functional-continuity` **accept 0/0** (exec
  `8411b59c-20c7-408c-b81d-58805b2f90a6`, fp `4b2e444f...`); process
  `seq66-lab-v3-5-r-only-mechanical-finalization` **accept 0/0** (exec
  `c5021990-001c-46f4-8a5e-5f0e6e57085c`, fp `7c082bcd...`). Both bound manifest `31b3fd55...`
  + 17 hashes. Accept does NOT authorize dispatch.
- Non-circular review binding under `receipts/headless-lab-v3.5-review-binding/`
  (functional/process preflights, bundles, verdicts, raw; `package-review-binding-v3.5.json`
  created_after_reviews=true, self_reviewed=false, technical_dispatch_eligibility=PASS,
  dispatch_authorized=false; `package-publication-index-v3.5.json` self-exclusion explicit).

## Verification

- package-validation.json: 17/17 files, 17/17 sizes, 17/17 hashes, zero missing/extra, JSON parse OK.
- selftest exit 0 (20/20); finalizer-reproducibility: LAB-A/LAB-B verifier prepublish exit 0
  (evidence bytes == blob bytes in E).
- controlled-build.json: all_exit_zero true.
- Functional continuity all_deep_equal true; ownership intersection empty.
- `git diff` on v3.4 package dirs vs their add-commits: empty (immutability).

## Current state

- `v3_5_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`
- `technical_dispatch_eligibility=PASS`; `dispatch_authorized=false`
- `integration_authorized=false`; `D0_D1_D2_authorized=false`; `PR_merge_authorized=false`
- leases=planned; leases_activated=false; lab_worktrees=0; leaves=0
- `forensic_closure_final=PASS`; `dispatch_result=DISPATCH_RESULT_REJECTED`;
  `no_integration_candidate_set=true`
- CI on the v3.5 evidence commit: observed after push (see exact-head CI section).

## Risks and unresolved questions

- Exact-head CI on the new commit is observed after push; a GitHub external outage would yield
  `V3_5_PACKAGE_CI_PENDING_EXTERNAL` (eligibility=PENDING_CI, dispatch_authorized=false).
- Any gate failure => `V3_5_PACKAGE_REJECTED`; a correction requires a new generation (CP-065+).
- Human decision required: authorize or reject one fresh v3.5 LAB-A/LAB-B dispatch.

## Next executable action

Publish evidence worktree (receipts/headless-lab-v3.5/*, v3.5-validation/*,
v3.5-review-binding/*, CP-064, CURRENT) to PR #34 fast-forward; update PR body; observe exact-head
CI; STOP for the human dispatch decision.

## Open first

PR #34 exact head == new evidence head; exact-head CI (Project control plane + Audio integration core).

## Safe reference point

`129e5b50d5f4b2054ada99d6b483a0c5237f2b36` (previous PR head) + package payload frozen read-only
(17 artifacts + manifest, sha `31b3fd55...`).
