# Checkpoint CP-073 — V3.8 rejected dispatch contract forensics

Checkpoint ID: `CP-073`
Checkpoint date: 2026-08-08
Supersedes: `CP-072` (2026-08-08, immutable)
Phase: `phase-5-exact-recording`

## Objective

Forensic review of the rejected v3.8 dispatch (DISPATCH_REJECTED, CP-072):
classify each blocking finalizer finding against the three evidence layers
(normative contract / actual leaf instructions / executable gate), preserve
the failed R commits byte-exact, audit SELFTEST normative-alignment claims,
and produce a leaf-contract visibility matrix. No product corrections, no new
leaves, no package modification, no integration.

## Completed

- Pre-dispatch state verified: PR #34 open/draft/mergeable=true/head
  `9df4bd7c…`; CURRENT→CP-072; exact-head CI runs 31282991264/31282991285
  success; zero v3.8 remote branches.
- Single-writer lock acquired fresh (flock).
- Local dispatch recovered: R_A `9a09b5f6…` and R_B `357fe389…` exist,
  R^==C both; worktrees preserved (WORKTREE_DELETION_AUTHORIZED=false).
- **Exact R preservation** (`receipts/headless-lab-v3.8-dispatch-forensic/
  20260808T223904Z-v3-8-authorized-lab-dispatch/r-preservation/`):
  - preservation manifests: commit object raw bytes + SHA-256, parent, tree,
    author/committer, full C..R diff, changed-path list, blob OID + SHA-256 +
    mode per path
  - minimal git bundles with C as prerequisite (`--not C`); `git bundle
    verify` OK; recovery in temp repo with only C: recovered commit ==
    original commit, recovered tree == original tree (byte-exact)
  - bundles are FORENSIC EVIDENCE ONLY (never candidate/result/E/A/integration)
- **Three-layer source reconstruction**: Layer 1 (7 normative artifacts),
  Layer 2 (actual leaf prompts; artifacts each leaf was told to read),
  Layer 3 (mechanical-finalizer, acceptance-verifier, selftest fixtures,
  dispatch receipts).
- **F-A1 (LAB-A standalone import) = FINALIZER_OVERCONSTRAINT** — controlled
  experiment: contractual package import (ft-a2) PASS; finalizer
  spec_from_file_location on the same file raises ImportError; minimal
  fixture with identical API: relative variant package-import=True /
  loader=ImportError, standalone variant package-import=True / loader=LOADED.
  Layer 1 has NO standalone-loading requirement.
- **F-B1 (tracked ELF) = LEAF_CONTRACT_VIOLATION** — Layer 1 explicitly
  rejects tracked ELF (build-provenance-policy; dispatch-plan gate) and says
  controller ignores leaf-produced binaries; R_B tracked two ELF fixtures
  (72,408B / 71,864B, mode 100755). Nuance: the canonical fixture
  representation (directories + .gitkeep) is NOT normative in Layer 1.
- **F-B2 (annotation representation) = FINALIZER_REPRESENTATION_OVERCONSTRAINT**
  — controlled experiment: evaluated annotations give correct
  typing.get_type_hints() semantics but fail the finalizer's
  str(annotation) string-match; `from __future__ import annotations` passes
  the string-match but makes get_type_hints() raise NameError (Path
  undefined). Layer 1 requires semantic annotation types, not a runtime
  string representation.
- **F-B3 (analyze_loop_reproduction) = FINALIZER_OVERCONSTRAINT** — R_B
  returns a dict with valid controlled WAV inputs (source_frames,
  captured_frames, ratio_percent, within_tolerance, tolerance_percent); the
  finalizer calls it with nonexistent 'x.wav'/'y.wav' → FileNotFoundError;
  Layer 1 does not prescribe missing-file behavior.
- **Selftest interpretation**: SELFTEST_GLOBAL=PASS (24/24) proves
  INTERNAL_CONSISTENCY (finalizer rejects its adversarial fixtures and
  accepts its own valid fixture); it does NOT prove NORMATIVE_ALIGNMENT (the
  selftest's valid fixtures silently encode standalone imports, future
  annotations, and .gitkeep dirs — none of which are Layer 1 requirements).
- **Leaf-contract visibility matrix**: F-A1 IMPLICIT_ONLY_IN_FINALIZER;
  F-B1 EXPLICIT_BUT_NOT_DELIVERED; F-B2 IMPLICIT_ONLY_IN_FINALIZER;
  F-B3 IMPLICIT_ONLY_IN_FINALIZER.
- **Senior forensic review** (codex-senior-consult, integrated-review,
  mission `seq66-v3-8-rejected-dispatch-contract-alignment`): status
  COMPLETED / VALID_ADVISORY_VERDICT, verdict `changes_required`,
  execution `0631a805-a40b-44f4-836c-f2a194c2b43e`.
  - Q1: F-B1 is the only genuine leaf contract violation.
  - Q2: F-A1, F-B2, F-B3 are confirmed finalizer overconstraints.
  - Q3: SELFTEST_GLOBAL does not prove normative alignment.
  - Q4: v3.8 is not safe for another dispatch unchanged.
  - Q5: v3.9 package correction is required (separate human authorization).
  - Q6: No failed R is eligible for integration.
  - Recommendation: Alternative B.

## Verification

- All controlled experiments executed on real R objects / canonical artifacts;
  fixtures live only inside the forensic RUN and are never candidates.
- R preservation verified end-to-end: bundle verify + recovery in isolated
  temp repo (commit + tree SHA equality).
- Senior verdict actionable: status=COMPLETED, detailed_status=
  VALID_ADVISORY_VERDICT, persisted artifact identity verified.
- Zero modifications to frozen v3.8 package, CP-072 or earlier, original
  dispatch receipts, or R commits.

## Current state

- Forensic conclusion gate: **OUTCOME B — ONE_OR_MORE_FINALIZER_OVERCONSTRAINTS_CONFIRMED**
- `v3_8_technical_dispatch_eligibility = REVOKED`
- `new_v3_8_dispatch_authorized = false`; `replacement_leaf_authorized = false`;
  `second_dispatch_authorized = false`
- `v3_9_package_preparation_authorized = false`; `v3_9_package_correction_recommended = true`
- `integration_authorized = false`; `D0_D1_D2_authorized = false`;
  `PR_merge_authorized = false`; `force_push_authorized = false`
- R_A / R_B preserved as forensic evidence only (no E, no A, no result branches)
- Corrected review binding remains valid (3/0/0); canonical package untouched
  (source of truth = GITHUB, manifest `bb4182…`).

## Risks and unresolved questions

- v3.9 must NOT be prepared without separate human authorization.
- A future corrected package must align the finalizer gates with Layer 1
  (loader context, annotation semantics, missing-file behavior) while
  retaining the normative tracked-ELF prohibition.
- R_A/R_B remain non-integration-eligible even though F-A1/F-B2/F-B3 are
  overconstraints; F-B1 (tracked ELF) is a genuine leaf violation.

## Next executable action

Publish controller-owned forensic evidence + CP-073 + CURRENT to the
control-plane source branch (fast-forward only, no fork-main, no force), then
obtain the human decision: authorize/reject v3.9 package correction
preparation (recommended path) — NOT a new v3.8 dispatch (eligibility
REVOKED).

## Open first

- PR #34 exact head (after control-plane commit)
- `receipts/headless-lab-v3.8-dispatch-forensic/` on GitHub
- Project control plane run (new head)
- Audio integration core run (new head)
- CP-073

## Safe reference point

- predecessor_head = `9df4bd7c7a47f4771a71cf0a1e3480dff6461c58`
- candidate = `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- package_manifest_sha256 = `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`
- R_A = `9a09b5f6cd4019873886c7714276312b62c7e4f5` (forensic only)
- R_B = `357fe38983784c721e25ed2c8ae59a6303e2ef5d` (forensic only)
- forensic_evidence_path = `receipts/headless-lab-v3.8-dispatch-forensic/20260808T223904Z-v3-8-authorized-lab-dispatch/`
- senior_execution_id = `0631a805-a40b-44f4-836c-f2a194c2b43e`
