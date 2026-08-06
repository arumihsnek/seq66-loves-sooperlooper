# CP-062 — V3.4 Dispatch Evidence Immutability Forensic Closure (2026-08-06)

Immutable checkpoint. Supersedes CP-061 as current. CP-048..CP-061 remain byte-identical and immutable.

## Objective

Forensic closure of the v3.4 dispatch evidence: document that after CP-061 was published at
`889f759b`, commit `e88f6300` replaced two previously indexed receipts (leaf-a-report.json,
leaf-b-report.json) — a post-checkpoint evidence mutation violating the append-only rule — and bind
both generations via an append-only forensic addendum without modifying any historical file.

## Status declarations

- CP-061 immutable; CP-048..CP-060 immutable; packages v3..v3.4 unmodified.
- Original indexed dispatch snapshot pinned to **`889f759b9774d877b950c0609d799c272f1c9923`**.
- Self-report amendment commit **`e88f630090acc877fcafdf3a9142c2f9a425f194`**.
- Only two reports changed post-CP061 (leaf-a-report.json, leaf-b-report.json); exactly one commit;
  zero checkpoint/index/verdict/package changes.
- Original index validates **19/19 effective files-map entries** at 889f759b (declared count field
  =20 is a documented pre-existing publisher artifact — duplicate dispatch-verdict.json key — NOT
  evidence deletion); original index **stale at e88f6300** (17/19; mismatches exactly
  leaf-a-report.json and leaf-b-report.json).
- Original blobs preserved (d7313cbe/99463244, 779/781 B, sha256 8bde156a.../29d975fe...) and
  amended blobs preserved (4fecb912/aa30bc11, 2095/2059 B, sha256 1dfb196e.../68697e21...).
- Append-only forensic addendum path:
  `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/forensic-addendum-v1/`
  (20 artifacts; manifest published sha256 `78b57464250afc9d7c79df65f054240b8647d61fd2b301e7136d68ea7d424fc0`;
  manifest as reviewed by senior `6131be045b23369d32d3c0277ad32128eea98fa80e61918d92c4cb116d7a2d50`,
  content preserved inside senior-review-bundle.json; delta = senior receipt hashes only).
- Senior forensic verdict: **accept**, 0 blocking, 0 required (mission
  `seq66-v3-4-dispatch-evidence-forensic-addendum`; execution `bd930aae-f912-4c5b-8b47-eba053d84dea`;
  fingerprint `3c3f33926d1bb6c5b2e4ce0ecc466bb8a8d38de458a9cd20d54d5ee69cc665ef`). Accept does NOT
  authorize package work or dispatch.
- Dispatch result unchanged: **DISPATCH_RESULT_REJECTED**; no integration candidate set.
- Leases v3.4 consumed; the exact v3.4 package cannot be redispatched with consumed exact lease IDs.
- No new dispatch authorized; no v3.5 package created; no integration; no D0/D1/D2.
- Worktrees and branches preserved (no destructive cleanup).

## Classification (forensic)

- `CP_061_DOCUMENT_IMMUTABLE=true`; `CP_061_REFERENCED_EVIDENCE_PATHS_MUTATED_AFTER_PUBLICATION=true`.
- `ORIGINAL_DISPATCH_INDEX_VALID_AT_889F=true` (19/19 effective); `ORIGINAL_DISPATCH_INDEX_VALID_AT_E88F=false`.
- `POST_CHECKPOINT_EVIDENCE_MUTATION=true`; `APPEND_ONLY_RULE_VIOLATED=true`.
- `DISPATCH_VERDICT_CHANGED=false`; `TOPOLOGY_VERDICT_CHANGED=false`; `INTEGRATION_STATE_CHANGED=false`.
- Causal limitation: "The addendum was applied by replacing two previously indexed receipts instead
  of publishing separate immutable addendum receipts." No evidence deletion; no fraud/malice asserted.

## Completed

- Remote revalidation (PR #34 open/draft/mergeable @ e88f6300; CI success; CURRENT->CP-061;
  CP-061/verdict/index blobs identical between 889f759b and e88f6300).
- git diff 889f759b..e88f6300: 1 commit, exactly 2 modified paths (leaf-a/leaf-b reports).
- Byte-exact snapshot exports (git show) original + amended; hash/blob bindings (OID, size, sha256).
- Index validations: 19/19 @889f759b; 17/19 @e88f6300 with exactly the two leaf reports.
- Branch live state + dispatch verdict revalidation (LAB-A remote==E; LAB-B remote absent, E^!=R).
- Forensic classification; append-only addendum (20 artifacts) + manifest + index.
- Senior plan-review: preflight VALID; **accept** 0/0 after resolving 2 blocking findings
  (19/19 effective wording; explicit manifest/hash binding).

## Verification

- Deterministic: git diff name-status/stat; per-blob sha256/size; index validations; JSON parse 20/20;
  branch revalidation; validate-project-control (to be run on evidence head).
- Senior: codex-senior-consult preflight + plan-review (label `seq66-v3-4-dispatch-evidence-forensic-addendum`).

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task P5-005 in_progress; P5-007 deferred.
- `dispatch_result=DISPATCH_RESULT_REJECTED`; `no_integration_candidate_set=true`.
- `forensic_closure=PASS`; `original_dispatch_snapshot_bound=true`;
  `post_checkpoint_mutation_documented=true`; `append_only_addendum_bound=true`.

## Risks and unresolved questions

- The historical dispatch-index declares count=20 with a 19-entry files map (pre-existing publisher
  artifact). Not modified; bound as-is by the forensic record.
- The published addendum manifest (78b57464) differs from the reviewed manifest (6131be04) only in
  the hashes of the two senior receipt artifacts; reviewed bytes are preserved in the senior bundle.

## Next executable action

- Human decision: authorize or reject a SEPARATE session to prepare a v3.5 process-hardening
  package (new batch/leases/result paths; leaf produces R only; leaf has NO push authority; frozen
  mechanical finalizer constructs E; controller sole publisher; create-only push; no force-push;
  manual envelopes rejected; dirty/untracked worktree rejected; ownership drift rejected; verifier
  PASS required before publication). NOT executed here.

## Open first

- `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/forensic-addendum-v1/forensic-addendum-manifest.json`
- `receipts/headless-lab-v3.4-dispatch/20260806T134656Z-v3-4-authorized-lab-dispatch/forensic-addendum-v1/forensic-classification.json`
- `doc/sooperlooper/checkpoints/CURRENT.md`

## Safe reference point

- Original dispatch evidence snapshot commit `889f759b9774d877b950c0609d799c272f1c9923`.
- Self-report amendment commit `e88f630090acc877fcafdf3a9142c2f9a425f194`.
- Current PR head (this commit).
