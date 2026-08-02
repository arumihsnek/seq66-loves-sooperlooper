# Current project checkpoint

Immutable checkpoint: `doc/sooperlooper/checkpoints/2026-08-02-CP-001-control-plane.md`

Checkpoint ID: `CP-001`
Checkpoint date: 2026-08-02
Active branch: `feature/sooperlooper-audio-clips`
Integration target: `fork-main`
Active pull request: #1
Current phase: Phase 1 — protocol core
Active task: `M1-001`

## Minimal resume summary

The project contract, backend policy, bidirectional OSC specification, initial
audio model, outbound OSC adapter, fake-engine test and pinned real-engine JACK
dummy smoke test are in place.

Both audio test workflows passed at the checkpoint. The next implementation is
the production typed inbound OSC receiver, followed by observed-state caching,
engine generations, ping/subscriptions and bounded command confirmation.

## Read next

1. the immutable checkpoint linked above;
2. `PROJECT-MANIFEST.json`;
3. the `M1-001` entry in `doc/sooperlooper/WORK-QUEUE.md`;
4. only the relevant protocol/specification sections for that task.

Do not treat `/TODO`, `/ROADMAP.md`, upstream `NEWS`, upstream `RELNOTES` or old
Seq66 planning prose as the active fork plan. Use the canonical fork control
files named above.

## Handoff rule

This file is a mutable pointer only. At the end of the next material session,
create a new immutable checkpoint and replace this file's pointer and summary.
Never edit a historical checkpoint.
