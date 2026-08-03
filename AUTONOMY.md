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
- `doc/sooperlooper/AUTONOMY-SCENARIOS.md`
- `doc/sooperlooper/AUTONOMY-ADOPTION.md`

Templates:

- `doc/sooperlooper/templates/HUMAN-DECISION.md`
- `doc/sooperlooper/templates/AUTONOMOUS-REPORT.md`

A new Hermes, Codex or human coordinator reads this file immediately after
`PROJECT-MANIFEST.json`, before `checkpoints/CURRENT.md` and the active
`WORK-QUEUE.md` task.

The default mode is autonomous execution inside the approved roadmap and
specification. Routine technical decisions are owned by the operator and
reviewed by `codex-senior-consult` when required. Human interruption is reserved
for product choices, destructive or irreversible actions, incompatible
licensing, unresolved requirement conflicts, unavailable physical/subjective
validation and milestone gates.

Ordinary task PRs may be merged autonomously only through the exact-head gate in
`AUTONOMOUS-MERGE.md`. Closing a milestone and opening the next remains one
explicit bounded human decision.

No chat prompt may weaken the repository's safety stops, architectural
invariants, required checks, expected-head protection or evidence requirements.
A prompt may temporarily narrow autonomy, but expanding autonomy requires a
reviewed policy change.
