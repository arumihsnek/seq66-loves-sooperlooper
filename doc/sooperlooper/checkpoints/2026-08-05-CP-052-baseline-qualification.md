# CP-052 — Baseline qualification for headless-lab recovery

Checkpoint ID: `CP-052`
Checkpoint date: 2026-08-05
Phase: `phase-5-exact-recording`
Active task: `P5-005`

## Objective

Qualify `OLD_SAFE_BASE` (`30561153aae9a47553332c601f6be8a3c99122fd`) as a CI-green baseline for the headless-lab recovery pre-dispatch package (`20260804T194037Z-headless-lab-recovery`). Two isolated repairs: (1) project-control validator compatibility with the historical forensic-closure checkpoint CP-051; (2) audio-core workflow JACK development dependency.

## Completed

- BASE-GOV: explicit narrow historical exemption in `contrib/scripts/validate-project-control.py` (`HISTORICAL_FORENSIC_CHECKPOINTS` + `is_historical_forensic_closure` requiring path in immutable list AND structural markers `## Why CP-` / `## Stop state`); durable pytest regression in `tests/control/validate_project_control_test.py` (5 tests).
- BASE-CI: `.github/workflows/audio-core.yml` installs `libjack-jackd2-dev` (provides `jack.pc` and `jack/jack.h`).
- Integration `merge --no-ff` of both result heads (`76a05870`, `f7d1c05c`) into `NEW_SAFE_BASE = 509538784afc2b828f2d922f65cf8ca3a39b5ee7`.
- PR #34 (draft, base fork-main) with CI green on the exact head.
- R2 exact-head review: verdict `accept`, 0 findings (execution `81a08944-a34f-4ac3-885f-937b27ce8f29`).

## Verification

- CI exact-head `509538784afc2b828f2d922f65cf8ca3a39b5ee7`: `validate-control-plane` success; `compile-and-test` success.
- Integration matrix (controller, exact head): validator PASS; pytest 5/5; `pkg-config --cflags --libs jack` PASS; integration test 16/16 PASS; transport matrix 180/180 PASS.
- CP-048, CP-049, CP-050, CP-051 byte-identical vs OLD_SAFE_BASE (`git diff --quiet` per path).
- Diff of candidate vs OLD_SAFE_BASE limited to the 3 owned paths.
- Envelopes `autonomous-session-result/v1` of BASE-GOV and BASE-CI validated with `session_contract.py result` (VALID).

## Current state

- `baseline qualified for headless-lab recovery`
- `LAB-A/LAB-B not dispatched`
- `D0/D1/D2 not executed`
- `old predispatch package requires rebinding`

## Risks and unresolved questions

- None blocking. PR #34 remains draft awaiting the human decision on the regenerated pre-dispatch package; merge of PR #34 itself is a separate decision.

## Next executable action

Regenerate the pre-dispatch package `20260804T194037Z-headless-lab-recovery` bound to `NEW_SAFE_BASE` (recompute safe-base collisions, revalidate port plan and LAB API contract, regenerate scope/manifest/dispatch/lease plans, repeat senior review on new hashes, leases `planned`, zero worktrees/leaves), then present the human decision.

## Open first

`external architect/human reviews the accepted port plan, ownership split, literal tests and pre-dispatch receipts; only after approval create isolated worktrees, activate leases and dispatch LAB-A/LAB-B` — now bound to `NEW_SAFE_BASE` with green exact-head CI.

## Safe reference point

`NEW_SAFE_BASE = 509538784afc2b828f2d922f65cf8ca3a39b5ee7` (`PR #34`, branch `integration/baseline-qualification-20260805`); `OLD_SAFE_BASE = 30561153aae9a47553332c601f6be8a3c99122fd`; RUN `20260805T015358Z-seq66-baseline-qualification`.
