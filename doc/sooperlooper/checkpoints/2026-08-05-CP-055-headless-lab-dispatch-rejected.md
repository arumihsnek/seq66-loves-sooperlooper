# CP-055 — Headless Lab Dispatch Rejected (2026-08-05)

Immutable checkpoint. Supersedes CP-054 as current. CP-048..CP-054 remain byte-identical and immutable.

## Objective

Forensic closure of the FIRST headless-lab LAB-A/LAB-B dispatch
(`20260805T134658Z-headless-lab-leaf-dispatch`), binding the human decision
(`LAB_A_INTEGRATION=REJECTED`, `LAB_B_INTEGRATION=REJECTED`,
`PREVIOUS_DISPATCH_AUTHORIZATION=CONSUMED`, `REDISPATCH_AUTHORIZED=false`),
preserving all failed-dispatch evidence, closing v2 leases, and freezing the
v3 replacement package — WITHOUT dispatching, integrating, or running D0/D1/D2.

## Completed

- First LAB-A/LAB-B dispatch attempted (delegation `deleg_676e13c0`, 2 leaves, parallel).
- LAB-A rejected: no result head, no envelope (placeholder code only; HEAD stayed at candidate).
- LAB-B rejected: exact head `0769d150f4989b8331db8273aa7a771f7b929026`
  (published to origin as evidence-only; ft-b3 FAIL on clean checkout; placeholder tests;
  byte-broken `fixtures/.gitignore`; false provenance receipt; no envelope).
- Integration candidate set: **empty**.
- v2 leases terminal `failed_validation`, `integration_eligible=false`, `reusable=false`
  (`LEASE-20260805T015358Z-LAB-A`, `LEASE-20260805T015358Z-LAB-B`).
- v2 package consumed and not reusable (all 8 v2 artifact hashes re-verified).
- D0/D1/D2 NOT executed; no integration performed; no CP-048..CP-054 edited.
- v3 package frozen under `receipts/headless-lab-v3/` (12 artifacts, new IDs,
  batch `BATCH-20260805T143214Z-DOGFOOD004-LAB-V3`) — **not dispatched**.
- Senior reviews: forensic `accept` (final-review), v3 `accept` (plan-review;
  no dispatch authorization conferred).
- Evidence commit pushed to PR #34 (draft) without history rewrite.

## Verification

- Source revalidation PASS: PR #34 open/draft/mergeable, head = receipt container
  `32494cf0…`; `result/lab-a-dogfood004-recovery` remote ABSENT; `result/lab-b-dogfood004-recovery`
  remote = `0769d150…`; `merge-base --is-ancestor candidate→lab-b` OK; rev-list count = 1.
- Preservation gate PASS (NUL-safe manifest + sha256 + tar of the failed dispatch run).
- Independent LAB-B verification: ft-b1 PASS, ft-b2 PASS, ft-b3 FAIL (TypeError),
  ft-b4 PASS; negative gates confirm placeholders, broken `.gitignore`, no ELF tracked.
- Controlled rebuild: fresh `gcc 13.3.0` build from canonical sources produces hashes
  identical to quarantine/LAB-B (`054b8428…`, `fa3874b1…`) → deterministic → equality
  inconclusive; leaf receipt false (claims gcc 11.4.0/03:24:18) → `provenance_not_demonstrated`.
- v3 validation: JSON parse PASS (12/12), contract validators PASS, verifier selftest PASS,
  live reject-test vs `0769d150` REJECT as expected, v2-ID reuse scan PASS.
- `validate-project-control.py` on the new exact head: PASS (see CI section).
- Exact-head CI on new PR head: `validate-control-plane` SUCCESS + `compile-and-test` SUCCESS.

## Current state

- `active_task = P5-005` remains `in_progress` (headless lab recovery is the executing
  workstream for P5-005; the failed dispatch does not change its status).
- `P5-007` remains `deferred` (Raspberry Pi validation).
- v2 package: consumed. v2 leases: terminal failed_validation. v3 package: frozen,
  not dispatched. Redispatch: not authorized.
- Worktrees `lab-a-dogfood004-recovery` and `lab-b-dogfood004-recovery` preserved
  (not removed, not repaired, no destructive ops). LAB-B branch `result/lab-b-dogfood004-recovery`
  preserved (no force-push, no delete).

## Risks and unresolved questions

- v2 acceptance lacked envelope/negative-gate enforcement; v3 addresses all nine gap
  classes but has not been exercised by a real dispatch.
- `0769d150` CI: no check runs recorded (rejected head never ran CI).
- Build environment for future ft-b4 requires gcc + libjack headers (present on this host).
- Human redispatch authorization is the only missing input for a future LAB-A/LAB-B dispatch.

## Next executable action

Human decision: **authorize or reject a NEW LAB-A/LAB-B dispatch** using exclusively
the exact v3 package (`receipts/headless-lab-v3/`), new branches, new worktrees and
new leases. Until then: `HUMAN_DECISION_PENDING`, no dispatch, no leases, no D0/D1/D2.

## Open first

`doc/sooperlooper/audits/2026-08-05-headless-lab-leaf-dispatch-failure.md`

## Safe reference point

- candidate_head = `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- verified_evidence_head = `2dff9052d3e94048b302eb60d9c7a12373b0229e`
- receipt_container_head = `32494cf00b446142fe24effd149894453b3b3ddd`
- rejected_lab_b_head = `0769d150f4989b8331db8273aa7a771f7b929026`
- PR #34 head after closure push: documented in PR body and closure RUN
  (`20260805T143214Z-failed-lab-dispatch-closure`) post-push.
