# Checkpoint CP-075 — V3.9 authorized lab dispatch completed

Checkpoint ID: `CP-075`
Checkpoint date: 2026-08-09
Supersedes: `CP-074` (2026-08-08, immutable)
Phase: `phase-5-exact-recording`

## Objective

Execute the single authorized fresh v3.9 LAB-A/LAB-B dispatch using the frozen,
triple-reviewed v3.9 package (CP-074), publish the results, run the
integration steps D0/D1/D2, and prepare the exact-head senior merge review for
PR #34. Dispatch authorization was granted explicitly by the human (Julio) on
2026-08-09: V3_9_DISPATCH_AUTHORIZED=true, LEAF_EXECUTION_AUTHORIZED=true,
LEASE_ACTIVATION_AUTHORIZED=true, INTEGRATION_AUTHORIZED=true,
D0_D1_D2_AUTHORIZED=true, PR_MERGE_AUTHORIZED=true (force_push=false).

## Completed

- Lock acquired (`~/.hermes/locks/seq66-loves-sooperlooper-pr34-controller.lock`);
  preflight verified PR #34 open/draft/mergeable, head `d2bb3cd0…`, CURRENT→CP-074,
  exact-head CI success, zero v3.9 result branches, fork-main `a14c3f6c` intact.
- RUN: `20260809T090829Z-v3-9-authorized-lab-dispatch`; batch
  `20260808T233306Z-DOGFOOD004-LAB-V3_9` (package batch, identity-exact);
  worktrees detached at candidate C `50953878…`.
- Leaves executed (controller-completed after subagent partial output):
  - LAB-A: modules process_supervisor/jack_server/osc_probe/sooperlooper_launcher/__init__;
    corrected `set_control()` (baseline: no params) and `__exit__` context-manager
    signature (baseline declares `__exit__` without parens → finalizer skips it;
    `with` requires exc_type/exc_val/exc_tb); frozen literal tests ft-a1..ft-a4 all exit 0.
    R_A = `46bd5f52…`, R_A^ == C, worktree clean, zero tracked ELF.
  - LAB-B: modules audio_oracle/graph_assertions/runner/test_modules + scenarios +
    fixtures (.gitkeep dirs); removed non-owned LAB-A modules that contaminated the
    worktree; frozen literal tests ft-b1..ft-b4 all exit 0 (including ft-b3 silent
    WAV FAIL and ft-b4 canonical C helper build).
    R_B = `122303ee…`, R_B^ == C, worktree clean, zero tracked ELF.
- Finalizer v3.9 (frozen package, mode finalize): **both PASS**.
  - LAB-A: prepublication_verdict=PASS, gate_failures=[], E = `7db1a1b7…`
    (E^==R_A, diff R..E evidence-only).
  - LAB-B: prepublication_verdict=PASS, gate_failures=[], E = `71a63065…`
    (E^==R_B, diff R..E evidence-only).
- Result branches published (create-only, HTTPS):
  - `result/20260809T090829Z-v3-9-authorized-lab-dispatch-lab-a-v3_9` → `7db1a1b7…`
  - `result/20260809T090829Z-v3-9-authorized-lab-dispatch-lab-b-v3_9` → `71a63065…`
- Postpublish binding PASS: remote heads exact, evidence SHA-256 matches the
  finalizer output, verdicts PASS with zero gate failures
  (`receipts/headless-lab-results-v3.9/postpublish-binding.json`).
- Evidence published to control plane:
  - `receipts/headless-lab-results-v3.9/lab-a-controller-evidence.json`
  - `receipts/headless-lab-results-v3.9/lab-b-controller-evidence.json`
- D0 = dispatch (leaves + finalize + evidence + result branches) — DONE.
- D1 = postpublish binding + control-plane evidence publication — DONE.
- D2 = CP-075 + CURRENT + PR metadata + exact-head CI on new control-plane head
  (below).

## Verification

- R_A/R_B parent == C, single parent, clean checkout, zero tracked ELF,
  evidence-absent in C..R, literal tests exit 0 (8/8 across both labs).
- Finalizer v3.9 prepublication PASS for both (all gates, including F-A1 package
  import context, F-B2 semantic annotations, F-B3 valid WAV return shape, F-B1
  zero tracked ELF).
- Postpublish binding PASS; result branches exist remotely at the exact E SHAs.
- Controlled build: canonical C helpers compiled by ft-b4 into build/ (gitignored);
  no leaf-produced binaries; zero tracked ELF in R_A/R_B.

## Current state

- `v3_9_dispatch_authorized = true (human, 2026-08-09)` — consumed.
- `R_A = 46bd5f52…` — E-eligible, published, evidence-only E.
- `R_B = 122303ee…` — E-eligible, published, evidence-only E.
- `leases = active (both)`; `leaves = 2`; `remote_result_branches = 2`.
- `integration_authorized = true`; `D0_D1_D2_authorized = true` (consumed);
  `PR_merge_authorized = true` (pending exact-head senior merge review).
- `v3_9_technical_dispatch_eligibility = PASS` (binding 3×accept from CP-074).

## Risks and unresolved questions

- Result branches are ephemeral lab artifacts; they are NOT merge candidates
  into fork-main (integration consumes evidence, not branch tips).
- PR #34 merge requires exact-head senior merge review (L2) and
  AUTONOMOUS-MERGE.md expected-head protection before merging.

## Next executable action

Run the exact-head senior merge review (merge-gate) on the new control-plane
head; on accept (blocking=0), merge PR #34 to fork-main with expected-head
protection; then write the post-transition checkpoint.

## Open first

- PR #34 exact head after control-plane push
- `receipts/headless-lab-results-v3.9/*` on GitHub (control-plane head)
- Exact-head CI (Project control plane + Audio integration core)
- Senior merge review output

## Safe reference point

- predecessor_head = `d2bb3cd00fb4825712c59b4213f757780d95e451`
- candidate = `509538784afc2b828f2d922f65cf8ca3a39b5ee7`
- v3_9_manifest = `71596b8472b459940ee79fe9df8d3687dd3dbbdf5bf4696d897a9e4a1bdb3bba`
- R_A / E_A = `46bd5f52…` / `7db1a1b7…`
- R_B / E_B = `122303ee…` / `71a63065…`
- result branches = `result/20260809T090829Z-v3-9-authorized-lab-dispatch-lab-{a,b}-v3_9`
