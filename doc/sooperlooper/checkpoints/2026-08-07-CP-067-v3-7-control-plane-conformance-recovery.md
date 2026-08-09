# Checkpoint CP-067 — V3.7 control-plane conformance recovery

Checkpoint ID: CP-067
Checkpoint date: 2026-08-07
Supersedes: CP-066 (immutable)
Phase: phase-5-exact-recording

## Objective

Restore "Project control plane" workflow compatibility after CP-066 omitted the
required checkpoint headings. The frozen v3.7 package is byte-identical and is
NOT modified by this recovery. This checkpoint exists only to repair the durable
control-plane/checkpoint layer so that `validate-project-control` passes again.

- No functional/process package behavior is changed.
- No v3.7 payload byte is touched.
- No LAB-A/LAB-B execution, no dispatch.

## Completed

- CP-066 remains byte-identical (immutable historical state; not edited).
- v3.7 package payload remains byte-identical
  (`package-payload-manifest-v3.7.json` SHA-256 unchanged).
- v3.7 review binding remains unchanged.
- CP-067 adds no functional/process package behavior — control-plane
  conformance only.
- `CURRENT.md` now points to CP-067.

## Verification

Run locally from a clean exact tree:

```bash
python3 contrib/scripts/validate-project-control.py
```

Require exit 0 (Project-control validation passed). The exact-head CI result is
observed separately after publication; no exact-head CI success is claimed until
GitHub confirms it.

## Current state

- v3_7_status = EXACT_BYTES_REVIEWED_NOT_DISPATCHED
- v3_7_package_payload_manifest_sha256 =
  `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3`
- dispatch_authorized = false
- integration_authorized = false
- D0_D1_D2_authorized = false
- PR_merge_authorized = false
- leases_activated = false
- lab_execution_worktrees = 0
- leaves = 0
- remote_result_branches = 0

Technical state is NOT upgraded because CP-067 validates; dispatch remains
unauthorized.

## Risks and unresolved questions

- exact_head_ci = PENDING
- dispatch_decision = NOT_REACHED

The v3.7 package is NOT described as CI-green until new exact-head runs succeed.

## Next executable action

Observe exact-head CI after publishing CP-067 (Project control plane and Audio
integration core on the new publication SHA). Not dispatch.

## Open first

- PR #34 exact head
- Project control plane
- Audio integration core
- v3.7 package manifest
- CP-067

## Safe reference point

- previous_publication_head = `30592aa1dd24f4c1e0bdcfd86b167ad2a0d0f591`
- frozen_v3_7_package_manifest =
  `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3`
