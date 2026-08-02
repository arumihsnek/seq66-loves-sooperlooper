# AGENTS.md

## Scope

These instructions apply to the whole repository.

A more specific `AGENTS.md` may refine rules for a subtree, but it MUST NOT
weaken the architecture, backend, testing, documentation, branch or handoff
requirements defined here.

This repository is a fork of Seq66 whose product goal is to make
SooperLooper-backed audio loops first-class Seq66 tracks and grid slots.
SooperLooper runs headless as a managed real-time engine; the musician operates
Seq66.

## Prime directive

Repository evidence is the durable source of truth.

Chat history, agent memory, private scratchpads and previous session narratives
are disposable context caches. No future agent may need them to recover project
state or continue work.

A state-changing session is incomplete until it leaves the required repository
checkpoint and updates the project-control files.

## Five-minute recovery path

A new Codex, Hermes or human development session MUST begin in this order:

1. read `/PROJECT-MANIFEST.json`;
2. read `doc/sooperlooper/checkpoints/CURRENT.md`;
3. read `doc/sooperlooper/WORK-QUEUE.md` and locate the active task ID;
4. inspect the active PR metadata and current CI status;
5. read only the requirement, architecture and protocol sections linked by the
   active task;
6. inspect only the source/tests named by the task and current PR diff;
7. expand context further only when a concrete dependency or failure requires
   it.

Do NOT start by loading every Markdown file, the complete upstream TODO, the
entire Seq66 manual or broad source directories.

## Context budget and progressive disclosure

Use each control file for one purpose:

- `PROJECT-MANIFEST.json`: repository, branches, upstream revisions, phase,
  active task, required workflows and canonical paths;
- `checkpoints/CURRENT.md`: compact latest handoff and next executable action;
- immutable checkpoint: detailed evidence and risks for one handoff;
- `WORK-QUEUE.md`: executable tasks, dependencies and acceptance criteria;
- `TRACEABILITY.md`: requirement -> implementation -> evidence -> task;
- `ROADMAP.md` under `doc/sooperlooper`: phase ordering and gates;
- `SPECIFICATION.md`: normative product behaviour;
- `ARCHITECTURE.md` and `DECISIONS.md`: ownership, boundaries and rationale;
- `OSC-CONTROL-AND-FEEDBACK.md`: bidirectional protocol contract;
- `TESTED-BEHAVIOUR.md`: revision-specific runtime evidence;
- `CHANGELOG-FORK.md`: durable notable fork changes;
- active PR and CI: review surface and executable evidence.

Do not reconstruct current status from commit messages when these files exist.
Do not paste large documents into checkpoints or duplicate canonical lists.

## Required reading by task type

Every task reads:

1. `PROJECT-MANIFEST.json`;
2. `checkpoints/CURRENT.md`;
3. its `WORK-QUEUE.md` entry;
4. affected `TRACEABILITY.md` rows.

Protocol work additionally reads:

- `OSC-CONTROL-AND-FEEDBACK.md`;
- pinned SooperLooper `OSC`, `src/control_osc.cpp`, `src/command_map.cpp`,
  `src/engine.*` and relevant DSP/JACK code;
- `TESTED-BEHAVIOUR.md`.

Architecture/backend/process work additionally reads:

- `ARCHITECTURE.md`;
- `DECISIONS.md`;
- `UPSTREAM-SYNC.md` when branch/upstream compatibility is involved.

UI, performer, recording and persistence work reads its complete normative
sections in `SPECIFICATION.md` and the corresponding phase gate.

When documents and source disagree, follow the source-of-truth order in the
manifest and document index. Record the mismatch; never silently guess.

## Documentation families

The repository contains inherited Seq66 documentation and fork documentation.
They do not have equal authority.

### Canonical fork planning/control

- `/PROJECT-MANIFEST.json`;
- `/AGENTS.md`;
- `/CHANGELOG-FORK.md`;
- `/ROADMAP.md` and `/TODO` as fork entry-point pointers;
- `/doc/sooperlooper/**` except archived/superseded material;
- `/tests/audio/README.md`;
- active PR/CI as evidence.

### Upstream reference only

- Seq66 planning/history on upstream-mirror `master`;
- upstream `/ROADMAP.md` and `/TODO`;
- `/ChangeLog`, `/NEWS` and upstream release content in `/RELNOTES`;
- inherited manuals and planning notes unless explicitly promoted by the fork
  documentation map;
- source TODO/FIXME comments.

An upstream TODO or roadmap item is NOT authorized fork work. It becomes work
only after receiving a stable task ID, linked requirements and acceptance tests.

Do not create duplicate active files named variants of roadmap, TODO, backlog,
status, manifest, handoff or checkpoint. Extend the canonical file or archive a
superseded fork document according to `DOCUMENTATION-MAP.md`.

## Branch model

### Permanent branches

- `master`: clean mirror of `ahlstromcj/seq66:master`; no fork-specific commits;
- `fork-main`: stable integration line for the fork.

### Working branches

- `feature/<scope>`;
- `fix/<scope>`;
- `docs/<scope>`;
- `sync/upstream-YYYY-MM-DD`;
- `experiment/<scope>` only for disposable work that cannot merge directly.

All normal fork PRs target `fork-main`, never `master`.

After the bootstrap PR, prefer one reviewable branch/PR per coherent task or
small milestone. Do not keep all future work in one indefinitely growing branch.

Never force-push a shared branch or merge/rebase shared integration history
without explicit authorization. Follow `UPSTREAM-SYNC.md` for upstream imports.

## Starting a work session

Before editing:

1. verify repository, active branch, PR base and upstream SHAs against the
   manifest;
2. verify `CURRENT.md` points to an existing immutable checkpoint;
3. inspect current required workflows;
4. select one primary ready task from `WORK-QUEUE.md`;
5. verify dependencies and expected file ownership;
6. mark the task `in_progress` and record an agent/session identifier when
   practical;
7. create a checkpoint for the status transition if work is being handed across
   sessions or agents;
8. state assumptions in repository/PR context when evidence is incomplete.

A session may make small prerequisite fixes, but unrelated cleanup is deferred
to another task.

## Senior consultation workflow

Use the `codex-senior-consult` skill as a bounded planning, escalation and
review mechanism. It does not replace repository recovery, direct source
inspection, tests or ownership by the primary agent.

Consultation is required at these high-leverage points when the skill is
available:

1. once after repository recovery and before finalizing the implementation plan
   for a non-trivial task involving architecture, protocol, concurrency,
   lifecycle, persistence, CI/branch history or several plausible approaches;
2. before crossing a project phase gate, task gate or accepting a durable
   decision whose consequences extend beyond the current local edit;
3. after one focused self-diagnosis when work is genuinely blocked, repeated
   failure has occurred or more than one credible root cause remains;
4. before moving a high-risk task to `review` or `done`, especially when it
   introduces or changes a protocol contract, state model, range, threading
   rule, persistence behaviour, backend policy or recovery procedure.

Do not consult for routine edits, formatting, obvious compiler diagnostics,
ordinary test reruns or questions already answered by canonical repository
evidence. Do not repeat a consultation on unchanged evidence.

The default consultation budget is one planning/review call per meaningful
phase or task gate and one call per distinct blocker. Re-consult only after new
evidence, a materially changed plan or a failed proposed remedy. Bundle related
questions into one focused request instead of issuing many small calls.

A consultation request should provide only the smallest sufficient packet:

- task ID, objective and current phase;
- relevant invariants and acceptance criteria;
- exact source, diff, failing command or runtime evidence;
- options already considered and what has been ruled out;
- one precise decision, diagnosis or review question.

Ask the consultant to challenge assumptions, compare the credible options,
identify failure modes, missing tests and rollback concerns, and recommend the
smallest safe next step.

The primary agent remains responsible for the decision and MUST verify advice
against pinned source, repository contracts and executable evidence.
Consultation output is advice, not proof. Record only durable conclusions,
rationale and accepted/rejected recommendations in the PR, checkpoint,
`DECISIONS.md` or affected control file; do not paste full consultation
transcripts into the repository.

If the skill is unavailable or fails, continue with bounded best effort after
recording that limitation. Unavailability alone is not a reason to abandon a
safe, well-evidenced task.

## Parallel agents

Parallel work is allowed only when tasks have:

- separate stable IDs;
- explicit dependencies;
- disjoint expected files, or a named integration owner;
- one declared owner for shared protocol/build/control documents.

Avoid simultaneous edits to:

- `PROJECT-MANIFEST.json`;
- `checkpoints/CURRENT.md`;
- the same immutable checkpoint;
- shared Meson files;
- the same protocol tables;
- the same API headers;
- the active PR body.

The integration owner resolves shared-file changes, validates the combined diff
and writes the final checkpoint.

## Mandatory handoff and checkpoint

Every session that changes repository state, task/phase status, product policy,
architecture, tested behaviour or the plan MUST leave a checkpoint before it
ends.

The mandatory exit sequence is:

1. run the narrowest relevant tests and record exact results;
2. update the task status/next action in `WORK-QUEUE.md`;
3. update affected rows in `TRACEABILITY.md`;
4. update `CHANGELOG-FORK.md` for durable notable changes;
5. update `DECISIONS.md` for accepted/provisional architecture or product
   choices;
6. update `PROJECT-MANIFEST.json` when phase, task, branches, upstream/tested
   revisions, compatibility or required workflows changed;
7. write one immutable checkpoint under `doc/sooperlooper/checkpoints/`;
8. update `checkpoints/CURRENT.md` to point to it;
9. update the active PR with scope, commits, exact test evidence, failures,
   risks and next task ID;
10. leave the branch buildable, or record the exact red gate and reproduction.

A chat summary, model handoff, commit message or PR comment alone is not a valid
checkpoint.

Follow `CHECKPOINTS.md`. Checkpoints are required at task transitions, phase
gates, durable runtime discoveries, blockers and branch/upstream changes.

Do not rewrite immutable checkpoints to reinterpret history. Write a new one.

## Task/status vocabulary

Tasks use only:

- `ready`;
- `in_progress`;
- `blocked`;
- `review`;
- `done`;
- `deferred`.

Requirements use only:

- `specified`;
- `partially_implemented`;
- `implemented`;
- `verified`;
- `blocked`;
- `deferred`.

A task is not `done` when code exists but required documentation, tests, review
or checkpoint are missing.

## Evidence levels

Always distinguish these claims:

1. specified;
2. implemented by code inspection;
3. compiled;
4. pure/unit tested;
5. fake-engine protocol tested;
6. pinned real SooperLooper tested;
7. native JACK tested;
8. PipeWire-JACK tested;
9. target Raspberry Pi/hardware tested;
10. soak/performance tested.

Never collapse them into “tested” without naming the actual level and command or
workflow.

## Product objective

Create a system where a musician can:

- create an audio-loop slot in the Seq66 grid;
- choose musical length and routing;
- record exactly the requested number of bars;
- launch, mute, overdub, replace and otherwise control it alongside MIDI;
- change tempo using free, tape or elastic policy;
- save and restore the whole project transactionally;
- operate headless or through Seq66's UI;
- do all of this without opening the SooperLooper GUI.

## Initial non-goals

- embedding SooperLooper DSP into the Seq66 process;
- pretending SooperLooper has direct ALSA audio support;
- silently launching JACK over ALSA;
- persisting mutable loop indexes as clip identity;
- blocking UI or real-time threads on OSC/process/file operations;
- treating outbound delivery as successful state change;
- broad unrelated Seq66 refactors;
- exposing an unverified SooperLooper control in the UI;
- replacing the fork work queue with upstream TODOs or agent notes.

## Architectural invariants

Agents MUST preserve these invariants:

1. Seq66 owns project state, musical intent, UI, transport policy, routing and
   process supervision.
2. SooperLooper owns real-time audio truth and reports it to Seq66.
3. Integration is bidirectional; outbound-only OSC is incomplete.
4. Desired and observed state remain separate.
5. Runtime loop indexes are valid only within one engine generation.
6. Stable clip identity uses UUIDs.
7. SooperLooper remains a separate headless process.
8. Production operation requires no SooperLooper GUI.
9. PipeWire-JACK and native JACK are supported initial audio environments.
10. ALSA-only audio hard-blocks SooperLooper tracks as
    `backend_unavailable`.
11. `backend_unavailable` is not mute and cannot be bypassed by UI, keyboard,
    MIDI automation or headless commands.
12. Existing audio clips/media are preserved when unavailable.
13. Seq66 does not silently start JACK over ALSA in the initial release.
14. OSC receive/send never blocks Qt paint or real-time MIDI/audio paths.
15. A sent command is not confirmed until observed feedback or a bounded
    verification query proves the result.
16. Project persistence is transactional; failure preserves the prior valid
    project.
17. Existing MIDI-only behaviour remains functional.
18. `master` remains an upstream mirror and normal fork work targets
    `fork-main`.
19. Current project state is recoverable from repository control files without
    chat history.

Violating an invariant requires an explicit decision, specification change,
traceability update, tests, changelog and checkpoint in the same reviewed PR.

## Backend terminology

Do not conflate MIDI and audio backends.

Seq66 may use ALSA for MIDI while SooperLooper audio uses PipeWire-JACK or native
JACK.

Use availability states consistently:

- `available`;
- `backend_unavailable`;
- `engine_offline`;
- `reconciling`;
- `stale`;
- `error`.

Do not use `muted` for backend/process/routing failure.

## OSC and feedback rules

- Centralize typed paths, commands, controls and observed outputs.
- Do not scatter protocol strings through performer or UI code.
- Validate outbound index, identifier, type, range and finite numbers.
- Validate inbound path and OSC signature before reading arguments.
- Preserve unknown engine-state integers safely as unknown raw values.
- Reject or quarantine unknown controls according to the protocol contract.
- Ignore feedback from obsolete engine generations.
- Timestamp observed values and distinguish zero from missing/stale.
- Coalesce meters/position when needed; do not intentionally drop state
  transitions, operation results or errors.
- Generate callback URLs/paths internally.
- Default to loopback OSC; remote control is outside initial scope.
- Constrain file operations to active project path policy.
- Treat `/set` as eventually consistent; immediate `/get` may be stale.
- Before adding a control, verify pinned source and add protocol tests.

## Threading rules

- No synchronous OSC request from Qt paint/event rendering.
- No network, process, file save or unbounded lock in a real-time callback.
- Receive callbacks produce typed events or bounded cache updates.
- UI consumes immutable snapshots or queued notifications.
- Performer scheduling uses non-blocking commands and observed events.
- Define ownership and shutdown order for every thread/server/process object.
- Test receiver shutdown, late callbacks and restart races.

## Loop allocation and identity

Until a later decision resolves the allocation policy:

- do not delete arbitrary middle indexes;
- normal removal uses only verified remove-last semantics;
- do not assume indexes survive restart/session load;
- use explicit engine generation;
- keep UUID/index mapping in one component;
- block conflicting commands during topology rebuild;
- verify topology after allocation/restore;
- do not persist runtime indexes as identity.

## Tempo and recording

- Support free, tape and elastic modes as specified.
- Reject unsupported rate/stretch values instead of silently changing the
  musical result.
- Keep independent pitch shift inside verified range.
- N-bar recording uses Seq66 timeline plus verified quantize/round semantics.
- Compare observed loop length with expected duration.
- Never claim sample accuracy without real-engine evidence.
- Propose a minimal SooperLooper extension only after tests prove upstream
  behaviour insufficient.

## UI rules

- Audio slots are first-class Seq66 slots, not an embedded external GUI.
- Empty-slot creation distinguishes MIDI and audio.
- Audio creation is disabled with explanation when backend unavailable.
- Loaded unavailable clips remain visible and preserved.
- Disabled operations cannot be bypassed through alternate input paths.
- UI reflects observed state and distinguishes pending, stale and error.
- Meter/progress painting reads cached data only.
- Core UI operations should have equivalent headless control paths.

## Persistence rules

- Persist UUIDs, musical metadata and versioned media/session references.
- Never persist runtime indexes as identity.
- Treat SooperLooper save/load as asynchronous with explicit result/timeout.
- Timeout is not success.
- Use temporary staging and atomic publication.
- Preserve prior valid project on failure.
- Loading without supported audio backend preserves clips and media.
- Missing media is a visible recoverable error, not silent deletion.
- Schema changes require migration, rollback tests and a human gate.

## Testing requirements

Follow `HEADLESS-TESTING.md` and affected traceability rows.

Every code change adds or updates the narrowest relevant test.

Required fast checks where applicable:

- warnings-as-errors compile;
- pure/unit tests;
- fake-engine OSC contract tests;
- audio-disabled compilation/build wiring;
- existing relevant Seq66 MIDI/build checks;
- project-control validation.

Required real-engine checks where applicable:

- pinned headless SooperLooper over JACK dummy;
- ping/version/topology;
- command-to-feedback transition;
- update subscriptions;
- graceful quit and cleanup.

Required negative checks where applicable:

- malformed OSC;
- stale/old-generation feedback;
- delayed, duplicated and reordered callbacks;
- timeout/process failure;
- ALSA-only hard block;
- no command emission from blocked slots.

Use bounded state-based waits, not long fixed sleeps. Preserve actionable logs
and fixture revisions on failure.

## Build and CI

Seq66 uses Meson. The audio integration currently also has focused GitHub
Actions.

When modifying Meson:

- preserve builds without audio/JACK/liblo support;
- avoid conceptual coupling of SooperLooper support to NSM;
- add a dedicated build option/dependency model at the appropriate phase;
- test enabled and disabled configurations.

Do not claim compilation from visual inspection. Run CI or a local command and
record the exact result.

Never weaken/remove a required workflow to make a PR green. A changed required
workflow needs manifest, changelog, checkpoint and review updates.

## Documentation update matrix

Update the same coherent change when modifying:

- architecture/ownership -> `ARCHITECTURE.md`, `DECISIONS.md`, specification;
- OSC path/signature/control -> OSC contract, tests, traceability;
- state/error/backend -> specification, architecture, tests, traceability;
- persistence schema -> specification, migration docs/tests, manifest;
- phase/task status -> work queue, roadmap/manifest as applicable, checkpoint;
- tested runtime behaviour -> `TESTED-BEHAVIOUR.md`, traceability, checkpoint;
- branch/upstream policy -> `UPSTREAM-SYNC.md`, manifest, checkpoint;
- notable fork behaviour/tooling -> `CHANGELOG-FORK.md`;
- canonical document path -> documentation map, manifest, validator and links.

Do not duplicate large protocol lists. Link canonical material.

## Codex-specific guidance

- Use one coherent branch/PR per task or small milestone after bootstrap.
- Read the active task before broad code search.
- Use a draft PR early for CI/review visibility.
- Keep commits logically reviewable.
- Do not commit build directories, sessions or captured audio except intentional
  small fixtures.
- When local tests cannot run, add/repair CI and state what remains unverified.
- Use available specialist consultation autonomously for ordinary technical
  problems.
- Do not merge without explicit human instruction.
- Before stopping, complete the mandatory checkpoint sequence.

## Hermes-specific guidance

Hermes may coordinate planner, implementer, reviewer and operator profiles, but
GitHub remains the shared source of truth.

- Give every profile the manifest, current checkpoint and task ID rather than a
  complete chat transcript.
- Separate investigation, implementation and review findings.
- Durable decisions go into repository documents, issues or PRs.
- A coordinator must verify diff and CI; another agent's narrative is not proof.
- A reviewer checks invariants, protocol schema, threading, backend blocking,
  persistence safety and evidence level.
- The integration owner writes the final combined checkpoint.
- Human gates remain for merge, destructive migration, backend-policy changes,
  maintained SooperLooper patch dependency and upstream-compatibility breaks.

## Failure handoff

A failed attempt is acceptable only when the checkpoint records:

- exact task and commit;
- failing command/workflow/assertion;
- smallest relevant diagnostics;
- what was ruled out;
- current hypothesis;
- changed files;
- safe rollback/reference point;
- next diagnostic action.

Do not hide failure by removing assertions, extending arbitrary sleeps or
marking a blocked task done.

## Review checklist

Before declaring a task ready/done:

- Is current state recoverable from manifest/checkpoint/work queue?
- Is the task status honest and traceability updated?
- Does Seq66 still own intent/lifecycle?
- Is runtime truth derived from feedback?
- Are desired and observed state separate?
- Are backend failures distinct from mute?
- Is ALSA-only hard-blocked without data loss?
- Are indexes generation-scoped and UUIDs stable?
- Are OSC paths/signatures/ranges verified?
- Can malformed or late feedback corrupt state?
- Can any UI/real-time path block?
- Are save/load outcomes explicit and transactional?
- Are waits bounded and diagnostics useful?
- Does MIDI-only behaviour remain intact?
- Are evidence levels stated precisely?
- Are docs, changelog, PR and checkpoint current?
- Does the PR target `fork-main`, not `master`?

## Current state

Do not encode volatile branch/task status here.

Read `PROJECT-MANIFEST.json` and `checkpoints/CURRENT.md`. They are validated by
`.github/workflows/project-control.yml` and are the authoritative compact state
entry points.
