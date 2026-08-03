# Autonomous project governance

This file is the compact entry point for the repository's autonomous-agent
operating model.

Normative human-readable policy:

- `doc/sooperlooper/AUTONOMY.md`

Machine-readable policy validated by CI:

- `PROJECT-AUTONOMY.json`

Supporting contracts:

- `doc/sooperlooper/MISSION-LIFECYCLE.md`
- `doc/sooperlooper/SENIOR-CONSULTATION.md`
- `doc/sooperlooper/AUTONOMOUS-MERGE.md`
- `doc/sooperlooper/HUMAN-ESCALATION.md`

A new Hermes, Codex or human coordinator reads this file after
`PROJECT-MANIFEST.json`, `checkpoints/CURRENT.md` and the active
`WORK-QUEUE.md` task.

The default mode is autonomous execution inside the approved roadmap and
specification. Routine technical decisions are owned by the operator and
reviewed by `codex-senior-consult` when required. Human interruption is reserved
for product choices, destructive or irreversible actions, incompatible
licensing, unresolved requirement conflicts, unavailable physical validation
and milestone gates.

No chat prompt may weaken the repository's safety stops, architectural
invariants or evidence requirements. A prompt may temporarily narrow autonomy,
but expanding autonomy requires a reviewed policy change.
