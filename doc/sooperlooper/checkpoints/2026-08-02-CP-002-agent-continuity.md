# Checkpoint CP-002 — Multi-agent continuity and upstream-safe fork workflow

Date: 2026-08-02
Branch: `feature/sooperlooper-audio-clips`
Target branch: `fork-main`
Pull request: #1
Pre-checkpoint verified head: `71bb6149d1828b111a2ad78f0959933564eda5fa`
Status: complete handoff; Phase 1 implementation has not started

This file is immutable after publication. Later sessions create a new checkpoint
and update `checkpoints/CURRENT.md`.

## Objective

Finish the repository operating system required for reliable long-running work
across Codex, Hermes and human agents while preserving a clean path for
importing upstream Seq66 fixes.

The session specifically had to eliminate ambiguity between fork planning and
the inherited upstream backlog, provide a low-context recovery path, require
checkpoints/changelog/traceability, and make those rules machine-verifiable.

## Completed

### Recovery and handoff

- Added machine-readable `PROJECT-MANIFEST.json` with repository, branch,
  upstream revision, phase, active task, canonical paths and required workflows.
- Added immutable checkpoint protocol and mutable `checkpoints/CURRENT.md`.
- Added `CP-001` as the bootstrap baseline and this `CP-002` as the final session
  handoff.
- Expanded `AGENTS.md` with a five-minute progressive-disclosure recovery path,
  context-budget rules, task claiming, evidence levels, parallel-agent file
  ownership and mandatory exit sequence.
- Added `WORKFLOW.md`, `WORK-QUEUE.md`, `TRACEABILITY.md`, `DECISIONS.md`,
  `DOCUMENTATION-MAP.md`, `CHANGELOG-FORK.md` and upstream sync policy.
- Added GitHub PR and task-proposal templates requiring task IDs, requirement
  IDs, evidence levels, control-file updates and next executable action.

### Planning and documentation authority

- Phase 0 is explicitly complete; Phase 1 protocol core is current.
- `M1-001` is the single active/ready next task.
- Root `TODO` and `ROADMAP.md` on the fork line are concise pointers to the
  canonical queue and roadmap.
- Full inherited upstream versions remain unchanged on mirror `master` and in
  `ahlstromcj/seq66`.
- The intentional divergence is recorded in `DECISIONS.md`,
  `DOCUMENTATION-MAP.md`, `UPSTREAM-SYNC.md`, changelog and manifest.
- Agents are forbidden from promoting an upstream TODO into work without a
  stable task ID, requirements and acceptance tests.

### Branch and upstream workflow

- `master` is reserved as clean upstream mirror/import line.
- `fork-main` is the stable fork integration line.
- PR #1 was retargeted from `master` to `fork-main` and is mergeable.
- Normal work uses short-lived `feature/*`, `fix/*`, `docs/*` or `sync/*`
  branches targeting `fork-main`.
- Upstream updates fast-forward mirror `master`, then enter `fork-main` through
  a reviewed `sync/upstream-YYYY-MM-DD` PR with regression evidence and a
  checkpoint.
- The planned GitHub default branch after bootstrap merge is `fork-main`; this
  repository setting still requires a manual change by the owner.

### Existing integration baseline retained

- `audio_clip` musical model;
- arbitrary positive bar counts/time signatures;
- free, tape and elastic tempo policies;
- independent pitch shift;
- initial outbound SooperLooper OSC adapter;
- fake-engine contract test;
- pinned real SooperLooper 1.7.9 / JACK-dummy smoke test;
- architecture, protocol, specification and testing documentation.

## Verification

All three required workflows passed on pre-checkpoint head
`71bb6149d1828b111a2ad78f0959933564eda5fa`:

1. `Project control plane`, run 32: PASS.
   - manifest JSON valid;
   - canonical paths exist;
   - branch model and PR base coherent;
   - current/completed phases present in roadmap;
   - `M1-001` exists with active status;
   - `CURRENT.md` points to a schema-valid immutable checkpoint;
   - root fork TODO/roadmap entry points are valid;
   - all three required workflows are declared.
2. `Audio integration core`, run 104: PASS.
   - warning-clean C++17 compile;
   - audio model test;
   - fake-engine OSC contract test.
3. `Real SooperLooper headless smoke`, run 38: PASS.
   - pinned 1.7.9 headless build;
   - JACK dummy startup;
   - ping/version/topology;
   - control feedback confirmation;
   - loop removal and graceful shutdown.

PR #1 at the verified head was open, draft, mergeable and based on `fork-main`.

The checkpoint and subsequent `CURRENT.md` pointer are documentation-only
changes. CI must run again on the resulting final head; agents should inspect
that latest result rather than assuming this pre-checkpoint evidence is the
final commit status.

## Current state

Completed phase: `phase-0-project-contract`.

Current phase: `phase-1-protocol-core`.

Active task: `M1-001` — typed protocol identifiers.

Task status remains `ready`; no production Phase 1 implementation was begun in
this documentation/control-plane session.

The repository can now be recovered without chat history from manifest,
`CURRENT.md`, work queue, task-specific contract sections and current CI.

## Risks and unresolved questions

- GitHub default branch is still `master` until the owner changes it after PR #1
  is merged; the manifest records both current and planned default branches.
- Branch protection/rulesets are not configured through this connector. The
  owner should later protect `fork-main` and require the three declared checks.
- The bootstrap PR is intentionally large. After merge, each task should use a
  smaller branch/PR rather than extending this feature branch indefinitely.
- Root `TODO`/`ROADMAP.md` are intentional upstream-sync conflict points; sync
  PRs must retain fork pointer versions while reviewing upstream changes.
- A GitHub issue or project board must not become a second authority unless the
  manifest/workflow explicitly defines synchronization with `WORK-QUEUE.md`.
- Production receiver, observed state, supervisor, backend gate, performer, UI
  and persistence remain unimplemented.

## Next executable action

Claim `M1-001` in `WORK-QUEUE.md` by changing it from `ready` to `in_progress`
and recording the agent/session identifier when practical.

Then implement only the typed protocol-identifier slice:

1. typed command, loop-control, global-control and observed-output identifiers;
2. canonical mapping in both directions;
3. range/type metadata where verified;
4. rejection of unknown outbound identifiers;
5. safe handling policy for unknown inbound values;
6. mapping/metadata unit tests plus existing fake-engine test;
7. documentation and traceability updates;
8. new checkpoint at task handoff.

Do not jump directly to Qt UI, process supervision or persistence.

## Open first

A new session should open, in order:

1. `PROJECT-MANIFEST.json`;
2. `doc/sooperlooper/checkpoints/CURRENT.md`;
3. only the `M1-001` section of `doc/sooperlooper/WORK-QUEUE.md`;
4. `OSC-CONTROL-AND-FEEDBACK.md` sections for identifiers/ranges;
5. affected audio client headers/sources and tests;
6. `TRACEABILITY.md` rows `OSC-001..003` and `STATE-001`;
7. latest PR #1 metadata and required checks.

Expand into other documents/source only when a concrete dependency requires it.
Do not load the inherited upstream TODO, possible-v2 roadmap or complete manual.

## Safe reference point

Repository: `arumihsnek/seq66-loves-sooperlooper`.

Seq66 upstream base:
`d6a588a48fdb0a30bf7223c5d325627163181615` / 0.99.26.

Pinned/tested SooperLooper:
`c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec` / 1.7.9.

Pre-checkpoint fully green feature head:
`71bb6149d1828b111a2ad78f0959933564eda5fa`.

PR: #1, draft, target `fork-main`.

Before ending the next material session, follow the mandatory `AGENTS.md` exit
sequence: tests, queue, traceability, changelog/decisions/manifest if affected,
new immutable checkpoint, `CURRENT.md`, and PR evidence.
