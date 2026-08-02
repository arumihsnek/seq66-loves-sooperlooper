# Checkpoint CP-003 — M1-001 typed protocol

Date: 2026-08-02
Author: `hermes` session `0bc5d91d41b9`
Branch: `feature/m1-001-typed-osc-protocol`
Target branch: `fork-main`
Pre-checkpoint verified head: `cb6929c0` (bootstrap merge of PR #1)
Status: implementation in progress; no draft pull request yet

This file is immutable after publication.  Later sessions create a new
checkpoint and update `checkpoints/CURRENT.md`.

## Objective

Deliver the M1-001 typed-protocol slice: a single source of truth for
SooperLooper OSC command, loop-control and global-control identifiers with
bidirectional string mappings, per-control range/type metadata, and bounded
observed-state integer parsing, plus a focused unit test that proves the
contract.

The session also had to migrate the existing outbound `sooperlooper_client` to
the new typed accessors, leave the integration without raw OSC string
literals, and keep the existing fake-engine OSC contract test and
`audio_clip_test` green against the migrated client.

## Completed

### New module

- `libseq66/include/audio/sooperlooper_protocol.hpp` declares
  `sooperlooper_command` (16), `loop_control` (51), `global_control` (17),
  `payload_kind`, `state_parse_result` and the public accessors
  `to_string`, `metadata_for`, `is_in_range`, `try_parse` and
  `parse_state_int`.
- `libseq66/src/audio/sooperlooper_protocol.cpp` implements a single
  `constexpr` table per kind with canonical string, payload kind, wire-level
  lo/hi bounds and inclusive/exclusive bound mode, and a 18-entry state
  table for the canonical SooperLooper loop states.  `is_in_range` rejects
  non-finite values and falls through to per-control range checks.

### Migrated client

- `libseq66/include/audio/sooperlooper_client.hpp` and
  `libseq66/src/audio/sooperlooper_client.cpp` were migrated to use the
  typed accessors; no raw OSC string literals remain in integration code.

### Build wiring

- `libseq66/include/meson.build` registers the new header.
- `libseq66/src/meson.build` registers the new translation unit.
- `.github/workflows/audio-core.yml` builds and runs the new focused test
  alongside the existing `audio_clip_test` and
  `sooperlooper_osc_contract_test`, with warnings-as-errors.

### New focused test

- `tests/audio/sooperlooper_protocol_test.cpp` exercises every mapped
  identifier: command strings, loop-control and global-control string
  round-trips, per-control range (valid, low, high, +Inf, -Inf, NaN),
  payload-kind metadata, and the canonical state-integer parser (including
  the `raw = 999` unknown path).

### Project control

- `PROJECT-MANIFEST.json` records the active branch
  (`feature/m1-001-typed-osc-protocol`), the active task and its
  `in_progress` status, and the bootstrap merge `cb6929c0` as the base.
- `doc/sooperlooper/WORK-QUEUE.md` advances `M1-001` to `in_progress` with
  agent, branch and start metadata.
- `doc/sooperlooper/TRACEABILITY.md` records the implementation pointer and
  evidence for `OSC-001`, `OSC-002`, `OSC-003` and `STATE-001`.
- `CHANGELOG-FORK.md` adds the new module and focused test to
  `Unreleased / Added`.
- `doc/sooperlooper/checkpoints/CURRENT.md` points at this checkpoint.

## Verification

Local build and run on the new files, with warnings-as-errors:

```
$ mkdir -p .ci/include .ci/bin
$ echo '#define SEQ66_HAVE_LIBLO 1'         >  .ci/include/seq66-config.h
$ echo '#define SEQ66_SOOPERLOOPER_SUPPORT 1' >> .ci/include/seq66-config.h
$ g++ -std=c++17 -Wall -Wextra -Werror -pedantic -pthread \
      -I.ci/include -Ilibseq66/include \
      libseq66/src/audio/audio_clip.cpp \
      libseq66/src/audio/sooperlooper_client.cpp \
      libseq66/src/audio/sooperlooper_protocol.cpp \
      tests/audio/sooperlooper_protocol_test.cpp \
      -llo -o .ci/bin/sooperlooper_protocol_test
$ .ci/bin/sooperlooper_protocol_test ; echo "exit=$?"
exit=0
```

The same flags build the migrated client against the existing
`audio_clip_test` and `sooperlooper_osc_contract_test`; both still pass
with exit code 0.

The focused test exercises:

- 16 `sooperlooper_command` string mappings;
- 51 `loop_control` string mappings + per-control range (valid, low, high,
  +Inf, -Inf, NaN) + bidirectional round-trip via `try_parse` + an unknown
  string guard;
- 17 `global_control` string mappings + per-control range + bidirectional
  round-trip + an unknown string guard;
- 51 `loop_control` and 17 `global_control` payload-kind metadata entries;
- 18 canonical state-integer mappings (including the `raw = 999` unknown
  path).

The `Project control plane`, `Audio integration core` and
`Real SooperLooper headless smoke` workflows have not been observed in
this session because no draft pull request has been opened yet.  They will
be exercised on the first PR push.

## Current state

Completed phase: `phase-0-project-contract`.

Current phase: `phase-1-protocol-core`.

Active task: `M1-001` — typed protocol identifiers.

Task status: `in_progress`.  The implementation, the migrated client and
the focused test pass locally; the only remaining work is the draft pull
request, the immutable checkpoint already exists.

The repository can be recovered without chat history from
`PROJECT-MANIFEST.json`, this checkpoint, the `M1-001` entry of
`doc/sooperlooper/WORK-QUEUE.md` and `TRACEABILITY.md` rows
`OSC-001..003` plus `STATE-001`.

## Risks and unresolved questions

- The per-control range table encodes only the wire-level acceptance
  window; it does not yet capture semantic groupings (e.g. destructive vs
  non-destructive) that M1-006 and the UI gating rules in
  `SPECIFICATION.md` will need.
- The receiver (M1-002) must still call `try_parse` on every observed
  control name and reject unknown names by preserving them as raw state
  instead of inventing new identifiers; the protocol module is ready but
  no production receiver exists.
- The wider `Project control plane` and `Real SooperLooper headless smoke`
  workflows have not been observed for this branch in the present
  session; their state on the first PR push must be reviewed before
  merging.
- `repository.active_pull_request` in the manifest remains `null` until
  the draft PR is opened; the validator requires a positive integer, so
  that field will be updated to the PR number in the next material
  session.
- The project-control validator is run manually here and not on the
  branch in this session; the next session must confirm it passes.

## Next executable action

1. Commit the M1-001 changes on `feature/m1-001-typed-osc-protocol` with a
   reviewable history.
2. Push the branch to `arumihsnek/seq66-loves-sooperlooper` and open a
   draft pull request against `fork-main`.
3. Update `PROJECT-MANIFEST.json` `repository.active_pull_request` to the
   new PR number and re-run `contrib/scripts/validate-project-control.py`.
4. Wait for the first execution of the three required workflows and
   record exact run identifiers and PASS evidence in a new checkpoint.

Do not start `M1-002` (receiver lifecycle) until the PR is open and the
CI results are recorded.

## Open first

A new session should open, in order:

1. `PROJECT-MANIFEST.json`;
2. `doc/sooperlooper/checkpoints/CURRENT.md`;
3. this checkpoint `2026-08-02-CP-003-m1-001-typed-protocol.md`;
4. only the `M1-001` section of `doc/sooperlooper/WORK-QUEUE.md`;
5. `OSC-CONTROL-AND-FEEDBACK.md` for the canonical contract;
6. the focused test and migrated client in
   `libseq66/src/audio/sooperlooper_protocol.cpp` and
   `libseq66/src/audio/sooperlooper_client.cpp`;
7. `TRACEABILITY.md` rows `OSC-001..003` and `STATE-001`;
8. the first PR's required-workflow results.

Do not load the inherited upstream TODO, possible-v2 roadmap or the
complete manual until a concrete dependency requires it.

## Safe reference point

Repository: `arumihsnek/seq66-loves-sooperlooper`.

Seq66 upstream base:
`d6a588a48fdb0a30bf7223c5d325627163181615` / 0.99.26.

Pinned/tested SooperLooper:
`c5e22ce76ae9a6b358fe7d85720c61dfc5af8bec` / 1.7.9.

Pre-checkpoint fully green integration head:
`cb6929c0` (bootstrap merge of PR #1 onto `fork-main`).

Active feature head: `cb6929c0` + uncommitted M1-001 changes on
`feature/m1-001-typed-osc-protocol`; the exact post-commit SHA will be
recorded in the next checkpoint.

Before ending the next material session, follow the mandatory `AGENTS.md`
exit sequence: tests, queue, traceability, changelog/decisions/manifest
if affected, new immutable checkpoint, `CURRENT.md`, and PR evidence.
