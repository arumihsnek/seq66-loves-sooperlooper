# Current project checkpoint

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-02-CP-002-agent-continuity.md`

Checkpoint ID: `CP-002`
Checkpoint date: 2026-08-02
Active branch: `feature/sooperlooper-audio-clips`
Integration target: `fork-main`
Active pull request: #1
Current phase: `phase-1-protocol-core` — Phase 1, protocol core
Completed phase: `phase-0-project-contract`
Active task: `M1-001`
Task status: `ready`

## Minimal resume summary

The repository now has a complete project-control plane for multi-agent work:
manifest, canonical documentation map, phased roadmap, stable-ID work queue,
traceability, decision log, fork changelog, checkpoint protocol, upstream sync
policy and GitHub PR/task templates.

`master` is the clean upstream mirror; `fork-main` is the fork integration line.
Root `TODO` and `ROADMAP.md` on the fork line are short pointers, while original
upstream contents remain on `master`.

The audio model, outbound OSC adapter, fake-engine contract and pinned real
SooperLooper/JACK-dummy smoke test are present. Production inbound feedback and
application integration are not.

## Next executable action

Claim `M1-001`, then implement canonical typed SooperLooper protocol identifiers,
mappings and metadata with focused tests. Do not begin UI, supervisor or
persistence work first.

## Read next

1. the immutable CP-002 checkpoint linked above;
2. `PROJECT-MANIFEST.json`;
3. only `M1-001` in `doc/sooperlooper/WORK-QUEUE.md`;
4. relevant identifier/range sections in
   `OSC-CONTROL-AND-FEEDBACK.md`;
5. affected client code/tests and traceability rows;
6. latest PR #1 checks.

Do not treat upstream `NEWS`, `RELNOTES`, `ChangeLog`, mirror `TODO`, mirror
`ROADMAP.md` or old Seq66 planning prose as the active fork plan.

## Handoff rule

This file is a mutable pointer only. At the end of the next material session,
create a new immutable checkpoint and replace this pointer and summary. Never
edit a historical checkpoint.
