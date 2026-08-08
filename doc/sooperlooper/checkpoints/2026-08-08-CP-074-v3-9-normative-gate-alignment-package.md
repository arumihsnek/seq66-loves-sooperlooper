# Checkpoint CP-074 — V3.9 normative gate alignment package

Checkpoint ID: `CP-074`
Checkpoint date: 2026-08-08
Supersedes: `CP-073` (2026-08-08, immutable)
Phase: `phase-5-exact-recording`

## Objective

Prepare, validate, review and publish the v3.9 package that aligns the
executable gates with the normative contract, correcting the three v3.8
finalizer overconstraints confirmed by the rejected-dispatch forensic review
(F-A1, F-B2, F-B3), reinforcing the leaf-visible tracked-ELF requirement
(F-B1), and adding normative-gate traceability + leaf requirement visibility.
No leaves, no leases activation, no result branches, no D0/D1/D2, no
integration, no PR merge.

## Completed

- Source state verified: PR #34 open/draft/mergeable=true/head
  `cc1853a0…`; CURRENT→CP-073; exact-head CI runs 31283688408/31283688397
  success; zero v3.9 remote branches.
- v3.8 immutability: `v3_8_status=REJECTED_AND_FORENSICALLY_CLOSED`,
  `v3_8_technical_dispatch_eligibility=REVOKED`, `v3_8_immutable=true`; no
  modification of v3.8 package/validation/review-binding/dispatch receipts/
  forensic receipts/CP-071/CP-072/CP-073/R_A/R_B.
- Forensic source bound: CP-073 evidence at
  `receipts/headless-lab-v3.8-dispatch-forensic/20260808T223904Z-v3-8-authorized-lab-dispatch/`;
  senior execution `0631a805-a40b-44f4-836c-f2a194c2b43e`; classifications
  F-A1/F-B1/F-B2/F-B3 verified supported.
- New generation: RUN `20260808T233306Z-v3-9-contract-alignment-package`;
  batch `20260808T233306Z-DOGFOOD004-LAB-V3_9`; leaves LEAF-LAB-A/B-V3_9;
  leases LEASE-…-LAB-A/B-V3_9 (planned, activated=false, authorization=false);
  result paths `receipts/headless-lab-results-v3.9/…`; branch templates
  `result/<future-run-id>-lab-a/b-v3_9`.
- Scope freeze v3.9: authorized semantic delta limited to F-A1 loader-context
  alignment, F-B2 semantic annotation comparison, F-B3 valid-input return-shape,
  F-B1 leaf-visible tracked-ELF reinforcement, normative-gate traceability,
  v3.9 IDs/paths/schemas; everything else semantically unchanged
  (`v3-8-v3-9-semantic-diff.json`).
- F-A1 corrected: finalizer inspects modules in the contractual package import
  context (tests/integration on sys.path, import headless_audio_lab.<module>),
  verifies resolved `__file__` == exact R checkout file; package-relative
  imports work (NA1 positive PASS); replay of R_A no longer fails for the
  relative import.
- F-B2 corrected: semantic annotation comparison via
  `_resolve_annotation`/`_normative_type`/`_types_equivalent` (supports both
  types.UnionType and typing.Union); evaluated annotations positive (NA2) and
  future annotations positive (NA3); genuine wrong types rejected (I2/I3).
- F-B3 corrected: return-shape gate uses real controlled WAV fixtures
  (valid source/capture), never nonexistent placeholder paths; valid-input
  returns dict (NA4/C1 positive); wrong return type rejected (C2);
  missing-file behavior diagnostic only (`missing_file_behavior=
  out_of_contract_for_this_gate`), never blocks E (C3).
- F-B1 retained fail-closed: R must contain zero tracked ELF (NA5 negative;
  replay R_B still REJECT); canonical controller build from candidate C
  sources; fixtures-as-.gitkeep-dirs NOT made normative — any representation
  satisfying path semantics with zero tracked ELF is accepted (D2 positive).
- Gate traceability (`gate-traceability-v3.9.json`): 25 gates with unique IDs,
  normative artifact, requirement, leaf_visible, verification method; finalizer
  traceability assertion (orphan blocking gate => no E; NA6 negative);
  principle NO EXECUTABLE REJECTION GATE WITHOUT NORMATIVE AUTHORITY.
- Leaf requirements (`leaf-requirements-v3.9.json`): LAB-A 10 requirements,
  LAB-B 12 requirements enumerating every rejection-capable requirement;
  future leaf prompt must say: "Read and satisfy leaf-requirements-v3.9.json.
  Anything not present there cannot become a new implementation rejection
  requirement during finalization unless it derives from another explicitly
  listed normative artifact."
- Independent verifier (`acceptance-verifier-v3.9.py`): verifies gate
  registry/traceability, normative artifact hashes vs manifest, finalizer
  output gate IDs; selftest split into INTERNAL_FAIL_CLOSED_TESTS +
  NORMATIVE_ALIGNMENT_TESTS; SELFTEST_GLOBAL never claimed as normative proof.
- Selftest: `SELFTEST_INTERNAL_CONSISTENCY=PASS`,
  `SELFTEST_NORMATIVE_ALIGNMENT=PASS`, `SELFTEST_GLOBAL=PASS` (NA1..NA7,
  I2/I3). Adversarial matrix A1..T2 executed (A1/A2/B1/B2/C1/D2/T1-positives,
  A3/B3/C2/D1/T1/T2-rejects). Regression: 8/8 literal argv byte-identical;
  ownership/identity/fail-closed/tracked-ELF/silent-WAV coverage preserved.
- Rejected-R forensic replay (diagnostic only): R_A no longer rejected for
  F-A1 (remaining failure: osc_probe method signature drift — genuine baseline
  non-conformance); R_B still rejected for tracked ELF (F-B1) and
  diff_snapshots signature drift, with return_shape=PASS (F-B2/F-B3
  corrected). R_A/R_B remain forensic-only, not integration eligible, not
  reusable as future dispatch results.
- Package frozen: `receipts/headless-lab-v3.9/` (21 operational files),
  manifest `package-payload-manifest-v3.9.json`,
  **V3_9_PACKAGE_PAYLOAD_MANIFEST_SHA256 =
  `71596b8472b459940ee79fe9df8d3687dd3dbbdf5bf4696d897a9e4a1bdb3bba`**,
  supersedes=v3.8 (`bb4182…`), supersession findings F-A1/F-B2/F-B3 +
  F-B1 leaf-visibility, zero missing/extra, self-excluded, review excluded.
  PAYLOAD_FROZEN=true; no byte changes after freeze.
- Validation receipts under `receipts/headless-lab-v3.9-validation/` (22
  files: preservation, forensic-source binding, semantic diff, traceability,
  leaf-requirements visibility, F-A1/F-B1/F-B2/F-B3, replay, selftest groups,
  adversarial matrix, command hash, ownership, identity, controlled build,
  publication create-only, postpublish, full completion, package validation).
- **Triple senior review (merge-gate, frozen bytes)**: 3 × `accept`,
  blocking_findings=0, required_actions=0, safe_to_merge=true.
  - Review 1 functional continuity: exec `5e86f9c8-5254-485a-bee1-85cac9708b81`
  - Review 2 normative alignment/process: exec `753f1500-6043-47ed-a50f-b62251c28ae9`
  - Review 3 adversarial validity: exec `55904bee-05f3-4acc-b76c-6f3a97a54ab0`
- Review binding (`receipts/headless-lab-v3.9-review-binding/v3-9-review-binding.json`):
  created_after_reviews=true, self_reviewed=false, manifest exact, payload
  hashes exact, 3 execution IDs, 3 fingerprints, 3 verdict hashes,
  accept_count=3, blocking=0, required=0, non-circular (no future commit SHA).

## Verification

- Selftest both groups PASS; adversarial matrix all cases confirmed;
  replay expectations met; manifest count/hashes verified; static-validate
  finalizer + verifier PASS; all JSON parse; all Python py_compile.
- Traceability: 25 gates, unique IDs, all entries have normative artifact and
  requirement; leaf-visible entries covered in leaf-requirements; orphan gate
  rejected (NA6).
- Triple review outputs: status COMPLETED, detailed_status
  VALID_ADVISORY_VERDICT for all three; verdict accept with zero blocking and
  zero required actions; exact manifest binding (71596b84…).

## Current state

- `v3_8_status = REJECTED_AND_FORENSICALLY_CLOSED`
- `v3_8_technical_dispatch_eligibility = REVOKED`
- `v3_9_status = EXACT_BYTES_REVIEWED_NOT_DISPATCHED`
- `v3_9_technical_dispatch_eligibility = PASS`
- `v3_9_dispatch_authorized = false`; `leases = planned`; `leases_activated = false`
- `leaves = 0`; `lab_execution_worktrees = 0`; `remote_result_branches = 0`
- `integration_authorized = false`; `D0_D1_D2_authorized = false`;
  `PR_merge_authorized = false`; `force_push_authorized = false`
- R_A / R_B: forensic-only, not integration eligible
- Corrected review binding (v3.8) remains valid (3/0/0); v3.9 binding created.

## Risks and unresolved questions

- The only next human decision is authorize or reject exactly ONE fresh v3.9
  LAB-A/LAB-B dispatch.
- Future leaf prompts MUST reference leaf-requirements-v3.9.json.
- Any required package change after freeze => V3_9_FROZEN_PACKAGE_REJECTED and
  a new generation (v3.10, CP-075+).

## Next executable action

Publish the frozen v3.9 package + validation receipts + review binding +
CP-074 + CURRENT to the control-plane source branch (fast-forward only, no
fork-main, no force); update PR metadata; observe exact-head CI; then the
human decision: authorize/reject exactly one fresh v3.9 dispatch.

## Open first

- PR #34 exact head (after control-plane commit)
- `receipts/headless-lab-v3.9/package-payload-manifest-v3.9.json` on GitHub
- `receipts/headless-lab-v3.9-review-binding/v3-9-review-binding.json`
- Project control plane run (new head)
- Audio integration core run (new head)
- CP-074

## Safe reference point

- predecessor_head = `cc1853a084d72e5d322d9304b25dbe63040a17d5`
- candidate = `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- v3_8_manifest = `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`
- v3_9_manifest = `71596b8472b459940ee79fe9df8d3687dd3dbbdf5bf4696d897a9e4a1bdb3bba`
- v3_9_batch = `20260808T233306Z-DOGFOOD004-LAB-V3_9`
- review_executions = [5e86f9c8, 753f1500, 55904bee]
- package_path = `receipts/headless-lab-v3.9/`
