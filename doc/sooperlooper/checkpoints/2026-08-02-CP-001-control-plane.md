# Checkpoint CP-001 — Project control plane and integration baseline

Date: 2026-08-02
Branch: `feature/sooperlooper-audio-clips`
Target branch: `fork-main`
Pull request: #1
Status: complete checkpoint; implementation continues in Phase 1

This is an immutable recovery artifact. Do not edit it after publication.
Future sessions create a new checkpoint and update `checkpoints/CURRENT.md`.

## Objective

Establish a recoverable baseline for the fork: architecture, bidirectional
SooperLooper contract, initial audio model, executable headless evidence and a
multi-agent control plane that does not depend on chat history.

## Completed

### Branch and upstream model

- `master` is the clean import line for upstream Seq66.
- `fork-main` is the product integration branch.
- feature/fix/docs branches start from and target `fork-main`.
- current branch: `feature/sooperlooper-audio-clips`.
- upstream changes enter `master` first and are integrated deliberately into
  `fork-main`; fork product work never targets `master`.

### Product contract

- Seq66 owns project intent, transport, lifecycle, routing and UI.
- SooperLooper remains an external headless real-time engine.
- OSC is bidirectional; outbound delivery is not completion.
- desired state and observed engine state are separate.
- native JACK and PipeWire-JACK are supported initially.
- ALSA may remain the MIDI backend, but ALSA-only audio causes hard
  `backend_unavailable` for SooperLooper clips.
- `backend_unavailable` is not mute and cannot be bypassed by UI, MIDI,
  automation or headless commands.
- stable clip identity uses UUIDs; loop indexes are runtime and scoped to an
  engine generation.

### Implemented code

- `audio_clip` musical model;
- arbitrary positive bar counts and time signatures;
- free, tape and elastic tempo policies;
- independent pitch shift;
- initial outbound `sooperlooper_client` using liblo;
- outbound command/control validation;
- initial Meson registration.

### Project-control plane

- `PROJECT-MANIFEST.json` — compact machine-readable state;
- `AGENTS.md` — repository-wide agent operating contract;
- `CHANGELOG-FORK.md` — durable fork changes;
- `doc/sooperlooper/WORKFLOW.md` — branch/task/review workflow;
- `doc/sooperlooper/WORK-QUEUE.md` — task IDs and acceptance criteria;
- `doc/sooperlooper/CHECKPOINTS.md` — checkpoint protocol;
- `doc/sooperlooper/TRACEABILITY.md` — requirements-to-tests map;
- `doc/sooperlooper/DECISIONS.md` — accepted/deferred decisions;
- `doc/sooperlooper/UPSTREAM-SYNC.md` — upstream import policy;
- `doc/sooperlooper/DOCUMENTATION-MAP.md` — canonical/inherited docs;
- project-control validator and CI workflow.

The root `TODO` and `ROADMAP.md` on the fork line are concise entry points to
the canonical fork queue and roadmap. Their original upstream contents remain
on `master` and in `ahlstromcj/seq66`.

## Verification

Fast audio-core gate:

- C++17 warning-clean build with `-Wall -Wextra -Wpedantic -Werror`;
- audio model unit test;
- fake-engine OSC contract test;
- result: PASS at this checkpoint.

Pinned real-engine gate:

- SooperLooper commit
  `c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec` / 1.7.9;
- headless `--without-gui` build;
- JACK dummy at 48 kHz / 256 frames;
- ping, version and loop count;
- loop creation and topology feedback;
- control write with bounded observed confirmation;
- loop deletion and graceful quit;
- result: PASS at this checkpoint.

Observed compatibility fact: `/set` followed immediately by `/get` can return
the old value. Confirmation must wait for observed state within a deadline.

Project-control CI was introduced during this session. Its only known failure
before this checkpoint was the deliberate absence of `checkpoints/CURRENT.md`
and this immutable file; these files close that bootstrap gap and must be
revalidated by CI.

## Current state

Completed phase: `phase-0-project-contract` / Phase 0.

Current phase: `phase-1-protocol-core` / Phase 1.

Active task: `M1-001` — production typed OSC receiver and callback namespace.

Implemented capabilities are limited to the model, outbound adapter and test
fixtures listed above. The production application is not yet receiving or
rendering SooperLooper state.

Not implemented:

- production OSC receiver;
- observed-state cache and freshness model;
- engine generation and stale-feedback rejection;
- process supervisor and backend detector;
- JACK/PipeWire routing manager;
- performer integration;
- native Qt audio slots;
- exact N-bar real-audio recording;
- transactional persistence and recovery;
- waveform or Song Mode integration.

## Risks and unresolved questions

- fixed loop pool versus controlled topology rebuild;
- session file versus per-clip WAV as media authority;
- final position/meter auto-update intervals;
- whether exact N-bar close needs a minimal SooperLooper extension;
- separating SooperLooper liblo support from Seq66 NSM build detection;
- project-bundle schema and transaction design;
- maintaining low-conflict upstream imports while root fork entry documents
  intentionally differ from upstream.

## Next executable action

Implement `M1-001` as the next vertical slice:

1. central typed OSC identifiers and payload types;
2. local liblo server with deterministic callback namespace;
3. strict path/signature/value validation;
4. bounded typed event delivery or observed-state update path;
5. tests for valid, malformed, unknown and late callbacks;
6. deterministic receiver shutdown and late-callback safety.

Do not begin process supervision, Qt UI or persistence before this protocol gate
is complete unless the roadmap and queue are deliberately revised.

## Open first

Use progressive disclosure and load only what the active task needs:

1. `PROJECT-MANIFEST.json`;
2. `doc/sooperlooper/checkpoints/CURRENT.md`;
3. the `M1-001` entry in `doc/sooperlooper/WORK-QUEUE.md`;
4. relevant sections of `SPECIFICATION.md` and
   `OSC-CONTROL-AND-FEEDBACK.md`;
5. changed code and tests for `M1-001`;
6. PR #1 and latest CI only for additional evidence.

Do not load every inherited Seq66 document. Root upstream planning prose on
`master`, upstream `NEWS` and upstream `RELNOTES` are not the fork work queue.

## Safe reference point

Repository: `arumihsnek/seq66-loves-sooperlooper`

Upstream Seq66 base:
`d6a588a48fdb0a30bf7223c5d325627163181615` / 0.99.26.

Pinned tested SooperLooper:
`c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec` / 1.7.9.

Feature branch reference at checkpoint creation began after commit
`af30e635fda19186aa8c45acd5fdb261b4e6b18c`; this checkpoint and its mutable
`CURRENT.md` pointer were added immediately afterward. Use the branch head and
CI, not this prose, for the newest commit SHA.

Before ending the next material session:

- update `WORK-QUEUE.md` task state;
- update `CHANGELOG-FORK.md` for durable changes;
- update manifest, traceability and decisions when affected;
- create a new immutable checkpoint;
- update `checkpoints/CURRENT.md`;
- update PR #1 with exact tests, failures and risks;
- leave the branch buildable or document the verified failure precisely.
