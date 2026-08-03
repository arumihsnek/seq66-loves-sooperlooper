# Autonomous project governance

This is the compact entry point for the repository's autonomous operating
model.

Read after `PROJECT-MANIFEST.json` and before `checkpoints/CURRENT.md`:

1. `PROJECT-AUTONOMY.json` — machine-readable authority and gates;
2. `doc/sooperlooper/AUTONOMY.md` — normative human-readable policy;
3. `doc/sooperlooper/MISSION-LIFECYCLE.md` — end-to-end task and phase loop;
4. `doc/sooperlooper/SENIOR-CONSULTATION.md` — independent review contract;
5. `doc/sooperlooper/AUTONOMOUS-MERGE.md` — exact-head merge and phase gate;
6. `doc/sooperlooper/HUMAN-ESCALATION.md` — genuine human decisions;
7. `doc/sooperlooper/AUTONOMY-SCENARIOS.md` — worked examples;
8. `doc/sooperlooper/AUTONOMY-ADOPTION.md` — adoption and rollback.

## Default mode

Hermes executes the approved roadmap autonomously. It implements, tests,
repairs CI, consults `codex-senior-consult`, merges eligible pull requests,
writes checkpoints and continues with the next task.

A clear phase transition is also autonomous when:

- the current phase definition of done is satisfied;
- all required checks pass on the exact integrated head;
- a global senior phase review accepts without blockers;
- the next phase is already bounded in the approved roadmap;
- no L3 or L4 trigger exists.

A phase boundary is not automatically a human gate.

## Human interaction

Hermes asks the human only for genuine L3/L4 matters: product scope, subjective
musical or visual behaviour, intentional incompatibility, licensing, data loss,
irreversible migration, unresolved requirement conflict, unavailable physical or
subjective acceptance, or a safety stop.

Whenever Hermes supports native interactive selection forms, it MUST use them
for human questions. Forms should contain two to four concrete options and,
when safe, an `Otra opción / Other` choice that opens free-text input. Plain chat
is only the fallback when the form capability is unavailable or cannot represent
the decision safely.

## Safety

Every autonomous merge and phase transition uses exact-head CI and
`expected-head` protection.

Autonomy never permits force push, shared-history rewrite, red-check merges,
stale senior verdicts, changed expected heads, silent requirement relaxation,
destructive actions without authority or hidden scope changes.

No session prompt may weaken these rules. Expanding authority requires a
reviewed change to the versioned policy and its validator.
