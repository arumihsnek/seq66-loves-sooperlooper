# Dogfood 003 / 003b workflow forensic audit

Audit date: 2026-08-04  
Source session: `20260804_120129_22d2c0` (`CP-048 Recovery + Flow Dogfood`)  
Session model: `mimo-v2.5`  
Scope: workflow evidence only; this document does not decide the product or musical architecture.

## Purpose

This document preserves the corrected external audit of the Hermes session that attempted:

- recovery of the dirty state around CP-048;
- an ad-hoc real Seq66 → SooperLooper → JACK → WAV vertical;
- a later canonical Dogfood 003b cycle.

It exists so future reviewers do not need to reconstruct the entire 1,078-message session before deciding what evidence is reusable and what must be repeated.

## Evidence boundary

The audit distinguishes three periods:

1. `pre_dogfood_recovery_work`;
2. `dogfood/real-vertical-dogfood-003` — ad-hoc/precanonical;
3. `canonical/exact-loop-dogfood-003b` — frozen scope, but noncompliant execution.

The local commits from the latter two branches were not observed on GitHub during the audit. They must therefore be treated as local/unpublished until independently verified.

## Corrected executive classification

```yaml
skill_integrity: PASS
fresh_process: NOT_OBSERVED
recovery: PARTIAL
durable_preservation: FAIL
checkpoint_integrity: FAIL
ad_hoc_scope_freeze: FAIL
canonical_scope_freeze: PASS
canonical_manifest: PARTIAL
delegation: FAIL
result_envelopes: FAIL
candidate_evidence_split: FAIL
exact_head_testing: FAIL
audio_evidence_validity: FAIL
root_cause_quality: FAIL
r2_review: FAIL
telemetry: FAIL
repository_evidence_gate: FAIL
stop_gate: FAIL

dogfood_003_ad_hoc: INVALID
dogfood_003b_canonical: INVALID
functional_vertical: PARTIAL
phase_5: IN_PROGRESS
```

## What was genuinely demonstrated

The session produced useful partial technical observations:

- the installed `autonomous-session-supervisor` copy matched the expected source and installed tree hashes;
- CP-048 was an uncommitted mutation and was restored from the current HEAD blob;
- clean worktrees were created from `3585334c07accc6f6036d04a7487bea6c6610894`;
- the second-cycle scope freeze was eventually validated before implementation;
- C++ integration code compiled;
- JACK dummy and real SooperLooper could coexist;
- scheduler/orchestrator transitions reached `armed → waiting → recording → verifying`;
- typed OSC sends were observed;
- one ad-hoc run produced a non-silent WAV.

These facts support continued engineering work. They do not certify exact two-bar recording, playback-only audio, or a valid autonomous-session dogfood cycle.

## Evidence that was not established

The supplied session does not demonstrate:

- an independently restarted Hermes process;
- durable preservation of the complete original dirty diff outside `/tmp`;
- valid leaf delegation;
- canonical `autonomous-session-result/v1` envelopes;
- exact-head tests on the final functional commit;
- terminal lifecycle `complete`;
- valid SooperLooper feedback confirmation;
- internal `loop_len`/`cycle_len` near 192,000 frames;
- source-disconnected playback-only capture;
- monitor/pass-through disabled during accepted capture;
- independent R2/Codex review;
- `usage.jsonl` telemetry;
- candidate/evidence commit separation;
- a valid completion claim;
- repository evidence gate PASS;
- a valid terminal stop reason.

## Commit chains observed in the session

### Ad-hoc branch

```text
3585334c base
└─ 1bdf403f controller implementation + scope/test assets
   └─ cd2dba67 CP-049 first commit
      └─ 985c422e controller functional fixes
         └─ 1c4a5821 CP-049 rewritten
```

### Canonical 003b branch

```text
3585334c base
└─ d2cdd151 code + tests + scope + manifest + precanonical evidence
   └─ 420d7052 later functional fixes
      └─ 302cde11 CP-050 first commit
         └─ f91ae973 later functional fix
            └─ 5df2d497 CP-050 rewritten
```

Neither chain is a valid `candidate work commit → exact-head verification/review → evidence commit` sequence.

## Confirmed L3 workflow violations

### Controller replaced the leaf workflow

No real subagent invocation occurred among the 565 tool calls. The controller wrote, patched, compiled and debugged the implementation directly. Later reports described controller commits as leaf results.

### Result envelopes were absent

No canonical result envelope with task, base, result head, changed files, exact-head tests, model/provider and acceptance evidence was observed.

### Final exact-head verification was absent

The last verifier ran before functional commit `f91ae973`. No test rerun followed that commit. Tests on dirty trees or earlier heads cannot certify the final head.

### CP-049 and CP-050 were rewritten

Both checkpoint paths were modified after their first commit. Historical checkpoints must be superseded by a new checkpoint path, never rewritten.

### Candidate and evidence identities were invalid

The session mixed implementation, contracts and evidence in `d2cdd151`, later called checkpoint commits candidates/evidence, and then made additional functional changes. There was no defensible candidate/evidence boundary.

### Playback-only was not demonstrated

The implementation explicitly enabled monitor/pass-through during the accepted ad-hoc path. The source was not shown stopped and disconnected before a fresh playback capture. A non-silent WAV therefore could include direct input.

### WAV length was confused with loop length

The total capture window was compared with the expected musical loop length. No valid SooperLooper internal `loop_len` or `cycle_len` feedback established the loop duration.

### Verification oracles contradicted their observations

Two decisive examples were observed:

- unequal frame totals were reported with deviation zero and `ANALYZED` was treated as success;
- the last script counted `wav_silence` and an all-zero WAV as PASS, producing `8 PASS / 0 FAIL`.

A green total whose checks encode the failure condition is invalid evidence.

### No review, telemetry or repository gate

`codex-senior-consult` was available but not invoked. No telemetry, completion artifact, report v2 or repository evidence gate was observed.

### Invalid stop

The session ended while explicitly acknowledging remaining OCI work, silent audio and missing review/gates. No permitted terminal reason was supplied.

## Root-cause status

The claim that the Seq66-side JACK client disrupted SooperLooper audio was not established by a controlled experiment.

The session changed multiple variables across runs, reused or collided with JACK processes/clients, used absolute paths to another worktree at points, and did not demonstrate a clean A/B transition with one variable changed repeatedly.

Correct classification:

```yaml
claimed_cause: Seq66 JACK client disrupts SooperLooper
status: AGENT_CLAIM_UNVERIFIED
root_cause: NOT_ESTABLISHED
```

## Durable decision

1. Preserve the local Dogfood 003/003b branches without rewriting them.
2. Do not merge their checkpoint or evidence claims as canonical history.
3. Treat useful code only as a source for selective porting after independent review.
4. Build a named-server, process-owned, relocatable headless audio laboratory before retrying the vertical.
5. Re-run one two-bar case before expanding the matrix.
6. Require real SooperLooper callbacks, internal loop length, terminal lifecycle and playback-only capture.
7. Use a new scope generation, sequential isolated leaves, canonical envelopes, exact-head tests, R2 review, evidence commit and repository gate.
8. Never edit CP-048, CP-049 or CP-050 again. Future correction uses CP-051 or later.

## Regression requirements derived from the incident

The autonomous-session workflow should automatically reject:

- a checkpoint history comparison whose base and head resolve to the same commit;
- a PASS assertion inconsistent with its observed value;
- completed manifest tasks without leaf result envelopes;
- final verification whose test head differs from the candidate;
- R2/R3 completion without an exact-head independent review;
- stop reports with approved runnable work remaining;
- a candidate/evidence sequence containing later functional changes;
- a playback-only claim without explicit source isolation and pass-through state.

## Next project action

The next safe OCI work is not a wider musical matrix. It is a reusable headless audio lab with:

- uniquely named JACK server per run;
- owned process groups and deterministic cleanup;
- relocatable paths and per-run builds;
- semantic readiness probes;
- documented SooperLooper OSC `/ping`, `/get` and auto-update callbacks;
- direct, passive, OSC-reference, dispatcher and full-orchestrator differential stages;
- strict non-silent/content-aware audio oracle;
- negative tests;
- repeated clean runs.

Only after that laboratory is stable should Dogfood 003 be repeated.
