# Current checkpoint — Control-plane coherence correction

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-05-CP-053-control-plane-coherence.md`

Checkpoint ID: `CP-053`
Checkpoint date: 2026-08-05
Phase: `phase-5-exact-recording`
Active task: `P5-005`
Status: `in_progress`
Branch: `integration/baseline-qualification-20260805`

## Head identity (explicit separation)

- `candidate_head` (NEW_SAFE_BASE): `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- `evidence/control-plane head`: `95b11e4d5cb08789a2eaeb131925beea1951cd25` (previous evidence head; current evidence head = SHA of this commit, documented in receipt post-push)
- `PR #34 remote head`: see `gh pr view #34 --json headRefOid` after push

## Baseline state

- `baseline qualified for headless-lab recovery` — CI exact-head green on the candidate (`50953878…`) and on the evidence head (`95b11e4d…`, previous) and on the current head (this checkpoint).
- `LAB-A/LAB-B not dispatched`
- `D0/D1/D2 not executed`
- `old predispatch package requires rebinding` — done: regenerated package v2 bound to `NEW_SAFE_BASE`; leases `planned`; human authorization still pending.

## Durable evidence

- `doc/sooperlooper/checkpoints/2026-08-05-CP-052-baseline-qualification.md` (immutable)
- `doc/sooperlooper/checkpoints/2026-08-05-CP-053-control-plane-coherence.md` (immutable)
- PR #34 (`integration/baseline-qualification-20260805` → `fork-main`)
- RUN `20260805T015358Z-seq66-baseline-qualification` (receipts, reviews, contracts, leases, results)

## Next action

Present the human decision (authorize/reject the regenerated LAB-A/LAB-B package against `NEW_SAFE_BASE` with green CI). P5-007 remains deferred per CP-051/CP-052.
