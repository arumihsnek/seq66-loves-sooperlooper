# Current checkpoint — V3.10 true-leaf provenance recovery package (CP-078)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-09-CP-078-v3-10-true-leaf-provenance-recovery-package.md`

Checkpoint ID: `CP-078`
Checkpoint date: 2026-08-09
Supersedes: `CP-077` (CP-077 remains immutable)
Phase: `V3.10 true-leaf provenance recovery package`
Current phase: `phase-5-exact-recording`
Active task: P5-005 (M5 exact musical recording — V3.10 provenance package)
Status: `EXACT_BYTES_REVIEWED_NOT_DISPATCHED — HUMAN_DECISION_PENDING`

## Current state

- `CURRENT -> CP-078`.
- `V3_9_STATUS=FUNCTIONALLY_PASS_WORKFLOW_PROVENANCE_INVALID_NOT_INTEGRATED`.
- `V3_9_SECOND_DISPATCH_AUTHORIZED=false`;
  `V3_9_LEASE_REUSE_AUTHORIZED=false`.
- `V3_10_STATUS=EXACT_BYTES_REVIEWED_NOT_DISPATCHED`.
- `V3_10_TECHNICAL_DISPATCH_ELIGIBILITY=PASS`;
  `V3_10_DISPATCH_AUTHORIZED=false`.
- `fresh_leases=planned`; `fresh_leases_activated=false`; `leaves=0`;
  `v3_10_result_branches=0`.
- `NEW_PRODUCT_IMPLEMENTATION_AUTHORIZED=false`;
  `INTEGRATION_AUTHORIZED=false`; `D0_D1_D2_AUTHORIZED=false`;
  `PR_MERGE_AUTHORIZED=false`.
- `RESULT_BRANCH_DELETION_AUTHORIZED=false`;
  `FORCE_PUSH_AUTHORIZED=false`; `FORK_MAIN_REWIND_AUTHORIZED=false`.
- V3.10 payload is frozen at manifest SHA-256
  `4ca2f2686f36e5b3455d2ec4bb6b5edb4e22099e1e1b13f0cfbb4899160c550d`.
- Planned publication: new draft recovery PR from exact fork-main
  `c965340f6be7655e2f4d796582e5acd7dc163a82`, branch
  `recovery/v3-10-true-leaf-provenance-20260809T205317Z`.
- No V3.10 leaves, leases, result branches, R commits, integration, or merge
  has occurred.

## Verification summary

- Triple senior review: 3× `accept`, blocking=0, required=0; non-circular
  binding `created_after_reviews=true`, `self_reviewed=false`.
- Functional semantic drift: `0`.
- V3.9 result branches preserved exactly; no V3.9 evidence reused as an input.

## Next action

Publish the frozen package and require exact-head Project control plane and Audio
integration core CI on the new draft PR. The next human decision is exactly one
fresh V3.10 LAB-A/LAB-B dispatch authorization with true leaves; that decision
excludes product integration.
