# Current checkpoint — V3.2 envelope-binding correction

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-05-CP-058-v3-2-envelope-binding-correction.md`

Checkpoint ID: `CP-058`
Checkpoint date: 2026-08-05
Phase: `phase-5-exact-recording`
Active task: `P5-005`
Status: `in_progress`
Branch: `integration/baseline-qualification-20260805`

## Head identity (explicit chain, no self-reference)

- `candidate_head = 509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- `rejected_lab_b_head = 0769d150f4989b8331db8273aa7a771f7b929026`
- `v3_1_evidence_head = 9678ff5070b560e073070a2aa2bf8d1e79907f9e`
- PR #34 head after this evidence commit: documented in PR body and closure RUN
  (`20260805T224640Z-v3-2-envelope-binding-correction`) post-push.

## V3.2 envelope-binding correction status

- **v3.1 immutable / rejected for dispatch**: same-commit envelope/tree-OID circularity
  (`V3_1_ENVELOPE_BINDING_DEFECT`, blocking_for_dispatch). v3.1 exact-byte review remains valid
  HISTORICAL evidence (CP-057). v3.1 package unmodified.
- **v3.2 package created** with the two-commit C→R→E model (only R integration-eligible; E
  evidence-only); selftest PASS (15/15); invalid same-commit model REJECT; rejected LAB-B head
  REJECT; senior v3.2 exact-byte verdict **accept** (0 findings, 0 required actions).
- v3.2 frozen but **not dispatched**; leases v3.2 planned; **zero** LAB worktrees; **zero** leaves;
  D0/D1/D2 not executed.
- P5-005 remains `in_progress`; P5-007 remains `deferred`.
- Human redispatch authorization **pending**.

## Verification summary

- PR #34: OPEN, draft=true, mergeable; head revalidated == `da89654e...`; exact-head CI SUCCESS
  (control plane + audio core) on the reviewed head and on the new evidence head.
- v3.2 validators 15/15 PASS; manifest validation 12/12 PASS; selftest PASS (15/15); valid C→R→E PASS;
  invalid fixtures REJECT; rejected-head test REJECT; `git diff --check` PASS;
  `validate-project-control.py` PASS.

## Next action

Human: authorize or reject a NEW LAB-A/LAB-B dispatch using exclusively the exact and reviewed
v3.2 package, new branches, new worktrees and new leases. Until then:
`HUMAN_DECISION_PENDING`, `v3_1_status=IMMUTABLE_REJECTED_FOR_DISPATCH`,
`v3_2_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED` — no dispatch, no leases, no D0/D1/D2.
