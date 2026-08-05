# Current checkpoint — Headless lab dispatch rejected (forensic closure)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-05-CP-055-headless-lab-dispatch-rejected.md`

Checkpoint ID: `CP-055`
Checkpoint date: 2026-08-05
Phase: `phase-5-exact-recording`
Active task: `P5-005`
Status: `in_progress`
Branch: `integration/baseline-qualification-20260805`

## Head identity (explicit three-identity chain, no self-reference)

- `candidate_head = 509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- `verified_evidence_head = 2dff9052d3e94048b302eb60d9c7a12373b0229e`
- `receipt_container_head = 32494cf00b446142fe24effd149894453b3b3ddd`
- `rejected_lab_b_head = 0769d150f4989b8331db8273aa7a771f7b929026`
- PR #34 head after this closure: documented in PR body and closure RUN
  (`20260805T143214Z-failed-lab-dispatch-closure`) post-push.

## Dispatch outcome (first LAB-A/LAB-B dispatch)

- Dispatch authorized and attempted (delegation `deleg_676e13c0`, 2 leaves parallel).
- LAB-A rejected: no result head / no envelope (placeholder code, no commit).
- LAB-B rejected: exact head `0769d150…` (ft-b3 FAIL on clean checkout; placeholder
  tests; byte-broken `.gitignore`; false provenance receipt; no envelope).
- Both results rejected; **no integration performed**; D0/D1/D2 not executed.
- v2 leases terminal `failed_validation`; v2 package consumed.
- v3 package frozen (`receipts/headless-lab-v3/`) but **not dispatched**;
  human redispatch authorization **pending**.
- P5-005 remains `in_progress`; P5-007 remains `deferred`.
- Full audit: `doc/sooperlooper/audits/2026-08-05-headless-lab-leaf-dispatch-failure.md`

## Verification summary

- PR #34: OPEN, draft=true, mergeable, head = receipt container (revalidated).
- LAB-B parentage verified (exactly 1 commit ahead of candidate); changed paths = 10 files.
- Independent clean-checkout verification at `0769d150`: ft-b1 PASS, ft-b2 PASS,
  ft-b3 FAIL, ft-b4 PASS; negative gates confirm placeholders and broken `.gitignore`.
- Controlled rebuild: deterministic; hash equality inconclusive; leaf provenance
  receipt false → `provenance_not_demonstrated`.
- v3 validations: JSON parse, contract validators, verifier selftest, reject-test — all PASS.
- Senior reviews: forensic `accept`; v3 `accept` (no dispatch authorization).
- Exact-head CI on the new PR head: Project control plane SUCCESS +
  Audio integration core SUCCESS.

## Next action

Human: authorize or reject a NEW LAB-A/LAB-B dispatch using exclusively the exact
v3 package, new branches, new worktrees and new leases. Until then:
`HUMAN_DECISION_PENDING` — no dispatch, no leases, no D0/D1/D2.
