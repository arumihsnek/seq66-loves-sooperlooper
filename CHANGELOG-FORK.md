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
- `AGENTS.md` instructions for Codex, Hermes and human contributors.
- Machine-readable `PROJECT-MANIFEST.json`.
- Multi-agent workflow, actionable work queue, checkpoint protocol, decision
  log and requirement/test traceability.
- Branch policy separating upstream mirror `master`, fork integration
  `fork-main` and short-lived feature branches.
- Structural CI validation for project-control metadata.

### Changed

- Pull request #1 now targets `fork-main` instead of upstream-mirror `master`.
- Documentation distinguishes fork plans from upstream Seq66 TODOs, roadmap,
  release notes and historical changelog.
- Control confirmation now models SooperLooper updates as eventually
  consistent, based on real-engine evidence.

### Fixed

- Real-engine smoke test no longer assumes a `/set` is visible to an immediate
  `/get`; it waits for bounded observed confirmation.

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
