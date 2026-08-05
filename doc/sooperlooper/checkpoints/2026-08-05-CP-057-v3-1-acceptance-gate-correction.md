# CP-057 — V3.1 Acceptance-Gate Correction (2026-08-05)

Immutable checkpoint. Supersedes CP-056 as current. CP-048..CP-056 remain byte-identical and immutable.

## Objective

Correct the v3 headless-lab acceptance-gate defect (LAB-A foreign-process contradiction) with a
completely NEW package version v3.1, re-review the exact bytes, publish durable binding evidence,
and record the stop gate — WITHOUT dispatching, creating LAB execution worktrees, activating
leases, implementing the lab, modifying the v3 package, or executing D0/D1/D2.

## Completed

- Identities revalidated: PR #34 OPEN/draft/MERGEABLE; remote head == `9678ff5070b560e073070a2aa2bf8d1e79907f9e`
  (EXPECTED_PR_HEAD); `Project control plane` run 31024861279 SUCCESS and `Audio integration core`
  run 31024859216 SUCCESS, both `head_sha == 9678ff50...`.
- `66f2f0c8..9678ff50` diff contains ONLY CP-056, CURRENT, and `receipts/headless-lab-v3-review-binding/*`;
  `receipts/headless-lab-v3/` byte-identical between the two heads (13/13).
- v3 preserved read-only from tracked blobs (26 artifacts incl. CP-055/CP-056/CURRENT); 12/12 v3
  artifact hashes match `v3-package-manifest.json`; manifest SHA-256 `2114aa70...` confirmed.
- LAB-A defect reproduced from exact bytes (`acceptance-verifier-v3.py`): probe creates `foreign`
  via `sup.start_process(...)` then requires it to SURVIVE `sup.cleanup(timeout=5.0)` —
  `process_created_by_start_process=owned`, `cleanup_is_required_to_terminate_owned_processes=true`,
  `v3_probe_expects_that_owned_process_to_survive_cleanup=true`, `semantic_contradiction=true`.
  Classified `V3_ACCEPTANCE_GATE_DEFECT`, severity `blocking_for_dispatch`,
  affected_control `no_signaling_of_foreign_processes`. No malice/fraud claim.
- Controlled experiments on the controller-owned fixture: owned child terminated by cleanup
  (`owned_child_alive_after_cleanup=false`); genuinely foreign process (`subprocess.Popen`,
  `start_new_session=True`, unregistered) SURVIVES supervisor cleanup
  (`foreign_child_alive_after_supervisor_cleanup=true`) and is torn down via its Popen object
  (TERM, bounded wait, KILL exact fallback); v3 probe pattern against the correct fixture makes
  the v3 gate fail (`v3_gate_would_pass_with_this_fixture=false`) — contradiction demonstrated.
- v3.1 package created under `receipts/headless-lab-v3.1/` (13 artifacts, NEW IDs and NEW hashes):
  batch `BATCH-20260805T165044Z-DOGFOOD004-LAB-V3_1`; leases
  `LEASE-20260805T165044Z-LAB-A-V3_1` / `LEASE-20260805T165044Z-LAB-B-V3_1` (planned, NOT activated);
  supersedes package v3; supersession reason = LAB-A foreign-process acceptance-gate contradiction.
- Corrected LAB-A probe: foreign process created OUTSIDE ProcessSupervisor via
  `subprocess.Popen(start_new_session=True)`; owned via `start_process`; cleanup must terminate all
  and only owned processes; foreign survival tested separately; teardown via Popen in try/finally.
- `acceptance-verifier-v3.1.py --selftest`: **PASS** (11/11: known-bad LAB-A placeholder rejected,
  known-bad LAB-B trivial tests rejected, known-good LAB-B test accepted, v3 defective pattern
  DETECTED, v3.1 corrected pattern ACCEPTED, external process survives supervisor cleanup, owned
  process does not survive cleanup, all temporary children cleaned).
- Rejected-head test on clean checkout of `0769d150...` (REJECTED_LAB_B_HEAD): **REJECT** both labs
  (LAB-A import failure; LAB-B placeholder tests + broken `.gitignore`).
- Controlled ownership probe (corrected probe vs controller fixture): **PASS** (8/8 conditions).
- v3.1 package validators: **14/14 PASS** (JSON parse, scope, batch-manifest, dispatch-plan, port-plan,
  lease, ownership overlap, no-globs, forbidden branch/worktree reuse, envelope schema,
  build-provenance policy, predispatch readiness, v2/v3-ID reuse scan, harness token scan).
- v3.1-package-manifest validation: **PASS** 12/12 files, 12/12 sizes, 12/12 SHA-256, zero missing,
  zero extra in frozen set, candidate binding exact, new batch exact.
- Senior plan-review (`seq66-lab-v3-1-corrected-acceptance-package`): preflight VALID (0 model
  processes); consult **accept** — bound to PR head `9678ff50...`, v3.1 package manifest SHA-256
  `d6ebac8ea127a93f7f582b98b2436f60f3b1fc0b6dbbaad2760d89306186ac48` and all twelve artifact hashes;
  0 blocking findings, 0 required actions; accept does NOT authorize dispatch.
- Durable evidence published under `receipts/headless-lab-v3.1-review-binding/` (indexed).
- v3 package under `receipts/headless-lab-v3/` **UNCHANGED**.

## Verification

- `git rev-parse HEAD` in evidence worktree == `9678ff50...`; clean worktree before staging;
  owned-path-only diff (receipts/headless-lab-v3.1/*, receipts/headless-lab-v3.1-review-binding/*,
  CP-057, CURRENT, minimal validator-required control changes only).
- v3.1 package manifest SHA-256: `d6ebac8ea127a93f7f582b98b2436f60f3b1fc0b6dbbaad2760d89306186ac48`.
- 12 v3.1 artifact SHA-256 verified from package bytes (list in `tracked-v3.1-hashes.json`).
- `git diff --check` PASS; JSON parse PASS; 14/14 validators PASS; selftest PASS; controlled probe
  PASS; rejected-head test REJECT; `python3 contrib/scripts/validate-project-control.py` PASS.
- Senior response fingerprint: `3b8839d7c154a958ccc933e8d5ba448c290ce1c380abe51f55e8e9c48d792e82`;
  execution id `9f8e508f-4944-4c88-be2e-559e5e6585aa`; verdict **accept**.
- Exact-head CI on the NEW PR head: Project control plane SUCCESS + Audio integration core SUCCESS.

## Current state

- **CP-056 immutable.** v3 exact-byte review remains valid HISTORICAL evidence; v3 REJECTED for
  dispatch due to the LAB-A harness contradiction; v3 package **unmodified**.
- **v3.1 package created** with new IDs/hashes; foreign process now created OUTSIDE
  ProcessSupervisor; owned cleanup and foreign survival tested separately.
- v3.1 selftest **PASS**; rejected LAB-B head (`0769d150...`) **REJECT**; senior v3.1 exact-byte
  verdict **accept** (0 findings, 0 required actions).
- **v3.1 frozen but NOT dispatched.** Leases v3.1 planned (not activated); **zero** v3.1 worktrees;
  **zero** v3.1 leaves; D0/D1/D2 NOT executed.
- `active_task = P5-005` remains `in_progress`; `P5-007` remains `deferred`.
- Human redispatch decision **pending** (`REDISPATCH_AUTHORIZED=false`).

## Risks and unresolved questions

- The senior accept binds the exact v3.1 bytes; any post-freeze mutation invalidates it.
- Accept does NOT authorize dispatch; the mandatory next step is the human dispatch decision.
- Hash equality/difference never proves copy or rebuild; only a controlled rebuild from canonical
  sources with captured commands/environment/toolchain/timestamps demonstrates provenance.
- A future regression to the v3 probe pattern would be caught by the pattern detector + selftest.

## Next executable action

Human decision: **authorize or reject the new LAB-A/LAB-B dispatch** using EXCLUSIVELY the exact
and reviewed v3.1 package, new branches, new worktrees and new leases. Until then:
`HUMAN_DECISION_PENDING`, `v3_status=IMMUTABLE_REJECTED_FOR_DISPATCH`,
`v3_1_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED` — no dispatch, no leases, no D0/D1/D2.

## Open first

`doc/sooperlooper/checkpoints/CURRENT.md` → CP-057. Previous: CP-056.

## Safe reference point

- v3 package: `receipts/headless-lab-v3/` at PR head `9678ff50...` (unchanged since `66f2f0c8...`).
- v3.1 package: `receipts/headless-lab-v3.1/` (frozen, reviewed, NOT dispatched).
- v3.1 review binding: `receipts/headless-lab-v3.1-review-binding/` (indexed, sha256 per file).
- CANDIDATE_HEAD `50953878...`; REJECTED_LAB_B_HEAD `0769d150...`; V3_REVIEWED_HEAD `66f2f0c8...`;
  EXPECTED_PR_HEAD `9678ff50...`; evidence container head: resolved externally after push.
