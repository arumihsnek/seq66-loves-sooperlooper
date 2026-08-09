# CP-059 — V3.3 Strict Envelope and Controller-Acceptance Correction (2026-08-06)

Immutable checkpoint. Supersedes CP-058 as current. CP-048..CP-058 remain byte-identical and immutable.

## Completed

- v3.2 incomplete-envelope defect reproduced from exact preserved bytes
  (`v3_2_verifier_incorrectly_accepts_incomplete_envelope=true`; 13/15 negative-matrix false-accepts).
- v3.3 package built and frozen: `receipts/headless-lab-v3.3/` (13 operational artifacts +
  manifest `ef17ecd1ba36be6597fe0279890d6e730144df7eaede7960167d3a5eaba2aefc`).
- v3.3 selftest **PASS 43/43**; controlled fixtures **8/8**; package validators **24/24**;
  manifest validation **13/13**; escaping test **PASS**; artifact-integrity test **PASS**.
- Senior plan-review **accept** (0 blocking findings, 0 required actions); accept does NOT
  authorize dispatch.
- CP-058 declared immutable; v3/v3.1/v3.2 packages untouched and byte-identical.

## Verification

- Selftest (real temp git repositories): 43/43 PASS.
- Controlled fixtures: valid leaf-envelope PASS, valid full-completion PASS, v3.2 minimal
  envelope REJECT, R empty REJECT, wrong artifact hash REJECT, missing controller receipt REJECT,
  rejected head `0769d150` REJECT (LAB-A and LAB-B).
- Package validators: PASS 24/24. Manifest: PASS 13/13. Literal-command escaping: PASS.
- PR #34 remote head `01d27c58...`; exact-head CI success (Project control plane, Audio
  integration core).

## Current state

- Phase: 5 (exact musical recording); active task `P5-005` in_progress.
- v3 / v3.1 / v3.2: IMMUTABLE_REJECTED_FOR_DISPATCH (historical evidence).
- v3.3: EXACT_BYTES_REVIEWED_NOT_DISPATCHED (frozen, not dispatched).
- Stop gate: `HUMAN_DECISION_PENDING`; `redispatch_authorized=false`;
  `integration_authorized=false`; `D0_D1_D2_authorized=false`.

## Risks and unresolved questions

- No new blocking risks found by senior review (0 findings).
- The only unresolved decision is human: authorize or reject a NEW dispatch.
- Prior evidence worktrees remain (no destructive authorization granted); they are NOT LAB
  worktrees and do not affect the zero-LAB-worktree declaration.

## Next executable action

- Human decision on a NEW LAB-A/LAB-B dispatch using exclusively the exact v3.3 package,
  new branches, new worktrees and new leases. Until then: no dispatch, no integration,
  no D0/D1/D2.

## Open first

- `PROJECT-MANIFEST.json` (live state)
- `doc/sooperlooper/checkpoints/CURRENT.md`
- `doc/sooperlooper/WORK-QUEUE.md` (P5-005)
- `receipts/headless-lab-v3.3/v3.3-package-manifest.json`
- `receipts/headless-lab-v3.3-review-binding/review-binding-receipt.json`

## Safe reference point

- PR #34 remote head `01d27c58330ab753c3499c6fb67336e0f91b961a` (v3.2 evidence container,
  byte-identical packages v3/v3.1/v3.2).
- v3.3 frozen evidence head: the commit that adds this checkpoint plus
  `receipts/headless-lab-v3.3/` and `receipts/headless-lab-v3.3-review-binding/`.

## Objective

Correct the v3.2 incomplete-envelope defect (`V3_2_ENVELOPE_VALIDATION_INCOMPLETE`,
severity=blocking_for_dispatch, affected_controls=result-envelope-schema-enforcement,
literal-test-binding, artifact-integrity, provenance-binding, non-empty-result-enforcement)
with a NEW package v3.3 that keeps the two-commit C→R→E model and adds STRICT runtime
enforcement of every normative envelope field plus a SEPARATE controller acceptance receipt.

## Status declarations

- `CP-058` immutable.
- v3.2 exact-byte review remains valid as historical evidence.
- v3.2 rejected for dispatch due to incomplete envelope enforcement.
- v3.2 package unmodified (byte-identical; 12/12 hashes revalidated).
- v3.3 strict leaf-envelope validation (all normative fields, exact IDs, unknown/self/controller keys rejected).
- separate controller acceptance receipt (`controller-acceptance-receipt/v3.3`), created after E is pushed, never inside E.
- artifact hash/size validation from R blobs; artifact set exactly == C..R tracked changes.
- literal command byte-binding (`command_sha256` == sha256 of frozen command bytes) and controller reexecution.
- R non-empty enforcement (artifacts never empty).
- v3.3 selftest verdict: **PASS 43/43** (real temp git repositories).
- v3.2 minimal envelope REJECT under v3.3 (controlled reproduction).
- rejected LAB-B head (`0769d150...`) REJECT under v3.3 (LAB-A and LAB-B).
- senior v3.3 verdict: **accept**, 0 blocking findings, 0 required actions
  (execution `d9128f8f-a4c5-4c59-a3e3-23505af8567e`, fingerprint `cfda9d4a833b568455f740d46bf3a76817666776e49f3a6d2ff84e78eedca709`).
- v3.3 frozen but NOT dispatched.
- new leases planned (`LEASE-20260806T011700Z-LAB-A-V3_3`, `LEASE-20260806T011700Z-LAB-B-V3_3`).
- zero LAB worktrees; zero leaves; evidence worktrees of prior runs inventoried and NOT removed
  (no destructive authorization granted).
- D0/D1/D2 not executed.

## Controlled reproduction of the v3.2 defect

- Exact v3.2 verifier (preserved bytes) returns **PASS** for an envelope containing only
  `candidate_commit_oid`, `implementation_result_commit_oid`, `implementation_result_tree_oid`,
  `git_object_format` and `artifacts={}` → `v3_2_verifier_incorrectly_accepts_incomplete_envelope=true`.
- v3.2 negative matrix: 15 isolated cases, 13 false-accepts; `artifact_path_missing` and `R_empty`
  rejected only incidentally.
- v3.3 rejects the same minimal envelope (strict schema + keys policy + artifact + R non-empty rules).

## Controlled fixtures (v3.3 CLI)

| Case | Expected | Observed |
|---|---|---|
| valid leaf-envelope phase | PASS | PASS |
| valid full-completion phase | PASS | PASS |
| v3.2 minimal envelope | REJECT | REJECT |
| R empty | REJECT | REJECT |
| wrong artifact hash | REJECT | REJECT |
| missing controller receipt (full-completion) | REJECT | REJECT |
| rejected head `0769d150` LAB-A | REJECT | REJECT |
| rejected head `0769d150` LAB-B | REJECT | REJECT |

## Validation results

- Package validators: **PASS 24/24** (json parse, scope, batch, dispatch, port-plan, leases,
  ownership overlap, no-globs, old-ID/path reuse, envelope schema, controller-acceptance schema,
  build provenance, predispatch, command escaping, artifact integrity, verifier compile).
- Manifest: **PASS 13/13** (13 operational artifacts; zero missing; zero extra; candidate exact;
  batch exact; supersedes v3.2 with declared reason).
- Literal-command escaping test: **PASS** (silent WAV contains real NUL bytes; no literal escaped text;
  frozen command executes exit 0).
- Artifact integrity test: **PASS**.

## Package v3.3

- Batch: `BATCH-20260806T011700Z-DOGFOOD004-LAB-V3_3`; leases and result paths NEW (no v2/v3/v3.1/v3.2 reuse).
- Package manifest SHA-256: `ef17ecd1ba36be6597fe0279890d6e730144df7eaede7960167d3a5eaba2aefc`.
- Supersedes v3.2; supersession reason: `incomplete runtime enforcement of required envelope,
  artifact, literal-test and provenance fields`.
- 13 operational artifacts under `receipts/headless-lab-v3.3/`; binding evidence under
  `receipts/headless-lab-v3.3-review-binding/` (17 receipts).

## Stop gate

- `HUMAN_DECISION_PENDING=true`; `redispatch_authorized=false`; `integration_authorized=false`;
  `D0_D1_D2_authorized=false`.
- `v3_status=IMMUTABLE_REJECTED_FOR_DISPATCH`; `v3_1_status=IMMUTABLE_REJECTED_FOR_DISPATCH`;
  `v3_2_status=IMMUTABLE_REJECTED_FOR_DISPATCH`; `v3_3_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`.
- The only human decision is to authorize or reject a NEW LAB-A/LAB-B dispatch using EXCLUSIVELY
  the exact and reviewed v3.3 package, new branches, new worktrees and new leases.

## Evidence

- `receipts/headless-lab-v3.3/` (package, frozen).
- `receipts/headless-lab-v3.3-review-binding/` (reproduction, matrices, fixtures, selftest,
  rejected-head, senior bundle/preflight/verdict, binding receipt + index).
- RUN: `/home/ubuntu/.hermes/runs/20260806T011700Z-v3-3-strict-envelope-correction/`.
