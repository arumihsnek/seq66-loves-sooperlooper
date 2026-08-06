# CP-063 — V3.4 Forensic Review Binding Correction (2026-08-06)

Immutable checkpoint. Supersedes CP-062 as current. CP-048..CP-062 remain byte-identical and immutable.

## Objective

Correct, append-only, the circular review binding discovered in forensic-addendum-v1: the senior
reviewed manifest generation `6131be04...` while the published manifest was `78b57464...`. v2
publishes a non-circular three-layer binding (frozen payload → payload-manifest → review/binding
after the verdict) without modifying any v1 artifact.

## Completed

- Audited all 20 forensic-addendum-v1 artifacts from Git blobs (byte-exact): inventory, counts,
  blob OIDs, duplicate content, review generations.
- v1 classification: reviewed generation preserved (`6131be04...`), published generation preserved
  (`78b57464...`), `V1_PUBLISHED_BYTES_EXACTLY_SENIOR_REVIEWED=false`,
  `V1_REVIEW_BINDING_CIRCULARITY=true`; manifest count 19 vs map 18; index 20 vs map 19 without
  explicit self-exclusion; 6 `blob_oid` fields in invalid `<commit>:<path>` form; real snapshot OIDs
  `d7313cbe`/`99463244`/`4fecb912`/`aa30bc11` verified; duplicate content groups (branch-live-state ==
  dispatch-verdict-revalidation; both index copies identical blob).
- Built Layer A payload (10 artifacts) and Layer B `payload-manifest-v2.json` frozen read-only
  (`PAYLOAD_MANIFEST_SHA256=a9a1de5b6e0bab45169ff0cd710a5a79bd9705c150f52b8181216fe7e65e8008`).
- Validator v2 (50 checks + 7 negative fixtures): real payload 50/50 PASS; F1-F6 REJECT; F7 PASS;
  re-run after Layer C materialization still PASS (post-freeze identity 10/10).
- Senior review: gen1 changes_required (2/4), gen2 changes_required (3/4), gen3 **accept 0/0**
  (execution `7fa3eb51-4ec6-4874-a8da-b07892507982`, fingerprint
  `e3cf22b32948fabb8fa9555be8012280319edd6408324b1aa361936e26fd90f2`) bound to
  `PAYLOAD_MANIFEST_SHA256=a9a1de5b...` and the 10 payload hashes.
- Layer C materialized after the verdict: senior-preflight-v2, senior-review-bundle-v2,
  senior-verdict-v2, review-binding-receipt-v2 (self-review=false, created post-verdict),
  publication-index-v2 (directory=16, indexed=15, self_excluded=true,
  self_excluded_filename=publication-index-v2.json).

## Verification

- `git diff fc54a04b..e2183003 -- forensic-addendum-v1/` empty → v1 byte-identical.
- `validator_v2.py` exit 0 twice; `VALIDATOR_GLOBAL=PASS` after Layer C.
- `git cat-file -e` for the four real snapshot OIDs OK.
- JSON parse: all 16 v2 publication artifacts OK.
- validate-project-control PASS (run pre-push).

## Current state

- `forensic_addendum_v1_preserved=true`
- `forensic_addendum_v1_final_exact_review_binding=false`
- `forensic_addendum_v2_noncircular_binding=true`
- `forensic_closure_final=PENDING` (requires exact-head CI success on the new head)
- `dispatch_result=DISPATCH_RESULT_REJECTED`; `no_integration_candidate_set=true`
- `v3_5_package_work_authorized=false`; `dispatch_authorized=false`; `integration_authorized=false`
- `D0_D1_D2_authorized=false`; `PR_merge_authorized=false`
- CI runs 31127675599/31127675627 were queued at e2183003 (not success); state re-observed on the new head.
- CURRENT points to this checkpoint (CP-063).

## Risks and unresolved questions

- GitHub partial system outage may leave exact-head CI queued/cancelled; gate stays PENDING until
  both Project control plane and Audio integration core report completed+success on the new PR head.
- v3.5 package work remains NOT authorized; human decision required.
- v1 defects are historical evidence, not corrected in place (append-only discipline).

## Next executable action

Push forensic-addendum-v2 + CP-063 + CURRENT to PR #34 (fast-forward, no history rewrite), update
PR body, observe exact-head CI, then STOP for the human decision on v3.5.

## Open first

PR #34 exact head == new evidence head; exact-head CI (Project control plane + Audio integration core).

## Safe reference point

`e218300361e3128d0154f24b1b55fe6cdfba4bff` (previous PR head) + v1 byte-identical; payload frozen
read-only in the RUN and byte-hash-locked.
