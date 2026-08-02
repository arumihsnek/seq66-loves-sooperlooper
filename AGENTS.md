# AGENTS.md

## Scope

These instructions apply to the whole repository. More specific `AGENTS.md`
files may refine them for a subtree but must not weaken architecture, safety,
testing or documentation requirements defined here.

This repository is a fork of Seq66 whose purpose is to make SooperLooper-backed
audio loops first-class Seq66 tracks and grid slots.

The normal product experience must be Seq66 alone. SooperLooper runs headless
as a managed real-time audio engine.

## Intended readers

This file is written for autonomous and semi-autonomous coding agents,
including Codex and Hermes profiles, and for human contributors supervising
those agents.

Agents must work from repository evidence and current tests. Chat history is not
a source of truth.

## Required reading

Before changing audio-loop integration, read:

1. `README.md`;
2. `doc/sooperlooper/README.md`;
3. `doc/sooperlooper/ARCHITECTURE.md`;
4. `doc/sooperlooper/OSC-CONTROL-AND-FEEDBACK.md`;
5. `doc/sooperlooper/SPECIFICATION.md`;
6. `doc/sooperlooper/HEADLESS-TESTING.md`;
7. `doc/sooperlooper/ROADMAP.md`;
8. relevant upstream Seq66 code and documentation;
9. the pinned/tested SooperLooper source, especially `OSC`,
   `src/control_osc.cpp`, `src/command_map.cpp`, `src/engine.*` and the JACK
   driver.

When documentation and source disagree, follow the source-of-truth order in
`doc/sooperlooper/README.md` and record the mismatch.

## Product objective

Create a system where a musician can:

- create an audio-loop slot in the Seq66 grid;
- choose musical length and routing;
- record exactly the requested number of bars;
- launch, mute, overdub, replace and otherwise control it alongside MIDI;
- change tempo using free, tape or elastic policy;
- save and restore the whole project;
- operate headless or through Seq66's UI;
- do all of this without opening the SooperLooper GUI.

## Non-goals for the initial implementation

- embedding SooperLooper DSP into the Seq66 process;
- pretending SooperLooper has a direct ALSA audio backend;
- silently launching JACK over ALSA;
- persisting mutable SooperLooper loop indexes as clip identity;
- blocking UI or real-time threads on OSC;
- treating outbound command delivery as successful state change;
- broad refactors unrelated to the active integration phase.

## Architectural invariants

Agents MUST preserve these invariants.

1. Seq66 owns project state, musical intent, UI and process supervision.
2. SooperLooper owns real-time audio truth and reports it back to Seq66.
3. The integration is bidirectional; outbound-only OSC is incomplete.
4. Desired state and observed state remain separate.
5. Runtime loop indexes are scoped to an engine generation.
6. Stable clip identity uses UUIDs.
7. SooperLooper remains a separate headless process.
8. Production operation requires no SooperLooper GUI.
9. PipeWire-JACK and native JACK are supported audio environments.
10. ALSA-only audio hard-blocks SooperLooper tracks as
    `backend_unavailable`.
11. `backend_unavailable` is not mute and cannot be unlocked by track
    controls.
12. Existing audio clips are preserved when the backend is unavailable.
13. Seq66 does not silently start JACK over ALSA in the initial release.
14. OSC receive/send never blocks Qt paint or real-time MIDI/audio paths.
15. A sent command is not confirmed until observed feedback or a bounded
    verification query proves the resulting state.
16. Project persistence is transactional and failure must preserve the prior
    valid project.
17. Existing MIDI-only behaviour must remain functional.

A proposed change that violates an invariant requires an explicit architecture
and specification change in the same PR, with tests and a written rationale.

## Backend terminology

Do not conflate MIDI and audio backends.

Seq66 may use ALSA for MIDI while audio-loop support uses PipeWire-JACK or
native JACK.

Use these availability terms consistently:

- `available`;
- `backend_unavailable`;
- `engine_offline`;
- `reconciling`;
- `stale`;
- `error`.

Do not use `muted` as a substitute for backend/process/routing failure.

## OSC rules

- Centralize OSC path, command and control identifiers.
- Do not scatter untyped strings across performer or UI code.
- Validate outbound loop indexes, control names, value ranges and finite
  numbers.
- Validate inbound path and OSC type signature before reading arguments.
- Preserve unknown engine-state integers safely.
- Ignore feedback from obsolete engine generations.
- Timestamp observed values.
- Distinguish zero from missing/stale feedback.
- Coalesce meters/position if needed, but never drop state transitions,
  operation results or errors intentionally.
- Generate callback URLs/paths internally.
- Default to loopback OSC; remote control is outside the initial scope.
- File-operation paths must obey project path policy.

Before adding a SooperLooper command/control, verify it in current source and
add a protocol test.

## Threading rules

- No synchronous OSC request from Qt paint/event rendering.
- No network, process, file save or unbounded lock in a real-time callback.
- Receive callbacks convert messages to typed events or update a bounded,
  thread-safe cache.
- UI consumes snapshots or queued notifications.
- Performer scheduling uses non-blocking commands and observed-state events.
- Define ownership and shutdown order for every thread/server object.
- Tests must cover receiver shutdown and late callbacks.

## Loop allocation

SooperLooper recommends deleting only the last loop. Until a tested policy is
selected:

- do not implement arbitrary middle-index deletion;
- do not assume indexes survive a restart/session load;
- use an explicit engine generation;
- keep UUID/index mapping in one component;
- block conflicting commands during topology rebuild;
- verify topology after allocation or restore.

## Tempo and recording

- Support free, tape and elastic modes according to the specification.
- Reject unsupported rate/stretch ratios instead of silently clamping to a
  different musical result.
- Keep independent pitch shift inside the tested range.
- N-bar recording uses Seq66's musical timeline plus SooperLooper quantize and
  round behaviour.
- Compare observed loop length with expected duration.
- Never claim sample accuracy without real-engine evidence.
- If upstream behaviour is insufficient, propose a minimal, documented
  SooperLooper extension only after tests demonstrate the need.

## UI rules

- Audio slots are first-class Seq66 slots, not an embedded external GUI.
- Empty-slot creation distinguishes MIDI and audio.
- Audio creation is disabled with an explanation when backend unavailable.
- Loaded unavailable audio clips remain visible and preserved.
- Disabled controls cannot be bypassed by keyboard/MIDI automation or headless
  commands.
- UI reflects observed engine state and visibly distinguishes pending/stale/
  error states.
- Meter/progress painting reads cached data only.
- New UI operations should have equivalent non-GUI control paths where
  practical.

## Persistence rules

- Persist stable UUIDs, musical metadata and media/session references.
- Never persist runtime indexes as identity.
- Treat SooperLooper save/load as asynchronous operations with explicit result
  or timeout.
- Timeout is not success.
- Use temporary files/directories and atomic publication.
- Preserve the previous valid project on any failure.
- Loading without supported audio backend preserves clips/media and marks them
  unavailable.
- Missing media is a visible recoverable error, not silent deletion.

## Testing requirements

Follow `doc/sooperlooper/HEADLESS-TESTING.md`.

At minimum, every code change must add or update the narrowest relevant test.

### Required fast checks

- compile changed C++ with warnings enabled;
- pure unit tests;
- fake-engine OSC contract tests for protocol changes;
- audio-disabled compilation when build wiring changes;
- existing relevant Seq66 tests/build checks.

### Required real-engine checks when applicable

- headless SooperLooper launch against JACK dummy;
- ping/version/topology;
- command-to-feedback transition;
- update subscription;
- graceful quit and cleanup.

### Required negative checks

- malformed OSC input;
- stale/old-generation feedback;
- timeout/failure path;
- ALSA-only hard block when backend logic changes;
- no command emission from blocked slots.

Do not replace state-based waits with long fixed sleeps. All waits are bounded
and diagnostics are collected on failure.

## Build and CI

The repository currently uses Meson for Seq66 and focused GitHub Actions for the
new audio core.

When modifying Meson:

- preserve builds without JACK/liblo/audio support;
- avoid coupling SooperLooper support to NSM conceptually even if both
  temporarily use liblo detection;
- add a dedicated build option/dependency model when the integration reaches
  process/receiver support;
- test enabled and disabled configurations.

Do not declare a change compiled merely because files look syntactically
correct. Use CI or a local build and report the exact command/result.

## Documentation requirements

Documentation is part of the implementation contract.

Update the relevant document in the same change when modifying:

- architecture or ownership;
- OSC paths/signatures/controls;
- states or error codes;
- backend support;
- persistence schema;
- test expectations;
- roadmap phase or definition of done;
- user-visible setup or limitations.

Do not duplicate large protocol lists in many files. Link to the canonical
document and keep summaries consistent.

## Working method for autonomous agents

1. Inspect repository state, active branch and existing PR before editing.
2. Read relevant documentation and source.
3. State assumptions explicitly in the PR/commit when evidence is incomplete.
4. Make the smallest coherent vertical change.
5. Add tests before or with implementation.
6. Run focused tests, then broader relevant checks.
7. Review the final diff for unrelated changes.
8. Update docs and PR description.
9. Leave the branch buildable; do not hide known failures.
10. Report exact commits, tests, failures and remaining risks.

Agents should continue autonomously through ordinary implementation problems.
Use repository search, source inspection, test output and specialist consultation
available in the environment. Ask the human only for decisions that materially
change product intent, destructive operations, credentials/hardware access or
irreducible ambiguity.

## Codex-specific guidance

- Prefer one feature branch per coherent milestone.
- Use a draft PR early for CI and review visibility.
- Do not commit generated build directories, local sessions or captured audio
  unless they are intentional small fixtures.
- Keep commits logically reviewable.
- When a test cannot run in the current environment, add/repair CI and state
  exactly what remains unverified.
- Do not merge the draft integration PR without explicit human instruction.

## Hermes-specific guidance

Hermes agents may coordinate investigation, planning, implementation and review,
but must preserve one source of truth in GitHub.

- Record durable decisions in repository docs, issues or PRs, not only agent
  memory.
- Separate coordinator, implementer and reviewer findings where profiles are
  used.
- A reviewer should inspect architecture invariants, OSC schema, threading,
  backend blocking and test evidence.
- Do not mark a task complete solely from another agent's narrative; verify the
  diff and CI.
- Human gates are required for merging, destructive project migration, adding
  a SooperLooper fork/patch dependency or changing backend product policy.

## Git and PR policy

- Do not push integration work directly to `master`.
- Use descriptive branches and conventional, focused commit messages.
- Keep the current integration PR in draft until its phase gate is met.
- Do not force-push shared branches unless explicitly authorized.
- Do not merge without explicit human instruction.
- Preserve upstream attribution and licensing.
- Document upstream Seq66 base commit and tested SooperLooper revision for
  release work.

## Review checklist

Before declaring work ready:

- Does Seq66 still own intent and lifecycle?
- Is runtime truth derived from feedback?
- Are desired and observed states separate?
- Are backend failures distinct from mute?
- Is ALSA-only hard-blocked without data loss?
- Are indexes generation-scoped and UUIDs stable?
- Are OSC paths/signatures/ranges verified?
- Can malformed or late feedback cause state corruption?
- Can any UI/real-time path block?
- Are save/load outcomes explicit and transactional?
- Are tests headless, bounded and diagnostic?
- Does MIDI-only behaviour remain intact?
- Are docs and PR description current?

## Current branch/phase note

At the time this file was introduced, `feature/sooperlooper-audio-clips`
contains the initial `audio_clip` model, outbound OSC adapter and focused CI.
The bidirectional receiver, process supervisor, backend gate, performer
integration, Qt audio slots and persistence are still pending. Verify current
repository state rather than assuming this note remains current.
