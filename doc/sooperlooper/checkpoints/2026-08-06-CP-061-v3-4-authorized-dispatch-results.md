# CP-061 — V3.4 Authorized LAB-A/LAB-B Dispatch Results (2026-08-06)

Immutable checkpoint. Supersedes CP-060 as current. CP-048..CP-060 remain byte-identical and immutable.

## Objective

Execute the single human-authorized v3.4 LAB-A/LAB-B dispatch (count=1, consumed), receive and
validate R and E from both leaves, create controller-owned receipts, and publish durable evidence
(CP-061). Result: **DISPATCH_RESULT_REJECTED** — no integration candidate set.

## Authorization scope (exact)

- REDISPATCH_AUTHORIZED=true; AUTHORIZED_PACKAGE=v3.4; AUTHORIZED_DISPATCH_COUNT=1 (consumed).
- INTEGRATION_AUTHORIZED=false; D0_D1_D2_AUTHORIZED=false; PR_MERGE_AUTHORIZED=false;
  EVIDENCE_WORKTREE_DELETION_AUTHORIZED=false.
- Package manifest: `63c371ba70491fa546ce4f8d980568440845c74fbb5af3a93502ff9b8c0a4da7` (v3.4,
  frozen, byte-validated 14/14 from blobs at 8aeb9358).

## RUN

- RUN_ID: `20260806T134656Z-v3-4-authorized-lab-dispatch`; controller eddy; fresh branches/worktrees from candidate `509538784afc2b828f2d922f65cf8ca3a39b5ee7`:
  `result/20260806T134656Z-v3-4-authorized-lab-dispatch-lab-a-v3_4`, `result/20260806T134656Z-v3-4-authorized-lab-dispatch-lab-b-v3_4`.
- Leases activated: `LEASE-20260806T020249Z-LAB-A-V3_4`, `LEASE-20260806T020249Z-LAB-B-V3_4`
  (terminal receipts: failed_validation, consumed=true; leases not reusable).

## Results (exact heads)

| Lab | R (implementation result) | E (envelope container) | remote branch |
|---|---|---|---|
| LAB-A | `0fd104ef2187346b8c00014829857e11ee9539ec` | `a30e3c9d888cc55b9d4d7e92bf6250dca4f3ebb3` | `result/20260806T134656Z-v3-4-authorized-lab-dispatch-lab-a-v3_4` (published) |
| LAB-B | `608cefef3a270a2ea6859b8d48ee931c636743f8` (correct implementation commit) | `2906dc4fd77e2d7455060c3248db73aabdeb010b` | not published |

Topology (controller-verified from origin): LAB-A remote==E, E^==R, R^==CANDIDATE, R non-empty,
diff C..R owned-only, diff R..E envelope-only (all PASS). LAB-B FAILS: remote not published;
E^ == 2b774260 != R (two envelope commits); ownership violation (`headless_audio_lab/__init__.py`);
owned `fixtures/synthetic_source` and `fixtures/deterministic_capture` ABSENT from R (left as
untracked .c files); worktree dirty at E; leaf misidentified R/E.

## Status declarations

- CP-060 immutable; v3/v3.1/v3.2/v3.3/v3.4 packages unmodified; CP-048..CP-060 unmodified.
- Leaf-envelope verifier (frozen v3.4): REJECT (LAB-A), REJECT (LAB-B).
- Branch binding: PASS (LAB-A, remote == E); FAIL (LAB-B, no remote branch).
- Controller literal tests (frozen commands, controller checkouts of R): **8/8 exit 0** —
  implementations functionally pass the literal gates; rejection is at envelope/binding level.
- Functional acceptance / full-completion: not reached for either lab (envelope/binding gates failed).
- DISPATCH RESULT: **DISPATCH_RESULT_REJECTED**; dispatch_authorized=consumed; dispatch_count=1.
- integration_authorized=false; D0_D1_D2_authorized=false; no integration performed.
- Evidence/leaf/controller-validation worktrees and result branches preserved (no destructive
  cleanup; cleanup requires a later human decision).

## Failure evidence (LAB-A)

Topology PASS; leaf-envelope verifier REJECT: envelope `leaf_literal_tests` records
`"exit code": -2` placeholders (wrong key name with space; tests not actually executed) instead of
`exit_code == 0` for ft-a1..ft-a4. Leaf also reset and force-pushed its result branch
(37f8ecb8 -> a30e3c9d), violating "publish once".

## Failure evidence (LAB-B)

Not publishable: remote branch absent; E^ != R (two envelope commits 2b774260, 2906dc4f); diff
C..R includes a LAB-A owned path (`tests/integration/headless_audio_lab/__init__.py`); owned
fixture files missing from R; leaf worktree dirty at E; leaf reported wrong R/E.

## Completed

- Remote revalidation PASS (PR #34 open/draft/mergeable, head 8aeb9358; CI success on 8aeb9358;
  CP-060 present; CURRENT -> CP-060).
- Byte-exact frozen package export (14/14) + review bindings + CP-060/CURRENT (read-only).
- Predispatch: JSON 17, selftest PASS, functional-continuity PASS, validate-project-control PASS
  (on PR head; fork-main shows a pre-existing historical CP-048 heading gap).
- Worktree inventory (14 incl. main) and freshness checks (branches/worktrees absent, PASS).
- Two fresh LAB worktrees created from candidate; leases activated; two leaves dispatched once.
- Controller topology/envelope/binding validation for both labs; controller literal re-execution 8/8;
  terminal lease receipts; dispatch verdict REJECTED; evidence published (20 receipts) + CP-061.

## Verification

- `controller_validate.py` (topology + leaf-envelope verifier + branch binding + controller checkouts).
- `controller_literal_only.py` (frozen literal tests on R checkouts, 8/8 exit 0).
- verifier v3.4 frozen bytes; manifest 63c371ba...; envelope schema file authoritative.
- validate-project-control PASS (evidence head); PR #34 exact-head CI to be confirmed on the new head.

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task P5-005 in_progress; P5-007 deferred.
- `v3_4_status=IMMUTABLE_REJECTED_FOR_DISPATCH` (previous) — package unchanged; dispatch result REJECTED.
- No integration candidate set; leases v3.4 consumed.

## Risks and unresolved questions

- Both leaves' implementations pass the frozen literal tests, but envelope/binding discipline
  failed: LAB-A envelope recorded placeholder test results; LAB-B never published a valid C->R->E chain.
- Root cause hypothesis (not proven): leaves executed the envelope contract partially and did not
  re-run the verifier successfully before pushing. A corrected future dispatch must require a
  verifier PASS receipt from the leaf BEFORE any push.

## Next executable action

- Human decision: review the exact failure evidence; decide a future correction (new authorized
  dispatch with corrected leaf process, or package correction if a verifier/schema defect is confirmed).

## Open first

- `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/dispatch-verdict.json`
- `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/topology-a.json` and `topology-b.json`
- `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/lease-terminal-*.json`
- `doc/sooperlooper/checkpoints/CURRENT.md`

## Safe reference point

- PR #34 head 8aeb9358 (v3.4 evidence container, unchanged); new evidence commit adds
  `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/`, CP-061 and CURRENT.
