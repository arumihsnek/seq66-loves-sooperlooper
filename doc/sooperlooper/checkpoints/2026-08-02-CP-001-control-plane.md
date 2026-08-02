# Checkpoint CP-001 — Project control plane and integration baseline

Date: 2026-08-02
Branch: `feature/sooperlooper-audio-clips`
Target branch: `fork-main`
Pull request: #1
Status: complete checkpoint; implementation continues in Phase 1

## Purpose

This immutable checkpoint records the state after establishing the fork's
architecture, bidirectional SooperLooper contract, initial audio model, headless
tests and multi-agent coordination system.

It is a recovery artifact, not a live planning document. Future agents must not
edit this file. Create a new checkpoint and update `checkpoints/CURRENT.md`.

## Branch model

- `master` is the clean import line for upstream Seq66.
- `fork-main` is the integration branch for this product fork.
- feature branches start from and target `fork-main`.
- current feature branch: `feature/sooperlooper-audio-clips`.
- upstream fixes are imported into `master`, then integrated deliberately into
  `fork-main`; fork product work never targets `master`.

## Product decisions fixed

- Seq66 is the user-facing authority and owns project intent, transport,
  lifecycle, routing and UI.
- SooperLooper remains an external headless real-time engine.
- OSC is bidirectional: outbound delivery is not completion.
- desired state and observed engine state are separate.
- PipeWire-JACK and native JACK are the initial supported audio environments.
- ALSA may remain the MIDI backend, but ALSA-only audio puts SooperLooper clips
  in hard `backend_unavailable` state.
- `backend_unavailable` is not mute and cannot be bypassed by UI, MIDI,
  automation or headless commands.
- stable clip identity uses UUIDs; SooperLooper loop indexes are runtime and
  engine-generation scoped.

## Implemented code

- `audio_clip` musical model;
- positive arbitrary bar counts and time signatures;
- free, tape and elastic tempo policies;
- independent pitch shift;
- initial outbound `sooperlooper_client` using liblo;
- validated command/control emission;
- Meson registration for the initial audio core.

## Executable evidence

Fast gate:

- C++17 warning-clean build with `-Wall -Wextra -Wpedantic -Werror`;
- audio model unit test;
- fake-engine OSC contract test;
- status at checkpoint: PASS.

Real-engine gate:

- pinned SooperLooper commit
  `c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec` / 1.7.9;
- headless build without GUI;
- JACK dummy at 48 kHz / 256 frames;
- ping/version/loop count;
- loop creation and topology feedback;
- control set/get with bounded observed confirmation;
- loop deletion and graceful quit;
- status at checkpoint: PASS.

Observed compatibility fact: `/set` followed immediately by `/get` can return
the old value. Confirmation must wait for observed state within a deadline.

## Coordination/control files established

- `PROJECT-MANIFEST.json` — compact machine-readable project state;
- `AGENTS.md` — repository-wide operating contract;
- `CHANGELOG-FORK.md` — durable fork changes;
- `doc/sooperlooper/WORKFLOW.md` — branch/task/review workflow;
- `doc/sooperlooper/WORK-QUEUE.md` — task IDs and acceptance criteria;
- `doc/sooperlooper/CHECKPOINTS.md` — checkpoint protocol;
- `doc/sooperlooper/TRACEABILITY.md` — requirements-to-tests map;
- `doc/sooperlooper/DECISIONS.md` — accepted/deferred decisions;
- `doc/sooperlooper/UPSTREAM-POLICY.md` — upstream sync policy;
- `doc/sooperlooper/DOCUMENTATION-MAP.md` — canonical versus inherited docs;
- `contrib/scripts/validate-project-control.py` and project-control CI.

The root upstream `TODO` and `ROADMAP.md` were replaced on the fork feature line
with short pointers to the canonical fork planning documents. Their original
contents remain available on `master` and in upstream Seq66.

## Current phase and next task

Completed phase: Phase 0 — project contract.

Current phase: Phase 1 — protocol core.

Active task: `M1-001`, production typed OSC receiver and callback namespace.

Expected next vertical slice:

1. central typed OSC identifiers and payload types;
2. local liblo server with deterministic callback namespace;
3. strict signature/path validation;
4. typed event queue or bounded observed-state update path;
5. tests for valid, malformed, late and unknown callbacks;
6. clean receiver shutdown and late-callback safety.

## Not implemented yet

- production bidirectional receiver;
- observed-state cache and freshness model;
- engine generation and stale feedback rejection;
- process supervisor;
- backend detector/gate in Seq66 application code;
- JACK/PipeWire routing manager;
- performer dispatch;
- native Qt audio slots;
- exact N-bar recording against real audio;
- transactional persistence and recovery;
- waveform or Song Mode integration.

## Known risks/open decisions

- fixed loop pool versus controlled topology rebuild;
- session file versus per-clip WAV authority;
- final auto-update intervals for position/meters;
- whether exact N-bar close needs a minimal SooperLooper extension;
- separation of SooperLooper liblo support from Seq66 NSM build detection;
- project bundle schema and media transaction design.

## Recovery procedure

Read, in order:

1. `PROJECT-MANIFEST.json`;
2. `doc/sooperlooper/checkpoints/CURRENT.md`;
3. `doc/sooperlooper/WORK-QUEUE.md` active task only;
4. relevant sections of `SPECIFICATION.md` and
   `OSC-CONTROL-AND-FEEDBACK.md`;
5. changed code/tests for the active task;
6. PR #1 and latest CI only when more evidence is required.

Do not load every inherited Seq66 document into context. Root upstream planning
files on `master` are not the fork work queue.

## Exit requirements for the next agent

Before ending a material work session:

- update the task state in `WORK-QUEUE.md`;
- update `CHANGELOG-FORK.md` for durable changes;
- update manifest/traceability/decisions when affected;
- create a new immutable checkpoint;
- point `checkpoints/CURRENT.md` to it;
- update PR #1 with exact tests, failures and risks;
- leave the branch buildable or describe the verified failure precisely.
