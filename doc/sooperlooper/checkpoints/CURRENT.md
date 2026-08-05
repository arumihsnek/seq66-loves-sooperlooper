# Current checkpoint — V3 exact-byte review binding

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-05-CP-056-v3-exact-byte-review-binding.md`

Checkpoint ID: `CP-056`
Checkpoint date: 2026-08-05
Phase: `phase-5-exact-recording`
Active task: `P5-005`
Status: `in_progress`
Branch: `integration/baseline-qualification-20260805`

## Head identity (explicit chain, no self-reference)

- `candidate_head = 509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- `reviewed_pr_head = 66f2f0c8ced0b29ca579fd01eafda2daca51b75d`
- `rejected_lab_b_head = 0769d150f4989b8331db8273aa7a771f7b929026`
- PR #34 head after this binding commit: documented in PR body and closure RUN
  (`20260805T160710Z-v3-exact-byte-review-binding`) post-push.

## V3 exact-byte review status

- **Observed GitHub hashes** verified against tracked blobs: MATCH (12/12).
- **Previously reported hashes** (`3948fb45…`/`58dc0208…`/`b8384f66…`): stale first-generation
  values; old bytes not recoverable → `historical_hash_origin_unresolved`, cause demonstrated.
- Package manifest validation: **MATCH** (12 files, sizes, SHA-256, batch, candidate).
- Acceptance selftest: **PASS**. Rejected-head test (`0769d150…`): **REJECT**.
- Senior exact-byte verdict: **accept** (fingerprint `54a77623…`, 0 findings, 0 required actions).
- v3 package **unchanged**; **not dispatched**; leases v3 planned; zero v3 worktrees/leaves;
  D0/D1/D2 not executed.
- P5-005 remains `in_progress`; P5-007 remains `deferred`.
- Human redispatch authorization **pending**.

## Verification summary

- PR #34: OPEN, draft=true, mergeable; head revalidated == `66f2f0c8…`; exact-head CI SUCCESS
  (control plane + audio core) on the reviewed head and on the new binding head.
- 10/10 v3 validators PASS from tracked bytes; `git diff --check` PASS;
  `validate-project-control.py` PASS.

## Next action

Human: authorize or reject a NEW LAB-A/LAB-B dispatch using exclusively the exact and
reviewed v3 package, new branches, new worktrees and new leases. Until then:
`HUMAN_DECISION_PENDING`, `v3_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED` —
no dispatch, no leases, no D0/D1/D2.
