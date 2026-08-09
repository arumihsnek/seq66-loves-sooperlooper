# CP-056 — V3 Exact-Byte Review Binding (2026-08-05)

Immutable checkpoint. Supersedes CP-055 as current. CP-048..CP-055 remain byte-identical and immutable.

## Objective

Close the exact-byte review gap for the PUBLISHED v3 package: revalidate identities,
export the 13 v3 artifacts from tracked blobs at the exact PR head
(`66f2f0c8ced0b29ca579fd01eafda2daca51b75d`), validate the package manifest against the
exported bytes, explain the historical hash discrepancy (reported vs GitHub manifest),
run all v3 validators + selftest + rejected-head test from tracked bytes, obtain a NEW
senior plan-review bound to the exact bytes, and publish durable evidence under
`receipts/headless-lab-v3-review-binding/` — WITHOUT dispatching, mutating v3, or running D0/D1/D2.

## Completed

- Identities revalidated: PR #34 OPEN/draft/MERGEABLE; remote head == `66f2f0c8…`
  (EXPECTED_PR_HEAD); `validate-control-plane` SUCCESS + `compile-and-test` SUCCESS on that head.
- Isolated worktree created from `66f2f0c8…` (HEAD verified, clean); no v2 dispatch worktrees reused.
- Byte-exact tracked export (13 files via `git cat-file -p` of blobs at `66f2f0c8…`);
  per-file blob SHA, size, SHA-256, export timestamp recorded.
- Package manifest validation: **MATCH** — 12/12 files present, 12/12 sizes exact,
  12/12 SHA-256 exact, zero missing, zero extra, batch ID exact, candidate binding exact
  (verified in scope-freeze/batch-manifest/dispatch-plan/lease plans).
- Historical hash discrepancy explained: reported `3948fb45…`/`58dc0208…`/`b8384f66…`
  were STALE first-generation values quoted in the CP-055 audit doc; the published package
  is the second generation (post verifier patch + regeneration); old bytes not recoverable
  → `historical_hash_origin_unresolved` for the old bytes, cause demonstrated.
- v3 validators from tracked bytes: 10/10 PASS (JSON 12/12, scope, batch-manifest,
  port-plan, lease, envelope schema, predispatch, v2-ID reuse refined, ownership overlap,
  forbidden branch/worktree reuse).
- `acceptance-verifier-v3.py --selftest`: **PASS** (interface discovered via `--help`).
- Rejected-head test on clean checkout of `0769d150…`: **REJECT** as expected (exit 1).
- New senior plan-review (`seq66-lab-v3-exact-published-package`): preflight VALID
  (0 processes), consult **accept** — bound to exact PR head, package manifest SHA-256 and
  all 12 artifact hashes; 0 blocking findings, 0 required actions.
- Durable evidence published under `receipts/headless-lab-v3-review-binding/` (10 files + index).
- v3 package under `receipts/headless-lab-v3/` **UNCHANGED**.

## Verification

- `git rev-parse HEAD` in review worktree == `66f2f0c8ced0b29ca579fd01eafda2daca51b75d`; `git status --porcelain=v1` empty.
- Package manifest SHA-256: `2114aa70a76ac23d70c374c1d5b042851d4c5eeec3b350e74cbfde1326c1c640`
  (manifest blob `6a7292b5…`).
- 12 artifact SHA-256 verified from tracked bytes (list in `tracked-package-hashes.json`).
- Validation receipt SHA-256: `5a9d4f333ed47a9f2f77e9c7…`; selftest receipt: `117645c5c3dc7a5e…`;
  rejected-head test receipt: `dd4d739e9a0c1c6d…`.
- Senior response fingerprint: `54a77623ae817a6799827a103286c178228d7f5e4ff6573dfa509d3df69017c3`;
  execution id `e2cd5958-702e-46c5-b1f2-1ed0323ba40c`; verdict **accept**.
- `git diff --check` PASS; JSON parse PASS; v3 binding validators PASS;
  `python3 contrib/scripts/validate-project-control.py` PASS.
- Exact-head CI on the NEW PR head: Project control plane SUCCESS + Audio integration core SUCCESS.

## Current state

- **Observed GitHub hashes** (correct): scope-freeze `c84c0c6d…`, acceptance-manifest
  `114bce61…`, acceptance-verifier `f6ee256d…`; all 12 match the published manifest and the
  tracked blobs.
- **Previously reported hashes**: scope-freeze `3948fb45…`, acceptance-manifest `58dc0208…`,
  acceptance-verifier `b8384f66…` — stale first-generation values; old bytes not recoverable.
- **Cause**: demonstrated (regeneration after verifier patch changed exactly the three
  artifacts that embed `frozen_at_utc`/verifier hash); origin of the old bytes: unresolved.
- **Package manifest validation**: MATCH. **Selftest**: PASS. **Rejected-head test**: REJECT.
- **Senior exact verdict**: accept (bound to exact bytes). **v3 package**: unchanged.
- **v3 not dispatched**; leases v3 planned (not activated); zero v3 worktrees; zero v3 leaves;
  D0/D1/D2 not executed.
- `active_task = P5-005` remains `in_progress`; `P5-007` remains `deferred`.
- Human redispatch authorization **pending** (`REDISPATCH_AUTHORIZED=false`).

## Risks and unresolved questions

- Old reported-hash bytes are unrecoverable; `historical_hash_origin_unresolved` recorded —
  does not block the exact-byte review (published bytes verified independently).
- Any future v3 correction must be a NEW package version (v3.1+), never a silent mutation of v3.
- Future dispatch still requires: human authorization, predispatch freshness checks,
  harness hash verification, selftest rerun, gcc/libjack build deps.

## Next executable action

Human decision: **authorize or reject the new LAB-A/LAB-B dispatch** using the exact and
reviewed v3 package (`receipts/headless-lab-v3/`), new branches, new worktrees and new
leases. Until then: `HUMAN_DECISION_PENDING`, `v3_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`,
no dispatch, no leases, no D0/D1/D2.

## Open first

`receipts/headless-lab-v3-review-binding/review-binding-receipt.json`

## Safe reference point

- candidate_head = `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- reviewed PR head = `66f2f0c8ced0b29ca579fd01eafda2daca51b75d`
- rejected_lab_b_head = `0769d150f4989b8331db8273aa7a771f7b929026`
- package manifest SHA-256 = `2114aa70a76ac23d70c374c1d5b042851d4c5eeec3b350e74cbfde1326c1c640`
- senior fingerprint = `54a77623ae817a6799827a103286c178228d7f5e4ff6573dfa509d3df69017c3`
- PR #34 head after this binding commit: documented in PR body and closure RUN
  (`20260805T160710Z-v3-exact-byte-review-binding`) post-push.
