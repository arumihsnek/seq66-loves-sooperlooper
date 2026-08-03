# AGENTS.md

## Scope

These instructions apply to the whole repository.

A more specific `AGENTS.md` may refine rules for a subtree, but it MUST NOT
weaken the architecture, backend, testing, documentation, branch, autonomy,
merge, escalation or handoff requirements defined here.

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

## Autonomous operating authority

The repository operates in `autonomous_by_default` mode inside an approved
milestone.

Mandatory governance sources:

- `/PROJECT-AUTONOMY.json` — machine-readable authority and gate contract;
- `/AUTONOMY.md` — compact entry point;
- `doc/sooperlooper/AUTONOMY.md` — normative operating model;
- `MISSION-LIFECYCLE.md` — recover/implement/review/merge/checkpoint loop;
- `SENIOR-CONSULTATION.md` — independent senior review contract;
- `AUTONOMOUS-MERGE.md` — exact ordinary-task merge gate;
- `HUMAN-ESCALATION.md` — valid human questions and safety stops.

Agents classify material decisions as:

- `L1_AUTONOMOUS` — operator decides and executes;
- `L2_SENIOR_REQUIRED` — operator consults senior, then decides and executes;
- `L3_HUMAN_REQUIRED` — one bounded human decision;
- `L4_SAFETY_STOP` — stop the affected mutation and report evidence.

Ordinary technical uncertainty is not a human gate. Human authority remains for
product choices, destructive/irreversible changes, incompatible licensing,
requirement conflicts, unavailable subjective/physical validation and milestone
close/open transitions.

An ordinary task PR may be merged autonomously only when every condition in
`AUTONOMOUS-MERGE.md` passes on the exact head. No prompt may weaken those
conditions or the architectural invariants below.

## Five-minute recovery path

A new Codex, Hermes or human development session MUST begin in this order:

1. read `/PROJECT-MANIFEST.json`;
2. read `/PROJECT-AUTONOMY.json` and `/AUTONOMY.md`;
3. read `doc/sooperlooper/checkpoints/CURRENT.md`;
4. read `doc/sooperlooper/WORK-QUEUE.md` and locate the active task ID;
5. inspect the active PR metadata and current CI status;
6. read only the requirement, architecture and protocol sections linked by the
   active task;
7. inspect only the source/tests named by the task and current PR diff;
8. expand context further only when a concrete dependency or failure requires
   it.

Do NOT start by loading every Markdown file, the complete upstream TODO, the
entire Seq66 manual or broad source directories.

## Context budget and progressive disclosure

Use each control file for one purpose:

- `PROJECT-MANIFEST.json`: repository, branches, upstream revisions, phase,
  active task, required workflows and canonical paths;
- `PROJECT-AUTONOMY.json`: decision authority, consultation, merge and human
  escalation gates;
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
2. `PROJECT-AUTONOMY.json` and `AUTONOMY.md`;
3. `checkpoints/CURRENT.md`;
4. its `WORK-QUEUE.md` entry;
5. affected `TRACEABILITY.md` rows.

Protocol work additionally reads:

- `OSC-CONTROL-AND-FEEDBACK.md`;
- pinned SooperLooper `OSC`, `src/control_osc.cpp`, `src/command_map.cpp`,
  `src/engine.*` and relevant DSP/JACK code;
- `TESTED-BEHAVIOUR.md`.

Architecture/backend/process work additionally reads:

- `ARCHITECTURE.md`;
- `DECISIONS.md`;
- `SENIOR-CONSULTATION.md`;
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
- `/PROJECT-AUTONOMY.json`;
- `/AUTONOMY.md`;
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
status, manifest, handoff, checkpoint, autonomy or agent governance. Extend the
canonical file or archive a superseded fork document according to
`DOCUMENTATION-MAP.md`.

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
2. verify the autonomy policy and classify material decisions L1-L4;
3. verify `CURRENT.md` points to an existing immutable checkpoint;
4. inspect current required workflows;
5. select one primary ready task from `WORK-QUEUE.md`;
6. verify dependencies and expected file ownership;
7. mark the task `in_progress` and record an agent/session identifier when
   practical;
8. create a checkpoint for the status transition if work is being handed across
   sessions or agents;
9. state assumptions in repository/PR context when evidence is incomplete.

A session may make small prerequisite fixes, but unrelated cleanup is deferred
to another task.

## Senior consultation workflow

Use the `codex-senior-consult` skill according to
`doc/sooperlooper/SENIOR-CONSULTATION.md` as a bounded planning, escalation,
failure-diagnosis and independent merge-review mechanism. It does not replace
repository recovery, direct source inspection, tests or ownership by the
primary agent.

Consultation is required at these high-leverage points when the skill is
available:

1. once after repository recovery and before finalizing the implementation plan
   for a non-trivial task involving architecture, protocol, concurrency,
   lifecycle, persistence, CI/branch history or several plausible approaches;
2. before accepting a durable L2 decision whose consequences extend beyond the
   current local edit;
3. after one focused self-diagnosis when work is genuinely blocked, repeated
   failure has occurred or more than one credible root cause remains;
4. before moving a high-risk task to `review` or `done`, especially when it
   introduces or changes a protocol contract, state model, range, threading
   rule, persistence behaviour, backend policy or recovery procedure;
5. before every ordinary functional autonomous merge and every milestone gate.

Do not consult for routine edits, formatting, obvious compiler diagnostics,
ordinary test reruns or questions already answered by canonical repository
evidence. Do not repeat a consultation on unchanged evidence.

Re-consult only after new evidence, a materially changed plan, a stale reviewed
head or a failed proposed remedy. Bundle related questions into one focused
request instead of issuing many small calls.

A consultation request should provide only the smallest sufficient packet:

- task ID, objective and current phase;
- relevant invariants and acceptance criteria;
- exact head/diff, source, failing command or runtime evidence;
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

If the skill is unavailable or fails, continue only with bounded safe L1 work
after recording that limitation. Unavailability cannot waive an L2 merge gate;
that task remains in review until independent review is available or the human
explicitly changes policy.

## Parallel agents

Parallel work is allowed only when tasks have:

- separate stable IDs;
- explicit dependencies;
- disjoint expected files, or a named integration owner;
- one declared owner for shared protocol/build/control documents.

Avoid simultaneous edits to:

- `PROJECT-MANIFEST.json`;
- `PROJECT-AUTONOMY.json` and autonomy contracts;
- `checkpoints/CURRENT.md`;
- the same immutable checkpoint;
- shared Meson files;
- the same protocol tables;
- the same API headers;
- the active PR body.

The integration owner resolves shared-file changes, validates the combined diff
and writes the final checkpoint.

A branch waiting for an L3 answer may pause while independent safe work
continues on disjoint tasks.

## Mandatory handoff and checkpoint

Every session that changes repository state, task/phase status, product policy,
architecture, tested behaviour, autonomy policy or the plan MUST leave a
checkpoint before it ends.

The mandatory exit sequence is:

1. run the narrowest relevant tests and record exact results;
2. update the task status/next action in `WORK-QUEUE.md`;
3. update affected rows in `TRACEABILITY.md`;
4. update `CHANGELOG-FORK.md` for durable notable changes;
5. update `DECISIONS.md` for accepted/provisional architecture or product
   choices;
6. update `PROJECT-MANIFEST.json` when phase, task, branches, upstream/tested
   revisions, compatibility or required workflows changed;
7. update `PROJECT-AUTONOMY.json` and linked governance documents together when
   authority or gates changed;
8. write one immutable checkpoint under `doc/sooperlooper/checkpoints/`;
9. update `checkpoints/CURRENT.md` to point to it;
10. update the active PR with scope, commits, exact test evidence, failures,
    decision level, senior/human references, risks and next task ID;
11. leave the branch buildable, or record the exact red gate and reproduction.

A chat summary, model handoff, commit message or PR comment alone is not a valid
checkpoint.

Follow `CHECKPOINTS.md`. Checkpoints are required at task transitions, phase
gates, durable runtime discoveries, blockers, autonomous merges and
branch/upstream changes.

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

A task is not `done` when code exists but required documentation, tests, review,
autonomy gate or checkpoint are missing.

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
20. Ordinary autonomy never bypasses required independent review, exact-head CI
    or human L3/L4 authority.

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
- Schema changes require migration, rollback tests and an L3 human gate.

## Testing requirements

Follow `HEADLESS-TESTING.md` and affected traceability rows.

Every code change adds or updates the narrowest relevant test.

Required fast checks where applicable:

- warnings-as-errors compile;
- pure/unit tests;
- fake-engine OSC contract tests;
- audio-disabled compilation/build wiring;
- existing relevant Seq66 MIDI/build checks;
- project-control validation;
- autonomy-policy validation.

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

`PROJECT-AUTONOMY.json` and governance documents are validated by
`contrib/scripts/validate-autonomy-policy.py` through the project-control
workflow.

## Documentation update matrix

Update the same coherent change when modifying:

- architecture/ownership -> `ARCHITECTURE.md`, `DECISIONS.md`, specification;
- OSC path/signature/control -> OSC contract, tests, traceability;
- state/error/backend -> specification, architecture, tests, traceability;
- persistence schema -> specification, migration docs/tests, manifest;
- phase/task status -> work queue, roadmap/manifest as applicable, checkpoint;
- tested runtime behaviour -> `TESTED-BEHAVIOUR.md`, traceability, checkpoint;
- branch/upstream policy -> `UPSTREAM-SYNC.md`, manifest, checkpoint;
- autonomy/merge/escalation authority -> `PROJECT-AUTONOMY.json`, autonomy
  documents, validator, PR template, changelog and checkpoint;
- notable fork behaviour/tooling -> `CHANGELOG-FORK.md`;
- canonical document path -> documentation map, manifest, validator and links.

Do not duplicate large protocol or governance lists. Link canonical material.

## Codex-specific guidance

- Use one coherent branch/PR per task or small milestone after bootstrap.
- Read the active task and autonomy policy before broad code search.
- Use a draft PR early for CI/review visibility.
- Keep commits logically reviewable.
- Do not commit build directories, sessions or captured audio except intentional
  small fixtures.
- When local tests cannot run, add/repair CI and state what remains unverified.
- Use available specialist consultation autonomously for ordinary technical
  problems.
- Merge eligible ordinary tasks only through `AUTONOMOUS-MERGE.md`.
- Do not close/open a milestone, perform destructive migration or cross an L3/L4
  gate without the required human answer.
- Before stopping, complete the mandatory checkpoint sequence.

## Hermes-specific guidance

Hermes may coordinate planner, implementer, reviewer and operator profiles, but
GitHub remains the shared source of truth.

- Give every profile the manifest, autonomy policy, current checkpoint and task
  ID rather than a complete chat transcript.
- Separate investigation, implementation and review findings.
- Durable decisions go into repository documents, issues or PRs.
- A coordinator must verify diff and CI; another agent's narrative is not proof.
- A reviewer checks invariants, protocol schema, threading, backend blocking,
  persistence safety, autonomy classification and evidence level.
- The integration owner writes the final combined checkpoint.
- Hermes may autonomously merge ordinary task PRs when every exact-head gate
  passes and the senior verdict has no blocker.
- Hermes asks the human only under `HUMAN-ESCALATION.md`, including milestone
  gates, destructive migration, backend product-policy changes, incompatible
  dependencies, upstream-compatibility breaks and unavailable subjective or
  physical acceptance.
- A blocked branch may wait while disjoint safe tasks continue.

## Failure handoff

A failed attempt is acceptable only when the checkpoint records:

- exact task and commit;
- failing command/workflow/assertion;
- smallest relevant diagnostics;
- what was ruled out;
- current hypothesis;
- changed files;
- safe rollback/reference point;
- decision level;
- senior or human reference when applicable;
- next diagnostic action.

Do not hide failure by removing assertions, extending arbitrary sleeps or
marking a blocked task done.

## Review checklist

Before declaring a task ready/done or merging autonomously:

- Is current state recoverable from manifest/checkpoint/work queue?
- Is the autonomy classification correct?
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
- Did the senior review the exact current head when required?
- Are all required checks green on that exact head?
- Are unresolved review threads zero?
- Will expected-head protection and the required merge method be used?
- Are docs, changelog, PR and checkpoint current?
- Does the PR target `fork-main`, not `master`?
- Is a milestone/human gate being crossed accidentally?

## Current state

Do not encode volatile branch/task status here.

Read `PROJECT-MANIFEST.json`, `PROJECT-AUTONOMY.json` and
`checkpoints/CURRENT.md`. They are validated by
`.github/workflows/project-control.yml` and are the authoritative compact state
and authority entry points.
