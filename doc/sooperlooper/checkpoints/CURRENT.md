# Current checkpoint — v3.8 package identity coherence (frozen, not dispatched)

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-08-CP-068-v3-8-package-identity-coherence.md`

Checkpoint ID: `CP-068`
Checkpoint date: 2026-08-08
Phase: `phase-5-exact-recording`
Active task: P5-007 (M5 exact musical recording); sub-task: v3.8 review-binding + publication (controller-owned, STOP gate)
Status: `v3.8 EXACT_BYTES_REVIEWED_NOT_DISPATCHED — HUMAN_DECISION_PENDING`
Branch: `fork-main`
Exact head (PR #34): `3090f2e078ca29762f7654b9e5a872f58ad4ac5b`

## Verification summary

- v3.7 byte-exact preserved (manifest SHA-256 `7ce975187bee7503dc43959a11cacdbc90f51cb6072860215af40d99a4a441d3`).
- v3.7 dispatch eligibility REVOKED (`v3_7_status=EXACT_BYTES_REVIEWED_REJECTED_FOR_DISPATCH`).
- v3.8 operational payload frozen: 19/19 files, manifest SHA-256 `bb4182ada0f1bf449741c05a749d91d4433e6fb09bf281d3f61c4268dc509c18`, zero missing, zero extra.
- Cross-manifest coherence: 59 sources checked, 0 mismatches, verdict PASS.
- Selftest: 32/32 (declared=executed=passed, failed=0, skipped=0).
- Identity negative matrix: 15/15 + control PASS (real CLI per case).
- S1-S11 harness: all PASS (fail-closed 8/8, runtime contract PASS, controlled build PASS, publication create-only PASS, postpublish binding PASS, full completion PASS, inherited negative matrix 29/29, static source PASS, package validation 19/19, functional continuity PASS).
- Part3: command hashes 8/8 (4 LAB-A + 4 LAB-B), ownership diff LAB-A + LAB-B PASS, runtime contract negatives 12/12.
- Triple senior review (frozen manifest + 19 payload hashes):
  - functional-continuity (final-review): `accept`, 0 blocking, 0 required.
  - package-identity-coherence (integrated-review): `accept`, 0 blocking, 0 required.
  - adversarial-identity-validity (risk-audit): `continue`, 0 blocking, 0 required (all 6 Q answered SÍ, residual risks acknowledged, controls enumerated).
- Review binding (`package-review-binding-v3.8.json`): `three_accepts=True`, `blocking=0`, `required_actions_total=0`.
- CI runs 31226194220 (control plane) + 31226194221 (audio integration core) on PR #34 head `3090f2e0…`: completed / success / exact head.

## Authorization scope (this checkpoint)

- `v3_7_dispatch_authorized = false` (REVOKED).
- `v3_8_dispatch_authorized = false`.
- `leases = planned`, `leases_activated = false`.
- `leaves = 0`, `lab_execution_worktrees = 0`, `remote_result_branches = 0`.
- `integration_authorized = false`, `D0_D1_D2_authorized = false`, `PR_merge_authorized = false`.
- Single-writer lease: PID `1915047` holds FD 9 on
  `/home/ubuntu/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`
  (reacquired 2026-08-08T20:27:33Z).

## Next action

HUMAN_DECISION_PENDING: authorize or reject **exactly one** fresh v3.8 LAB-A/LAB-B
dispatch. No leaves, no worktrees, no integration, no D0/D1/D2, no PR merge
authorized by this checkpoint. See `CP-068` for full context.
