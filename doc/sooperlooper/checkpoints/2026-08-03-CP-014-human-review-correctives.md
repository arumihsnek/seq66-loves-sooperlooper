# Checkpoint: Human-review corrective audit — three blockers

## Checkpoint ID: `CP-014`
## Checkpoint date: 2026-08-03
## Checkpoint type: immutable audit baseline

## Objective
Human review has identified three functional blockers and several gate
incoherencies. This checkpoint captures the audit baseline and corrective
plan. No PRs are approved, merged, or marked ready during this cycle.

## Blockers found by human review

### B-001: M1-005 subscription wire protocol incorrect
- Current `subscribe_loop/global` sends only `s:control` (one string arg)
- SooperLooper contract requires `s:control s:return_url s:return_path`
- `subscribe_loop_auto` sends interval as `float` (`"sf"`) instead of `int` (`"sif"`)
- No `unregister` methods exist
- No shared receiver for callback delivery
- Ping is blocking, not async/worker-bound

### B-002: M1-006 reconciliation lacks generation awareness
- `pending_operation` has no engine generation field
- Reconciler receives only `loop_index` (int), not full typed operation
- No immutable operation copy during reconciliation
- No check that operation didn't change during external callback
- No protection against confirming stale operations after restart
- `outcome()` returns `cancelled` for unknown UUID — should be distinct from cancelled
- Generic state query can produce false confirmation

### B-003: M1-007 reordered event semantics not tested
- Current fault injection test does not prove late packets are rejected
- No monotonic timestamp/sequence check for coalescible telemetry
- No sequence/generation ordering for transition-critical events
- No documentation of arrival-ordered vs timestamp-ordered fields

## Gate incoherencies
- CP-013 declares Phase 1 complete prematurely
- OSC-005 verified but subscription protocol is wrong
- OSC-006/STATE-007/FAIL-003 need downgrade
- TEST-003/FAIL-004 need coverage adjustment
- M1-005A marked in_progress after being merged
- Duplicate M1-008 entries
- Active task header incorrect
- capabilities/missing_capabilities stale

## Corrective plan
1. `fix/m1-005b-subscription-wire-protocol` — fix subscription messages
2. `fix/m1-006b-confirmation-correlation` — generation-aware reconciliation
3. `fix/m1-007b-reordered-event-semantics` — real event ordering policy

## Rules during corrective cycle
- No PR approval, ready marking, or merge
- No Phase 2 work
- No modification of old checkpoints
- CP-013 remains as immutable historical record

## Evidence
- 13 PRs merged (PR #1-#13)
- PR #14 is open draft with no file changes
- Human review identified B-001, B-002, B-003

## Next immediate action
Begin M1-005B: create fix/m1-005b-subscription-wire-protocol branch.
