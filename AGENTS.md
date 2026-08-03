# AGENTS.md

## Scope and authority

These instructions apply to the whole repository. A subtree `AGENTS.md` may add
local detail but must not weaken architecture, testing, autonomy, merge,
escalation, evidence or safety requirements.

Repository evidence is the durable source of truth. Chat history and model
memory are disposable caches.

Mandatory governance sources:

- `/PROJECT-MANIFEST.json` — live repository/task/phase state;
- `/PROJECT-AUTONOMY.json` — machine-readable authority and gates;
- `/AUTONOMY.md` — compact entry point;
- `doc/sooperlooper/AUTONOMY.md` — normative operating model;
- `doc/sooperlooper/MISSION-LIFECYCLE.md` — task and phase loop;
- `doc/sooperlooper/SENIOR-CONSULTATION.md` — independent review;
- `doc/sooperlooper/AUTONOMOUS-MERGE.md` — exact-head merge gate;
- `doc/sooperlooper/HUMAN-ESCALATION.md` — valid human interactions.

No session prompt may weaken these contracts.

## Product objective

This fork makes SooperLooper-backed audio loops first-class Seq66 tracks and
grid slots. SooperLooper remains a separate managed headless real-time engine;
the musician operates Seq66.

## Autonomous authority

The repository operates in `autonomous_by_default` mode across the approved
roadmap.

Material decisions use four levels:

- `L1_AUTONOMOUS`: Hermes decides and executes;
- `L2_SENIOR_REQUIRED`: Hermes obtains `codex-senior-consult` review, then
  decides and executes;
- `L3_HUMAN_REQUIRED`: one bounded human product/safety decision;
- `L4_SAFETY_STOP`: stop the affected mutation and report evidence.

A phase boundary is not automatically L3. Hermes may close a clearly completed
phase and open the next already-approved bounded phase after the exact-head
senior phase gate passes and no L3/L4 trigger exists.

## Human interaction contract

Hermes asks the human only for genuine L3/L4 matters:

- product scope or priority;
- subjective musical, visual or UX behaviour;
- intentional incompatibility;
- non-FOSS/incompatible licensing;
- data loss or irreversible migration;
- unresolved approved-requirement conflict;
- unavailable physical or subjective acceptance;
- destructive or sensitive safety decisions.

When Hermes supports native interactive selection forms, it MUST use them for
human decisions whenever safe. Provide two to four concrete choices, visibly
mark the senior recommendation and include `Otra opción / Other` with free-text
input when a custom answer is safe. Plain chat is fallback only when forms are
unavailable or cannot represent the decision safely.

## Five-minute recovery path

A new session reads, in order:

1. `PROJECT-MANIFEST.json`;
2. `PROJECT-AUTONOMY.json` and `AUTONOMY.md`;
3. `doc/sooperlooper/checkpoints/CURRENT.md`;
4. the active `WORK-QUEUE.md` task;
5. active PR metadata and exact-head CI;
6. linked traceability/specification/architecture sections;
7. exact source and tests owned by the task.

Do not load every document or reconstruct state from chat.

## Task execution loop

For every material task Hermes:

1. verifies repository, branch, base, checkpoint and task consistency;
2. selects one ready task and checks dependencies;
3. defines smallest complete scope, acceptance criteria and evidence;
4. classifies decisions L1-L4;
5. consults senior for L2 architecture, lifecycle, concurrency, persistence,
   compatibility, backend or security work;
6. implements with focused positive and negative tests;
7. validates locally;
8. opens or updates a PR early;
9. repairs CI autonomously;
10. obtains exact-head senior merge review;
11. merges only through `AUTONOMOUS-MERGE.md`;
12. updates traceability, changelog and control state;
13. writes an immutable checkpoint and updates CURRENT;
14. continues to the next ready task.

Ordinary compiler errors, test design, branch management, CI reruns and eligible
green merges are not human decisions.

## Phase execution loop

At phase completion Hermes:

1. verifies definition of done item by item;
2. runs the full required matrix on the exact integrated head;
3. records unavailable evidence honestly;
4. audits traceability and control consistency;
5. obtains global exact-head senior phase review;
6. records blocking/non-blocking residual risks;
7. confirms the next phase is already approved, bounded and has a first task;
8. confirms no L3/L4 trigger exists;
9. publishes the phase gate checkpoint and PR;
10. merges with expected-head protection;
11. writes the post-transition checkpoint;
12. opens the next phase and continues.

Human interaction is required only if this transition introduces a genuine L3
choice or L4 stop.

## Branch and merge rules

Permanent branches:

- `master`: clean upstream mirror;
- `fork-main`: stable fork integration.

Normal work uses `feature/*`, `fix/*`, `docs/*`, `sync/*` or bounded
`experiment/*` branches and targets `fork-main`.

Never force-push a shared branch. Never rewrite shared integration history.
Use merge commits unless reviewed policy explicitly says otherwise.

Every autonomous merge requires:

- task or phase gate in the control plane;
- satisfied dependencies;
- exact-head required checks all green;
- focused negative tests where applicable;
- control and autonomy validators green;
- current traceability and immutable checkpoint;
- exact-head senior acceptance with no blocker;
- zero unresolved review threads;
- mergeable intended base;
- expected-head protection;
- no unresolved L3/L4 trigger.

A material change after senior review invalidates the verdict.

## Senior consultation

Use `codex-senior-consult` for:

- non-trivial architecture or protocol planning;
- threading, ownership and lifecycle;
- restart and recovery;
- persistence, migration and compatibility;
- backend and security boundaries;
- repeated or ambiguous failure diagnosis;
- every functional merge gate;
- every phase gate.

Provide the smallest sufficient packet: task/phase ID, objective, invariants,
exact head/diff, acceptance criteria or definition of done, test/CI evidence,
residual risks and one precise question.

Advice is not proof. Hermes verifies it against pinned source and executable
evidence. Senior unavailability cannot waive an L2 gate.

## Parallel work

Parallel work is allowed only with separate task IDs, declared dependencies,
disjoint ownership or one named integration owner. Shared control files have one
writer at a time.

Avoid concurrent ownership of:

- `PROJECT-MANIFEST.json`;
- `PROJECT-AUTONOMY.json` and autonomy documents;
- `checkpoints/CURRENT.md`;
- `WORK-QUEUE.md` and `TRACEABILITY.md`;
- shared build files, protocol headers and active PR bodies.

A branch waiting for L3 may pause while disjoint safe work continues.

## Mandatory checkpoint

Every state-changing session must:

1. record exact tests and evidence;
2. update task/phase status and next action;
3. update traceability;
4. update durable changelog/decisions where needed;
5. update manifest when live state changes;
6. update autonomy policy, validator and templates together when authority
   changes;
7. write one immutable checkpoint;
8. update CURRENT;
9. update the PR with exact head, evidence, risks, senior/human references and
   next task;
10. leave the branch buildable or record the exact red gate.

Never rewrite historical checkpoints. Write a new superseding checkpoint.

## Architectural invariants

Agents MUST preserve:

1. Seq66 owns desired project state, musical intent, UI, transport, routing and
   process supervision.
2. SooperLooper owns real-time audio truth and reports observed state.
3. Integration is bidirectional; outbound delivery is not confirmation.
4. Desired and observed state remain separate.
5. Runtime loop indexes are valid only within one engine generation.
6. Stable clip identity uses UUIDs; runtime indexes are never persisted as
   identity.
7. SooperLooper remains an external headless process; no GUI is required.
8. Native JACK and PipeWire-JACK are supported initial audio environments.
9. ALSA-only audio is hard `backend_unavailable`, never mute and never silently
   starts JACK over ALSA.
10. Existing clips/media are preserved when backend or engine is unavailable.
11. OSC/process/file work never blocks UI paint or real-time paths.
12. Commands are confirmed only by observed feedback or bounded verification.
13. Persistence is transactional and preserves the prior valid project on
    failure.
14. Existing MIDI-only behaviour remains functional.
15. `master` remains upstream mirror; fork work targets `fork-main`.
16. Project state is recoverable without chat history.
17. Autonomy never bypasses exact-head CI, independent senior review or genuine
    L3/L4 authority.

Violating an invariant requires a reviewed specification/decision change,
traceability, tests, changelog and checkpoint.

## Backend and state terminology

Do not conflate MIDI and audio backends. Seq66 may use ALSA MIDI while
SooperLooper audio uses native JACK or PipeWire-JACK.

Use states consistently: `available`, `backend_unavailable`, `engine_offline`,
`reconciling`, `stale`, `error`. Never use `muted` for infrastructure failure.

## Protocol and threading rules

- centralize typed OSC paths, commands and controls;
- validate inbound path/signature and outbound type/range/index;
- reject non-finite values and obsolete-generation feedback;
- timestamp observed values and distinguish zero from absent/stale;
- do not drop state transitions, operation results or errors;
- use loopback OSC by default;
- treat `/set` as eventually consistent;
- no synchronous OSC from paint/render paths;
- no network, process, file or unbounded lock in real-time callbacks;
- define ownership and shutdown order for every thread/server/process;
- test shutdown, late callbacks and restart races.

## UI, persistence and compatibility

- audio slots are first-class Seq66 slots, not embedded external UI;
- unavailable clips stay visible and preserved;
- blocked operations cannot be bypassed by alternate inputs;
- UI reflects observed pending/stale/error state;
- persist UUIDs and versioned metadata/media references, never runtime indexes;
- save/load is asynchronous, bounded and transactional;
- missing media is visible and recoverable;
- schema migration requires rollback tests and becomes L3 only when destructive,
  irreversible or intentionally incompatible.

## Testing and evidence

Always name the actual evidence level:

1. specified;
2. code-inspected;
3. compiled;
4. unit tested;
5. fake-engine tested;
6. pinned real-engine tested;
7. native JACK tested;
8. PipeWire-JACK tested;
9. target hardware tested;
10. soak/performance tested.

Where applicable require warnings-as-errors, focused unit and negative tests,
audio-enabled/disabled builds, project-control validation, autonomy validation,
pinned real-engine smoke, backend hard-block tests and bounded state-based waits.

Never remove assertions or required workflows merely to obtain green CI.

## Hermes-specific rule

Hermes may coordinate multiple profiles, but GitHub is the shared truth. Give
profiles the manifest, autonomy policy, current checkpoint, task ID and exact
review surface rather than a chat transcript.

Hermes may autonomously merge tasks and clear phases when their exact gates pass.
Hermes asks the human only through `HUMAN-ESCALATION.md`, using native selection
forms with `Otra opción / Other` when safe and supported.

## Review checklist

Before merge or phase transition verify:

- state is recoverable;
- L1-L4 classification is correct;
- task/phase status and traceability are honest;
- architectural invariants hold;
- exact evidence levels are stated;
- senior reviewed the current exact head;
- all required checks are green on that head;
- unresolved review threads are zero;
- expected-head protection will be used;
- docs, PR and checkpoint are current;
- no hidden L3/L4 decision is crossed.

## Current state

Do not encode volatile state here. Read `PROJECT-MANIFEST.json`,
`PROJECT-AUTONOMY.json` and `checkpoints/CURRENT.md`.
