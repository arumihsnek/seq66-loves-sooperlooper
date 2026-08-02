# Changelog — Seq66 Loves SooperLooper

All notable changes specific to this fork are recorded here.

This file is separate from upstream Seq66 `ChangeLog`, `NEWS` and `RELNOTES`.
Those files remain upstream historical/release references and are not the fork's
work queue or product changelog.

The format follows Keep a Changelog principles without assigning release
versions before the fork reaches a releasable state.

## [Unreleased]

### Added

- Fork purpose and authority model in the root README.
- `audio_clip` model with arbitrary positive bar counts and time signatures.
- Free, tape and elastic tempo policies plus independent pitch shift.
- Initial non-real-time SooperLooper OSC command adapter.
- Fake-engine OSC contract test with strict path/signature validation.
- Pinned SooperLooper 1.7.9 real-engine smoke test over JACK dummy.
- Architecture, bidirectional OSC contract, normative specification, headless
  testing strategy, development guide and phased roadmap.
- `AGENTS.md` operating contract for Codex, Hermes and human contributors.
- Machine-readable `PROJECT-MANIFEST.json`.
- Multi-agent workflow, actionable work queue, checkpoint protocol, decision
  log, documentation map and requirement/test traceability.
- Immutable checkpoint `CP-001` plus the small mutable
  `checkpoints/CURRENT.md` resume pointer.
- Branch policy separating upstream mirror `master`, fork integration
  `fork-main` and short-lived feature branches.
- Dedicated upstream synchronization policy preserving a clean import line.
- Structural CI validation for project-control metadata, canonical files,
  active task, checkpoint schema and required workflows.
- Root fork entry points replacing the inherited unstructured `TODO` and
  possible-v2 `ROADMAP.md` on the fork line while preserving their originals on
  `master` and in upstream Seq66.

### Changed

- Pull request #1 now targets `fork-main` instead of upstream-mirror `master`.
- Documentation distinguishes fork plans from upstream Seq66 TODOs, roadmap,
  release notes and historical changelog.
- The integration documentation index now provides a progressive-disclosure
  recovery path designed to minimize agent context use.
- Control confirmation models SooperLooper updates as eventually consistent,
  based on real-engine evidence.
- Material agent sessions must leave an immutable checkpoint, update the work
  queue and record exact CI evidence before handoff.

### Fixed

- Real-engine smoke test no longer assumes a `/set` is visible to an immediate
  `/get`; it waits for bounded observed confirmation.
- Project-control bootstrap now includes the checkpoint and `CURRENT.md` files
  required by its own validator.

### Security and safety

- ALSA-only audio is specified to fail closed as `backend_unavailable` without
  deleting or silently muting persisted clips.
- OSC callback paths, signatures and file operations are constrained by the
  integration specification.

## Changelog rules

Update this file in the same coherent change for:

- user-visible behaviour;
- supported backend or compatibility changes;
- architecture or persistence changes;
- new required workflows or tooling;
- migrations;
- important fixes;
- deprecations or removed fork functionality.

Do not add every internal refactor. Checkpoints record session-level progress;
this changelog records durable project changes.
