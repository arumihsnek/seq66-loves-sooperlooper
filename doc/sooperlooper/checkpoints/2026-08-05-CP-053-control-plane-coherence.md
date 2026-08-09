# CP-053 — Control-plane coherence correction

Checkpoint ID: `CP-053`
Checkpoint date: 2026-08-05
Phase: `phase-5-exact-recording`
Active task: `P5-005`

## Objective

Repair an externally detected control-plane incoherence on the exact remote head of PR #34: PROJECT-MANIFEST `next_milestone.active_task` said `P5-007` while CP-052 and CURRENT.md said `P5-005`, and CURRENT.md described `50953878…` as the current PR head although the remote head had advanced to the evidence commit `95b11e4d…`. Correction must keep the candidate unchanged and only mutate control-plane surfaces (new commit descendant of `95b11e4d…`).

## Completed

- PROJECT-MANIFEST.json: `next_milestone.active_task` corrected `P5-007 → P5-005`, `active_task_status` stays `in_progress`. `P5-007` remains deferred (per CP-051/CP-052).
- WORK-QUEUE.md: `P5-005` status corrected `done → in_progress` (required for validator consistency with the active task; the headless-lab recovery is the executing workstream); `P5-007` status corrected `in_progress → deferred` with note.
- CURRENT.md: rewritten to point at CP-053 and to separate explicitly: `candidate_head = 509538784afc2b828f2d922f65cf8ca3a39b5ee7`, `evidence/control-plane head = 95b11e4d…` (previous evidence head; current evidence head = SHA of this commit, documented in receipt post-push). `50953878…` is no longer described as the current PR head.
- PR #34 body updated (no history rewrite) to separate NEW_SAFE_BASE/candidate head, current evidence/control-plane head, exact-head CI of candidate, exact-head CI of evidence head, R2 accept, regen review accept, human decision pending; obsolete "pending" section removed.

## Verification

- `python3 contrib/scripts/validate-project-control.py` → PASS (exit 0) on the exact new head.
- Fresh semantic receipt (ad-hoc, not canonical test-suite evidence): PROJECT-MANIFEST active task == CURRENT active task == CP-053 active task == `P5-005`; CURRENT candidate head == `509538784afc2b828f2d922f65cf8ca3a39b5ee7`; CURRENT evidence head == `git rev-parse HEAD`; PR #34 remote head == `git rev-parse HEAD`; CP-048..CP-052 byte-identical; LAB leases remain `planned`; LAB worktrees created == 0; LAB leaves dispatched == 0; D0/D1/D2 executed == false.
- Exact-head CI on the new head: `Project control plane` success; `Audio integration core` success.
- R2 and regen review bundles unchanged (identical hashes: r2 bundle `2bbee68d…`, regen bundle `c6e0a040…`; contracts and candidate unchanged — no re-review required).

## Current state

- `CP-052 remains immutable`
- `candidate_head = 509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- `previous evidence head = 95b11e4d5cb08789a2eaeb131925beea1951cd25`
- `correction = manifest P5-007 → P5-005 and PR-head identity separation`
- `LAB-A/LAB-B not dispatched`
- `leases remain planned`
- `D0/D1/D2 not executed`
- `human authorization still pending`

## Risks and unresolved questions

- None new. PR #34 remains draft; the regenerated pre-dispatch package is bound to `NEW_SAFE_BASE` and still awaits the human decision. Merge of PR #34 is a separate decision.

## Next executable action

Present the human decision: authorize or reject the regenerated LAB-A/LAB-B package against `NEW_SAFE_BASE 50953878…` with green exact-head CI. Only after approval: create isolated worktrees, activate leases and dispatch LAB-A/LAB-B (re-verify clean worktree + exact HEAD before activation).

## Open first

`external architect/human reviews the accepted port plan, ownership split, literal tests and pre-dispatch receipts; only after approval create isolated worktrees, activate leases and dispatch LAB-A/LAB-B` — bound to `NEW_SAFE_BASE` with green exact-head CI.

## Safe reference point

- Candidate / NEW_SAFE_BASE: `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- Previous evidence head: `95b11e4d5cb08789a2eaeb131925beea1951cd25`
- Current evidence/control-plane head: SHA of this commit (documented in receipt post-push; previous = `95b11e4d5cb08789a2eaeb131925beea1951cd25`)
- PR #34 branch: `integration/baseline-qualification-20260805` → `fork-main`
- RUN: `20260805T015358Z-seq66-baseline-qualification`
