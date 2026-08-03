# CP-015 — Phase 1 correctives published for gate verification

Checkpoint ID: `CP-015`  
Checkpoint date: 2026-08-03  
Phase: `phase-1-protocol-core`  
Active task: `M1-008`  
Branch: `gate/m1-008-phase-1`  
Draft PR: `#14`

## Objective

Publish and verify the M1-005B, M1-006B and M1-007B corrective work
before the human Phase 1 integration decision.

The gate must demonstrate the corrected subscription wire protocol,
generation-aware confirmation correlation and reordered-event semantics
without declaring Phase 1 complete prematurely.

## Completed

- M1-005B corrects subscription messages:
  - exact loop/global paths;
  - `sss` and `siss` signatures;
  - callback URL and callback path;
  - integer auto-update interval;
  - unregister and cancellation paths.
- M1-006B adds:
  - engine generation to pending operations;
  - immutable reconciliation requests;
  - generation cancellation;
  - unknown-UUID handling;
  - reconciliation callbacks outside internal locks.
- M1-007B adds:
  - monotonic timestamp handling for coalescible observed values;
  - stale-event rejection;
  - explicit arrival-order semantics for transitions;
  - duplicate and reordered-event tests.
- The corrective commits were published on
  `gate/m1-008-phase-1`.
- PR #14 now contains real changes rather than an empty gate marker.

## Verification

Local ad-hoc verification before publication:

- M1-005B subscription wire protocol: PASS, 16 tests.
- M1-006B generation-aware confirmation: PASS, 25 tests.
- M1-007B reordered observed events: PASS, 38 tests.
- Cross-cut fault injection: PASS, 36 tests.
- Existing protocol no-regression tests: PASS.
- Compilation used:
  `-Wall -Wextra -Wpedantic -Werror`.

Remote verification on head
`572a5fa67d3a793ec182a51cdaefd980654245b9`:

- Project control plane run `30811828427`: FAIL.
  Cause: manifest active branch/state mismatch and this checkpoint missing
  the mandatory checkpoint headings.
- Audio integration core run `30811828663`: FAIL.
  Cause: focused compile commands linked `sooperlooper_client.cpp`
  without `sooperlooper_receiver.cpp`, leaving `started()` and `port()`
  undefined.

These failures are evidence of gate-infrastructure defects, not successful
Phase 1 verification. A new commit and fresh CI runs are required.

## Current state

- Branch: `gate/m1-008-phase-1`.
- Draft PR: `#14`.
- Base: `fork-main`.
- M1-008 status: `review`.
- Working tree was clean before this gate-repair change.
- Phase 2 is not authorized.
- PR #14 must remain draft.
- The previous failed runs remain historical evidence.
- Fresh checks must run on the next exact head SHA.

## Risks and unresolved questions

- The pinned real-SooperLooper/JACK-dummy smoke has not yet been executed
  on the final corrective head.
- TSAN cannot currently execute in the known ARM64 environment.
- The complete GitHub Actions suite has not yet passed from a clean checkout.
- Subscription timing remains to be validated on target hardware.
- The final global senior review has not yet been recorded.
- Human approval is still required before declaring Phase 1 complete.

## Next executable action

1. Repair `.github/workflows/audio-core.yml` so every target that links
   `sooperlooper_client.cpp` also satisfies its receiver dependency.
2. Validate the manifest and checkpoint locally.
3. Compile and run the three previously blocked focused binaries.
4. Commit and push the gate-infrastructure repair.
5. Wait for fresh CI on the exact new head.
6. Run the pinned real-engine smoke.
7. Update PR #14 and create the next immutable gate checkpoint.
8. Request final `codex-senior-consult` review.
9. Stop for human Phase 1 approval.

## Open first

1. `PROJECT-MANIFEST.json`
2. `doc/sooperlooper/checkpoints/CURRENT.md`
3. this checkpoint
4. `doc/sooperlooper/WORK-QUEUE.md`
5. PR #14 and checks for its exact head
6. `.github/workflows/audio-core.yml`
7. only the corrective implementation and tests relevant to a failing gate

## Safe reference point

Published corrective head before the CI repair:

`572a5fa67d3a793ec182a51cdaefd980654245b9`

PR:

`https://github.com/arumihsnek/seq66-loves-sooperlooper/pull/14`

The working tree was clean when that head was pushed. No force push,
history rewrite or destructive cleanup is required.
