# Current checkpoint — V3.1 acceptance-gate correction

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-05-CP-057-v3-1-acceptance-gate-correction.md`

Checkpoint ID: `CP-057`
Checkpoint date: 2026-08-05
Phase: `phase-5-exact-recording`
Active task: `P5-005`
Status: `in_progress`
Branch: `integration/baseline-qualification-20260805`

## Head identity (explicit chain, no self-reference)

- `candidate_head = 509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- `reviewed_pr_head = 66f2f0c8ced0b29ca579fd01eafda2daca51b75d`
- `rejected_lab_b_head = 0769d150f4989b8331db8273aa7a771f7b929026`
- PR #34 head after this evidence commit: documented in PR body and closure RUN
  (`20260805T165044Z-v3-1-acceptance-gate-correction`) post-push.

## V3.1 acceptance-gate correction status

- **v3 immutable / rejected for dispatch**: LAB-A foreign-process acceptance-gate contradiction
  (`V3_ACCEPTANCE_GATE_DEFECT`, blocking_for_dispatch). v3 exact-byte review remains valid
  HISTORICAL evidence (CP-056). v3 package unmodified.
- **v3.1 package created** with new IDs/hashes (`BATCH-20260805T165044Z-DOGFOOD004-LAB-V3_1`);
  foreign process created OUTSIDE ProcessSupervisor; owned cleanup and foreign survival tested
  separately; selftest PASS; rejected LAB-B head REJECT; senior v3.1 exact-byte verdict **accept**
  (0 findings, 0 required actions).
- v3.1 frozen but **not dispatched**; leases v3.1 planned; **zero** v3.1 worktrees; **zero** v3.1
  leaves; D0/D1/D2 not executed.
- P5-005 remains `in_progress`; P5-007 remains `deferred`.
- Human redispatch authorization **pending**.

## Verification summary

- PR #34: OPEN, draft=true, mergeable; head revalidated == `9678ff50...`; exact-head CI SUCCESS
  (control plane + audio core) on the reviewed head and on the new evidence head.
- v3.1 validators 14/14 PASS; manifest validation 12/12 PASS; selftest PASS; controlled probe PASS;
  rejected-head test REJECT; `git diff --check` PASS; `validate-project-control.py` PASS.

## Next action

Human: authorize or reject a NEW LAB-A/LAB-B dispatch using exclusively the exact and reviewed
v3.1 package, new branches, new worktrees and new leases. Until then:
`HUMAN_DECISION_PENDING`, `v3_status=IMMUTABLE_REJECTED_FOR_DISPATCH`,
`v3_1_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED` — no dispatch, no leases, no D0/D1/D2.
