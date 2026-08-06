# CP-060 — V3.4 Functional-Contract Restoration (2026-08-06)

Immutable checkpoint. Supersedes CP-059 as current. CP-048..CP-059 remain byte-identical and immutable.

## Objective

Restore the canonical v3.2 functional contract that v3.3 had regressed
(`V3_3_FUNCTIONAL_CONTRACT_REGRESSION`, severity=blocking_for_dispatch, affected_controls=
lab-api-contract-continuity, audio-oracle-verification-semantics, jack-osc-sooperlooper-api-surface,
literal-test-coverage, canonical-helper-build-provenance, ownership-manifest-consistency) and combine
it with the retained v3.3 strict workflow layer. No third functional architecture is introduced.

## Status declarations

- `CP-059` immutable.
- v3.3 exact review remains valid as historical evidence.
- v3.3 strict workflow enforcement retained (envelope/controller receipt layer).
- v3.3 rejected for dispatch due to functional regression.
- v3.3 package unmodified.
- v3.2 functional contract restored (module/function/class surface, silence verification=FAIL,
  AnalysisResult, analyze_loop_reproduction, GraphAssertions, LabRunner, JACK/OSC/SooperLooper APIs).
- v3.3 strict envelope/controller receipt retained (C→R→E, exact IDs, literal byte-binding,
  artifact hash/size, controller acceptance receipt, full-completion).
- canonical helper build restored (gcc -std=c11 -O2 -ljack -lm -lpthread on the two canonical C sources).
- full literal tests restored (4 per lab: ft-a1..ft-a4, ft-b1..ft-b4).
- ownership consistency restored (LAB-A 5, LAB-B 12 incl. fixtures/.gitignore across
  scope/batch/baseline/lease/verifier).
- v3.4 selftest result: **PASS 28/28** (real temp git repos; incl. LAB-B full completion with real gcc rebuild).
- rejected head result: `0769d150...` **REJECT** (LAB-A and LAB-B).
- functional senior verdict: **accept** (execution `6aa5ba00-...`, fingerprint `78119705...`).
- workflow senior verdict: **accept** (execution `2733bea3-...`, fingerprint `b9ef169f...`).
- technical dispatch eligibility: **PASS**; dispatch authorization: **false** (separate fields).
- v3.4 frozen but not dispatched.
- leases planned (`LEASE-20260806T020249Z-LAB-A-V3_4`, `LEASE-20260806T020249Z-LAB-B-V3_4`).
- zero LAB worktrees; zero leaves; evidence worktrees preserved (no destructive authorization).
- P5-005 in_progress; P5-007 deferred.
- D0/D1/D2 not executed; human redispatch decision pending.

## Regression reproduction (v3.3)

- v3.2 analyze_wav: `(wav_path, expected_loop_frames, observed_loop_frames, tempo, numerator,
  sample_rate, silence_threshold=0.01, peak_threshold=0.9) -> AnalysisResult`;
  silent WAV => `verification=FAIL`; analyze_loop_reproduction + AnalysisResult (5 fields) declared;
  full method sets for JackServer/OscProbe/SooperLooperLauncher/GraphAssertions/LabRunner/ProcessSupervisor.
- v3.3: no structured surface (modules as plain lists); verifier hardcoded `analyze_wav(wav_bytes)->dict`
  with `silent=True`; 1 literal test per lab; synthetic python JSON build; `fixtures/.gitignore`
  omitted from scope/batch but hardcoded in the verifier.
- `regression_demonstrated=true`, `envelope_fix_scope_exceeded=true`, `functional_contract_continuity=false`.

## Controlled fixtures (v3.4)

| Case | Expected | Observed |
|---|---|---|
| functional continuity | PASS | PASS |
| v3.3 simplified API | REJECT | REJECT |
| v3.3 synthetic build | REJECT | REJECT |
| valid leaf-envelope | PASS | PASS |
| valid full-completion (LAB-A) | PASS | PASS |
| real canonical helper rebuild (LAB-B full) | PASS | PASS |
| silent WAV verification=FAIL | PASS | PASS |
| rejected head `0769d150` LAB-A | REJECT | REJECT |
| rejected head `0769d150` LAB-B | REJECT | REJECT |

## Validation results

- Selftest: **PASS 28/28** (functional continuity, strict envelope, full completion, negatives).
- Package validators: **PASS 37/37** (comparative vs v3.2: method sets, signatures, returns,
  silence semantics, literal completeness, canonical sources, compile commands, ownership, schemas,
  C→R→E, artifact integrity, old-ID reuse, predispatch, verifier compile).
- Manifest: **PASS 14/14** (zero missing, zero extra, candidate exact, batch exact, supersedes v3.3
  with declared reason).
- Literal-command escaping / silent-WAV test: **PASS** (real NUL PCM, no literal escaped text,
  6-arg mandatory signature, verification observed FAIL).

## Package v3.4

- Batch: `BATCH-20260806T020249Z-DOGFOOD004-LAB-V3_4`; leases, result paths and schema names NEW.
- Package manifest SHA-256: `63c371ba70491fa546ce4f8d980568440845c74fbb5af3a93502ff9b8c0a4da7`.
- Supersedes v3.3; supersession reason: `v3.3 strict workflow was internally valid but introduced
  unauthorized functional-contract and build-provenance regression`.
- 14 operational artifacts under `receipts/headless-lab-v3.4/`; binding evidence under
  `receipts/headless-lab-v3.4-review-binding/` (22 receipts).

## Stop gate

- `HUMAN_DECISION_PENDING=true`; `redispatch_authorized=false`; `integration_authorized=false`;
  `D0_D1_D2_authorized=false`.
- `v3_status=IMMUTABLE_REJECTED_FOR_DISPATCH`; `v3_1_status=...`; `v3_2_status=...`;
  `v3_3_status=...`; `v3_4_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`;
  `v3_4_technical_dispatch_eligibility=PASS`.
- The only human decision is to authorize or reject a NEW LAB-A/LAB-B dispatch using EXCLUSIVELY
  the exact, functionally continuous, doubly reviewed v3.4 package, new branches, new worktrees
  and new leases.

## Evidence

- `receipts/headless-lab-v3.4/` (package, frozen).
- `receipts/headless-lab-v3.4-review-binding/` (regression, continuity, API diff, ownership,
  escaping, canonical build, fixtures, selftest, rejected-head, dual senior bundles/preflights/
  verdicts, binding receipt + index).
- RUN: `/home/ubuntu/.hermes/runs/20260806T020249Z-v3-4-functional-contract-restoration/`.

## Completed

- Regression v3.3 reproduced from preserved bytes; v3.2 surface restored byte-equal in v3.4 contract.
- Selftest 28/28 PASS; validators 37/37 PASS; manifest 14/14 PASS; fixtures 9/9 PASS; rejected head REJECT.
- Dual senior review accept (functional continuity + strict workflow); technical eligibility PASS,
  authorization false.

## Verification

- Verifier v3.4 phases: functional-continuity / leaf-envelope / full-completion.
- Controlled runs on real temp git repos (incl. gcc/libjack compile of canonical C helpers).
- validate-project-control PASS; PR #34 exact-head CI success.

## Current state

- Manifest current_phase: `phase-5-exact-recording`; active task `P5-005` in_progress; P5-007 deferred.
- Headless-lab packages: v3/v3.1/v3.2/v3.3 IMMUTABLE_REJECTED_FOR_DISPATCH; v3.4
  EXACT_BYTES_REVIEWED_NOT_DISPATCHED (technical eligibility PASS, not authorized).

## Risks and unresolved questions

- No blocking risks found by either senior review.
- Only human decision pending: authorize/reject a NEW dispatch with the exact v3.4 package.

## Next executable action

- Human decision on a NEW LAB-A/LAB-B dispatch using exclusively the v3.4 package, new branches,
  new worktrees and new leases. Until then: no dispatch, no integration, no D0/D1/D2.

## Open first

- `PROJECT-MANIFEST.json` · `doc/sooperlooper/checkpoints/CURRENT.md` · `doc/sooperlooper/WORK-QUEUE.md`
- `receipts/headless-lab-v3.4/v3.4-package-manifest.json`
- `receipts/headless-lab-v3.4-review-binding/review-binding-receipt.json`

## Safe reference point

- PR #34 remote head `8609ca5028e9d4c0b3c379da0b7eae95888224b3` (v3.3 evidence container).
- v3.4 frozen evidence head: the commit that adds this checkpoint plus `receipts/headless-lab-v3.4/`
  and `receipts/headless-lab-v3.4-review-binding/`.
