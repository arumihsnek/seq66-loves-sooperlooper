# Decision log

## Purpose

This file records durable product and architecture decisions that constrain
implementation. It is not a substitute for `ARCHITECTURE.md`; it explains why
important choices were made and when they may be reconsidered.

Statuses:

- `accepted` — current project policy;
- `provisional` — used until named evidence/gate resolves it;
- `superseded` — replaced by a later decision;
- `rejected` — considered and intentionally not selected.

A decision that changes an accepted invariant requires specification, tests,
manifest, changelog and checkpoint updates in the same PR.

## D-001 — keep SooperLooper as a separate headless process

- Status: `accepted`
- Date: 2026-08-02
- Scope: process architecture

Decision: Seq66 supervises an external SooperLooper engine. SooperLooper DSP is
not embedded in the Seq66 process, and normal operation does not require `slgui`.

Rationale:

- preserves real-time and crash isolation;
- uses SooperLooper's intended JACK/OSC architecture;
- avoids coupling Seq66 UI/MIDI threads to DSP internals;
- allows independent engine restart and diagnostics.

Consequences:

- process supervision and reconciliation are mandatory;
- OSC delivery alone cannot prove engine state;
- packaging must provide or locate a compatible engine executable.

## D-002 — Seq66 owns intent; observed SooperLooper feedback owns runtime truth

- Status: `accepted`
- Date: 2026-08-02
- Scope: state ownership

Decision: desired project state and observed engine state are separate. Seq66
owns desired state, project structure, transport policy and UI. SooperLooper
feedback determines actual loop state, position, length, meters and applied
controls.

Consequences:

- commands have pending/confirmed/failed or indeterminate outcomes;
- UI renders cached observed state rather than optimistic command results;
- timeouts trigger reconciliation, not fabricated success.

## D-003 — support native JACK and PipeWire-JACK; reject ALSA-only audio

- Status: `accepted`
- Date: 2026-08-02
- Scope: audio backends

Decision: initial SooperLooper tracks require native JACK or PipeWire's JACK
compatibility. ALSA may still be used for Seq66 MIDI. Seq66 does not silently
launch JACK over ALSA.

Consequences:

- audio capability detection is explicit;
- configuration must explain why an ALSA-only audio setup cannot create or run
  SooperLooper tracks;
- future JACK-over-ALSA orchestration requires a new accepted decision.

## D-004 — unavailable audio is a hard state, not mute

- Status: `accepted`
- Date: 2026-08-02
- Scope: failure/UI semantics

Decision: clips loaded without a supported audio backend enter
`backend_unavailable`. They remain visible and persisted but cannot be launched,
recorded, unmuted or unlocked by MIDI, automation, UI or headless commands.

Rationale: mute means a valid engine loop exists and can resume. Backend absence
means no legal audio execution path exists.

## D-005 — UUID is stable identity; loop index is generation-scoped

- Status: `accepted`
- Date: 2026-08-02
- Scope: allocation and persistence

Decision: each audio clip has a stable UUID. SooperLooper indexes are transient
runtime handles valid only for one engine generation.

Consequences:

- restart invalidates index mappings;
- late feedback from prior generations is ignored;
- raw loop indexes are not persisted as identity;
- topology rebuild blocks conflicting commands.

## D-006 — protocol confirmation is eventually consistent

- Status: `accepted`
- Date: 2026-08-02
- Evidence: pinned SooperLooper 1.7.9 real-engine smoke test
- Scope: OSC command semantics

Decision: a successful OSC send and even an immediate `/get` do not necessarily
confirm that a `/set` has reached the audio engine. Confirmation waits for
observed state within a bounded deadline and may use repeated verification
queries.

Consequences:

- no send-and-immediate-read assumption;
- tests use state-based bounded polling/subscriptions;
- pending indicators remain until observed confirmation.

## D-007 — repository control files are the agent handoff authority

- Status: `accepted`
- Date: 2026-08-02
- Scope: multi-agent operations

Decision: project continuation uses `PROJECT-MANIFEST.json`, `CURRENT.md`,
`WORK-QUEUE.md`, traceability, changelog and immutable checkpoints. Chat history
or agent memory is never required state.

Consequences:

- every state-changing session leaves a checkpoint;
- task and phase transitions update all relevant control files coherently;
- CI checks structural metadata consistency;
- agents recover context using the bounded reading path in `WORKFLOW.md`.

## D-008 — loop allocation policy remains provisional

- Status: `provisional`
- Date: 2026-08-02
- Scope: loop topology

Current constraint: normal deletion uses only “remove last loop”, matching the
public SooperLooper safety guidance.

Open alternatives:

- fixed/preallocated pool;
- append/remove-last allocation;
- controlled full rebuild while stopped.

Resolution evidence required:

- topology and restart tests;
- memory/performance measurements;
- persistence/reconciliation design;
- Raspberry Pi representative-load validation.

## New decision template

```markdown
## D-NNN — title

- Status: accepted/provisional/superseded/rejected
- Date: YYYY-MM-DD
- Supersedes: D-NNN or none
- Evidence: source/test/issue/PR
- Scope: ...

Decision: ...

Rationale: ...

Consequences: ...

Reconsider when: ...
```
