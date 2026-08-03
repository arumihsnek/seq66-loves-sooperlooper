# Documentation map

## Read this before following any plan in the repository

This fork contains two documentation families:

1. inherited Seq66 upstream documentation;
2. canonical Seq66 Loves SooperLooper documentation.

Only the second family defines the fork's active roadmap, tasks, checkpoints,
product contract and autonomous-agent authority.

## Fast navigation by question

| Question | Open |
|---|---|
| What repository/branch/phase/task is active? | `/PROJECT-MANIFEST.json` |
| Who may decide, consult, merge or stop? | `/PROJECT-AUTONOMY.json`, `/AUTONOMY.md` |
| What happened most recently? | `checkpoints/CURRENT.md` |
| What should an agent do next? | `WORK-QUEUE.md` |
| How does a complete autonomous mission run? | `MISSION-LIFECYCLE.md` |
| How is the policy adopted without disrupting active work? | `AUTONOMY-ADOPTION.md` |
| How should common situations be classified? | `AUTONOMY-SCENARIOS.md` |
| How should an agent recover context and hand off? | `WORKFLOW.md`, `CHECKPOINTS.md`, `/AGENTS.md` |
| When is senior consultation required? | `SENIOR-CONSULTATION.md` |
| When may Hermes merge without asking? | `AUTONOMOUS-MERGE.md` |
| What decisions must be asked of the human? | `HUMAN-ESCALATION.md` |
| What is the long-term phase order? | `doc/sooperlooper/ROADMAP.md` |
| What behaviour is mandatory? | `SPECIFICATION.md` |
| Why are architecture choices fixed this way? | `ARCHITECTURE.md`, `DECISIONS.md` |
| What does SooperLooper send and receive? | `OSC-CONTROL-AND-FEEDBACK.md` |
| Which requirements have code/tests? | `TRACEABILITY.md` |
| What has been proven against the real engine? | `TESTED-BEHAVIOUR.md` |
| How are tests/builds reproduced? | `DEVELOPMENT.md`, `HEADLESS-TESTING.md` |
| What durable fork changes exist? | `/CHANGELOG-FORK.md` |
| How do we import Seq66 upstream fixes? | `UPSTREAM-SYNC.md` |
| Where are quick fork planning entry points? | root `/ROADMAP.md` and `/TODO` on `fork-main` |
| What did upstream Seq66 plan historically? | root `/ROADMAP.md` and `/TODO` on mirror branch `master` or `ahlstromcj/seq66` |
| What changed in upstream Seq66 releases? | `/NEWS`, upstream `/RELNOTES`, `/ChangeLog` — reference only |

## Canonical fork documents

These files may establish requirements, authority, task status or gates:

- `/PROJECT-MANIFEST.json` — machine-readable project pointer and compatibility
  manifest;
- `/PROJECT-AUTONOMY.json` — machine-readable autonomy, consultation, merge and
  escalation contract;
- `/AUTONOMY.md` — compact governance entry point;
- `/AGENTS.md` — mandatory repository-wide technical and safety rules;
- `/CHANGELOG-FORK.md` — durable fork changes;
- root `/ROADMAP.md` and `/TODO` on `fork-main` — concise entry points only;
- `README.md` before its clearly marked upstream section — user-facing fork
  introduction;
- this `doc/sooperlooper/` documentation set;
- `tests/audio/README.md` for executable test ownership;
- active PR body and CI only as evidence, not as substitutes for the control
  documents.

## Autonomous governance set

The autonomous operating model is intentionally split by responsibility:

- `AUTONOMY.md` — normative role and decision-level overview;
- `MISSION-LIFECYCLE.md` — end-to-end recover/implement/review/merge/checkpoint
  loop;
- `SENIOR-CONSULTATION.md` — when and how independent senior advice is used;
- `AUTONOMOUS-MERGE.md` — exact ordinary-task merge gate;
- `HUMAN-ESCALATION.md` — valid human questions and waiting behaviour;
- `AUTONOMY-SCENARIOS.md` — worked classification examples that act as policy
  acceptance cases;
- `AUTONOMY-ADOPTION.md` — safe stacked introduction and activation sequence;
- `templates/README.md` — template index;
- `templates/HUMAN-DECISION.md` — bounded one-question decision packet;
- `templates/AUTONOMOUS-REPORT.md` — sparse progress/blocker report;
- `/PROJECT-AUTONOMY.json` — CI-validated machine contract linking all of the
  above.

These documents complement, and do not weaken, `/AGENTS.md`, architecture,
specification or checkpoint requirements.

## Upstream inherited documents

The following remain useful references but cannot assign work to a fork agent:

- root `/ROADMAP.md` on `master` — upstream Seq66 v2 ideas;
- root `/TODO` on `master` — upstream Seq66 backlog, issues and experiments;
- `/ChangeLog`, `/NEWS`, upstream content in `/RELNOTES` — upstream history and
  release information;
- most pre-existing content under `/doc`, `/contrib` and platform installation
  notes — upstream feature/build documentation;
- source comments containing TODO/FIXME — local technical clues, not approved
  fork tasks.

The fork intentionally replaces only the root `ROADMAP.md` and `TODO` contents
on `fork-main` with compact pointers because these names are automatically read
by agents and the upstream versions are very large. The original files remain
unchanged on clean-mirror `master` and in `ahlstromcj/seq66`.

An item from inherited documentation becomes fork work only when it receives a
stable task ID in `WORK-QUEUE.md` and linked requirements/acceptance tests.

## Status-bearing documents

Only these documents contain live project status:

1. `PROJECT-MANIFEST.json`;
2. `checkpoints/CURRENT.md`;
3. `WORK-QUEUE.md`;
4. `TRACEABILITY.md`;
5. active PR/CI evidence.

`PROJECT-AUTONOMY.json` contains durable authority and workflow policy, not live
task status. `doc/sooperlooper/ROADMAP.md` gives phase intent. Neither substitutes
for the work queue or current checkpoint.

## History-bearing documents

- immutable checkpoint files preserve operational handoffs;
- `CHANGELOG-FORK.md` preserves notable fork changes;
- `DECISIONS.md` preserves product/architecture rationale;
- `TESTED-BEHAVIOUR.md` preserves revision-specific runtime evidence;
- Git commits and PR reviews preserve implementation history;
- mirror branch `master` preserves inherited upstream planning/history files.

Do not overload one file to serve all these purposes.

## Modification ownership

| Document | Normal owner during parallel work |
|---|---|
| `PROJECT-MANIFEST.json` | integration/coordinator agent |
| `PROJECT-AUTONOMY.json` and autonomy contracts | governance/integration owner plus senior review |
| `checkpoints/CURRENT.md` | session or integration owner at handoff |
| immutable checkpoint | session owner |
| `WORK-QUEUE.md` | task owner for its row; coordinator for reprioritization |
| `TRACEABILITY.md` | task owner for affected IDs |
| `SPECIFICATION.md` | feature owner plus reviewer |
| `DECISIONS.md` | architecture/product decision owner |
| `CHANGELOG-FORK.md` | integration owner or task owner for its change |
| root fork `ROADMAP.md`/`TODO` | integration owner; keep as concise pointers |
| other upstream inherited docs | modify only for necessary fork behaviour/corrections |

A governance-policy edit must update human-readable and machine-readable
contracts together and pass `validate-autonomy-policy.py`.

## Preventing stale duplicate plans

Do not create additional active files named variations of:

- roadmap;
- plan;
- TODO;
- status;
- handoff;
- backlog;
- manifest;
- architecture decision record;
- autonomy or agent governance policy.

Extend the canonical file instead. A temporary investigation note must state its
owner, expiry/decision target and must be removed or promoted before task
completion.

## Archiving obsolete fork documents

When a fork document is superseded:

1. create a decision/checkpoint identifying its replacement;
2. move it under `doc/sooperlooper/archive/` only if historical content remains
   valuable;
3. prepend `SUPERSEDED` with date and replacement link;
4. remove it from the manifest/index/control path;
5. ensure CI no longer treats it as active;
6. never archive upstream files merely because they are not active fork plans.
