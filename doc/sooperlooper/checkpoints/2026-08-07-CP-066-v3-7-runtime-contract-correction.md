# Checkpoint CP-066 — V3.7 runtime contract correction package

Checkpoint ID: `CP-066`
Checkpoint date: 2026-08-07
Supersedes: `CP-064` (2026-08-06, immutable) — v3.6 package (20260806T234554Z) was prepared and
reviewed but REJECTED for dispatch (findings fc-f1/fc-f2/fc-f3) and remains frozen/immutable.
Phase: `phase-5-exact-recording` (no dispatch, no integration)

## v3.6 bytes preserved

- RUN: `~/.hermes/runs/20260806T234554Z-v3-6-fail-closed-package` (local, immutable)
- v3.6 payload frozen at manifest `b2872665…`; v3.6 senior verdicts returned `changes_required`
  (fc-f1 runtime contract enforcement, fc-f2 command-hash/ownership evidence closure, fc-f3)
- v3.6 NOT unfrozen, NOT patched, NOT published; v3.6 dispatch eligibility NOT granted.

## v3.7 correction authorization

- `V3_7_PACKAGE_PREPARATION_AUTHORIZED=true`
- `V3_7_DISPATCH_AUTHORIZED=false`; `LEAF_EXECUTION_AUTHORIZED=false`;
  `LEASE_ACTIVATION_AUTHORIZED=false`; `INTEGRATION_AUTHORIZED=false`;
  `D0_D1_D2_AUTHORIZED=false`; `PR_MERGE_AUTHORIZED=false`; `WORKTREE_DELETION_AUTHORIZED=false`

## Candidate / batch / leases / result paths

- Candidate head: `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- PR: #34 (open, draft, mergeable; PR head `55b028cc6f1b2c9c9a0ba645542f331ca42ee869`)
- Batch: `BATCH-20260807T161935Z-DOGFOOD004-LAB-V3_7`
- Leaves: `LEAF-LAB-A-V3_7`, `LEAF-LAB-B-V3_7`
- Leases: `LEASE-20260807T161935Z-LAB-A-V3_7`, `LEASE-20260807T161935Z-LAB-B-V3_7` — status `planned`,
  `activated=false`, `authorization=false`
- Result paths (planned, not created): `receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json`,
  `receipts/headless-lab-results-v3.7/lab-b-controller-evidence.json`
- Planned result branches (NOT created): `result/<future-run-id>-lab-a-v3_7`,
  `result/<future-run-id>-lab-b-v3_7`

## Closure of fc-f1 / fc-f2 / fc-f3

1. **fc-f1 — FULL RUNTIME CONTRACT ENFORCEMENT**: finalizer now executes `runtime_full_contract`
   on a real detached checkout of R: imports modules, inspects functions/classes/methods/
   signatures, checks parameter ORDER/KIND/DEFAULTS and return annotations, class fields and
   method signatures, executes return-shape checks, executes silent WAV requiring
   `verification=FAIL`. Any mismatch → REJECT, no E.
2. **fc-f2 — COMMAND-HASH / OWNERSHIP EVIDENCE CLOSURE**: `command_hash_evidence` recomputes
   sha256 of canonical argv serialization for every literal test; `declared == recomputed` is a
   mandatory gate (mismatch with test exit 0 → REJECT, no E). `ownership_diff_evidence` compares
   the COMPLETE `C..R` diff against `owned_paths` + `forbidden_paths` (changed_not_owned=[] and
   forbidden_changed=[] required). Both evidence sections are recorded in every E and in the
   acceptance verifier's independent recomputation.
3. **fc-f3 — validation evidence**: 12 new runtime-contract negative cases (N13..N24), full
   negative matrix 29/29 inherited + 12/12 runtime = no coverage reduction, selftest 32/32.

## Validation results (all PASS)

- `selftest-results.json`: declared=executed=passed=32, failed=0, skipped=0, `SELFTEST_GLOBAL=PASS`
- `static-source-validation.json`: `STATIC_VALIDATOR_GLOBAL=PASS`, production_findings=0,
  debug_markers=0; sanity bad-snippet 6/6 rejected
- `finalizer-fail-closed.json`: 8/8 precondition failures → exit!=0, E absent, evidence not added,
  no commit, remote unchanged
- `command-hash-verification-lab-a/b.json` (+ aggregate): 4/4 per lab, `total_8_of_8_match=true`
- `ownership-diff-verification-lab-a/b.json`: complete C..R diff, changed_not_owned=[],
  forbidden_changed=[]
- `runtime-contract-negative-matrix.json`: 12/12 runtime mutations rejected (real CLI, exit
  recorded, side effects)
- `runtime-contract-inspection.json` + `constraint-coverage.json`: 31 constraints, structural +
  return-shape + constraint coverage PASS
- `controlled-build.json`, `publication-create-only.json`, `postpublication-binding.json`,
  `full-completion.json`: PASS
- `functional-continuity.json`: no functional drift vs v3.6 baseline; 8 literal tests (4 per lab)
  exact; `supersedes=v3.6`
- `v3.6-frozen-rejection-preservation.json`, `v3.6-v3.7-semantic-diff.json`: v3.6 preserved;
  delta limited to fc-f1/fc-f2/fc-f3 + renames

## Package payload

- 17 operational artifacts frozen at
  `receipts/headless-lab-v3.7/` (operational_file_count=17, zero_missing, zero_extra)
- `V3_7_PACKAGE_PAYLOAD_MANIFEST_SHA256 = 7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3`
- `PAYLOAD_FROZEN=true`; supersedes: v3.6 (manifest b2872665…); supersession findings
  fc-f1/fc-f2/fc-f3

## Triple senior review — three accepts 0/0

1. `seq66-lab-v3-7-functional-continuity` (full runtime contract enforcement): **accept** 0/0
   (execution `eedfae43-446a-496a-962d-edabe5fbfb5e`)
2. `seq66-lab-v3-7-fail-closed-process`: **accept** 0/0 (execution `6b4b5dad-47f6-4c12-b7b9-a03f2d79e4db`;
   run under mission-id -v3 after the original mission-id exhausted its replacement allowance on
   protocol failures — no verdict ever produced; evidence-completion iteration on command-hash
   receipt aggregate fields; payload byte-identical)
3. `seq66-lab-v3-7-adversarial-validity`: **accept** 0/0 (execution
   `0e3bdc1d-06b8-421f-9ca1-d64c3beb09e4`; run under -v3 after v1 changes_required was closed by
   evidence completion — source slices + full selftest transcript; payload byte-identical)

- All three bound `bound_manifest_sha256 = 7ce97518…` and `bound_payload_hashes` = exact 17
- Binding: `receipts/headless-lab-v3.7-review-binding/package-review-binding-v3.7.json`
  (created_after_reviews=true, self_reviewed=false, dispatch_authorized=false)

## Technical eligibility

- v3.6: `v3_6_status=EXACT_BYTES_REVIEWED_REJECTED_FOR_DISPATCH`;
  `v3_6_technical_dispatch_eligibility=REVOKED`
- v3.7: `v3_7_status=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`;
  `v3_7_technical_dispatch_eligibility=PASS`
- `v3_7_dispatch_authorized=false`; leases=planned; leases_activated=false;
  lab_execution_worktrees=0; leaves=0; remote_result_branches=0
- `integration_authorized=false`; `D0_D1_D2_authorized=false`; `PR_merge_authorized=false`

## Next action

Human decision: authorize or reject EXACTLY ONE fresh v3.7 LAB-A/LAB-B dispatch using the exact
frozen v3.7 package (new branches, new worktrees, new leases). That authorization will NOT
authorize integration, D0/D1/D2, or merge.
