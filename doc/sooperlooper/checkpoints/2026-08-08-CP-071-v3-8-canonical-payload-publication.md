# Checkpoint CP-071 — V3.8 canonical payload publication

Checkpoint ID: `CP-071`
Checkpoint date: 2026-08-08
Supersedes: `CP-070` (2026-08-08, immutable)
Phase: `phase-5-exact-recording`

## Objective

Publish to GitHub the EXACT reviewed and frozen v3.8 operational payload that
was previously local-only, closing the GitHub source-of-truth gap. The payload
was absent from the PR tree (`CANONICAL_V3_8_PAYLOAD_PRESENT=false`): the PR
contained review-binding and validation receipts but NOT
`receipts/headless-lab-v3.8/`. This checkpoint publishes the frozen bytes
byte-for-byte (no regeneration, no reformatting, no version/batch/path edits),
creates the durable publication binding, and records the closure. No dispatch,
no integration, no D0/D1/D2, no PR merge.

## Completed

- PR #34 revalidated: open, draft=true, mergeable=true, head
  `c75914a4124ae38c79ee81d61dc5b2b733ffeb90`; CURRENT → CP-070; zero v3.8
  remote result branches; zero activated v3.8 leases; zero v3.8 leaves.
- Canonical absence confirmed: `receipts/headless-lab-v3.8/` and
  `receipts/headless-lab-v3.8/package-payload-manifest-v3.8.json` ABSENT from
  the PR tree (CANONICAL_V3_8_PAYLOAD_PRESENT=false). Not used to regenerate.
- Reviewed payload recovered from local RUN
  `20260808T132428Z-v3-8-identity-coherence-package`:
  `frozen-payload/` (19 operational artifacts) + canonical manifest.
- Manifest raw SHA-256 =
  `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`.
- 19/19 operational hashes match the manifest AND the corrected review binding
  (`corrected-review-binding-v3.8.json`, already on GitHub): zero mismatch.
- Corrected review binding remains valid: effective accepts=3, blocking=0,
  required=0, dispatch_authorized=false.
- Byte-for-byte copy into the evidence worktree: 19/19 operational + manifest
  byte-identical (cmp verified).
- Publication binding created (outside payload):
  `receipts/headless-lab-v3.8-publication-closure/payload-publication-binding.json`
  with canonical paths, SHA-256 and expected Git blob OIDs (`git hash-object`)
  for all 19 artifacts and the manifest; future commit SHA NOT embedded.
- Zero payload modification; `byte_exact_copy=true`.

## Verification

- Pre-commit gate (evidence worktree from `c75914a4`): byte-identical compare
  19/19 operational + manifest vs local frozen source — PASS.
- Manifest raw SHA recalced = `bb4182…` — PASS.
- `git diff --check` — PASS (no whitespace errors).
- `python3 contrib/scripts/validate-project-control.py` — PASS.
- All new JSON parse; all published Python `py_compile` — PASS.
- Post-push: re-verify from GitHub (raw bytes): manifest SHA, 19/19 payload
  hashes vs manifest and vs corrected binding, Git blob OIDs vs binding.

## Current state

- `v3_8_payload_source_of_truth = GITHUB` (after publication)
- `v3_8_canonical_payload_binding = PENDING (post-push verification)`
- `v3_8_payload_manifest = bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`
- `v3_8_payload_unchanged = true`; `v3_8_corrected_review_binding_valid = true`
- `v3_8_technical_dispatch_eligibility = PASS`
- `v3_8_operational_dispatch_gate = PENDING (exact-head CI must complete)`
- `v3_8_dispatch_authorized = false`
- leases = planned; `leases_activated = false`; leaves = 0;
  lab_execution_worktrees = 0; remote_result_branches = 0
- `integration_authorized = false`; `D0_D1_D2_authorized = false`;
  `PR_merge_authorized = false`
- PR predecessor head: `c75914a4124ae38c79ee81d61dc5b2b733ffeb90`

## Risks and unresolved questions

- `exact_head_ci = PENDING` — workflows must run and succeed on the resulting
  PR head; runs on `c75914a4` do not count.
- `post_push_github_verification = PENDING` — must recover manifest + 19
  artifacts from GitHub and verify hashes + blob OIDs; if any artifact cannot
  be recovered: CANONICAL_PAYLOAD_PUBLICATION_FAILED STOP.
- `PR_mergeability = PENDING` after publication (must remain true).

## Next executable action

Stage the byte-exact payload + publication binding + this checkpoint + CURRENT;
run pre-push validation; re-check PR head (`c75914a4`, mergeable=true); push
one material commit (fast-forward only, no force, no fork-main write); then
verify canonically from GitHub and observe exact-head CI. Not dispatch.

## Open first

- PR #34 exact head (after push)
- `receipts/headless-lab-v3.8/package-payload-manifest-v3.8.json` on GitHub
- 19 operational artifacts on GitHub
- Project control plane run (head == new PR head)
- Audio integration core run (head == new PR head)
- CP-071

## Safe reference point

- previous_publication_head = `c75914a4124ae38c79ee81d61dc5b2b733ffeb90`
- reviewed_manifest_sha256 =
  `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`
- canonical_payload_path = `receipts/headless-lab-v3.8/`
- publication_binding_path =
  `receipts/headless-lab-v3.8-publication-closure/payload-publication-binding.json`
