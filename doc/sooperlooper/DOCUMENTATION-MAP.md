# Documentation map

## Read this before following any plan in the repository

This fork contains two documentation families:

1. inherited Seq66 upstream documentation;
2. canonical Seq66 Loves SooperLooper documentation.

Only the second family defines the fork's active roadmap, tasks, checkpoints and
product contract.

## Fast navigation by question

| Question | Open |
|---|---|
| What repository/branch/phase/task is active? | `/PROJECT-MANIFEST.json` |
| What happened most recently? | `checkpoints/CURRENT.md` |
| What should an agent do next? | `WORK-QUEUE.md` |
| How should an agent recover context and hand off? | `WORKFLOW.md`, `CHECKPOINTS.md`, `/AGENTS.md` |
| What is the long-term phase order? | `doc/sooperlooper/ROADMAP.md` |
| What behaviour is mandatory? | `SPECIFICATION.md` |
| Why are architecture choices fixed this way? | `ARCHITECTURE.md`, `DECISIONS.md` |
| What does SooperLooper send and receive? | `OSC-CONTROL-AND-FEEDBACK.md` |
| Which requirements have code/tests? | `TRACEABILITY.md` |
| What has been proven against the real engine? | `TESTED-BEHAVIOUR.md` |
| How are tests/builds reproduced? | `DEVELOPMENT.md`, `HEADLESS-TESTING.md` |
| What durable fork changes exist? | `/CHANGELOG-FORK.md` |
| How do we import Seq66 upstream fixes? | `UPSTREAM-SYNC.md` |
| What did upstream Seq66 plan historically? | `/ROADMAP.md`, `/TODO` — reference only |
| What changed in upstream Seq66 releases? | `/NEWS`, `/RELNOTES`, `/ChangeLog` — reference only |

## Canonical fork documents

These files may establish requirements, task status or gates:

- `/PROJECT-MANIFEST.json` — machine-readable project pointer and compatibility
  manifest;
- `/AGENTS.md` — mandatory repository-wide agent rules;
- `/CHANGELOG-FORK.md` — durable fork changes;
- `README.md` before its clearly marked upstream section — user-facing fork
  introduction;
- this `doc/sooperlooper/` documentation set;
- `tests/audio/README.md` for executable test ownership;
- active PR body and CI only as evidence, not as substitutes for the control
  documents.

## Upstream inherited documents

The following remain useful references but cannot assign work to a fork agent:

- root `/ROADMAP.md` — upstream Seq66 v2 ideas;
- root `/TODO` — upstream Seq66 backlog, issues and experiments;
- `/ChangeLog`, `/NEWS`, upstream content in `/RELNOTES` — upstream history and
  release information;
- most pre-existing content under `/doc`, `/contrib` and platform installation
  notes — upstream feature/build documentation;
- source comments containing TODO/FIXME — local technical clues, not approved
  fork tasks.

An item from inherited documentation becomes fork work only when it is assigned
a stable task ID in `WORK-QUEUE.md` and linked to requirements/acceptance tests.

## Status-bearing documents

Only these documents contain live project status:

1. `PROJECT-MANIFEST.json`;
2. `checkpoints/CURRENT.md`;
3. `WORK-QUEUE.md`;
4. `TRACEABILITY.md`;
5. active PR/CI evidence.

`ROADMAP.md` gives phase intent. It must not contain volatile session notes or
be used as a substitute for the work queue.

## History-bearing documents

- immutable checkpoint files preserve operational handoffs;
- `CHANGELOG-FORK.md` preserves notable fork changes;
- `DECISIONS.md` preserves product/architecture rationale;
- `TESTED-BEHAVIOUR.md` preserves revision-specific runtime evidence;
- Git commits and PR reviews preserve implementation history.

Do not overload one file to serve all these purposes.

## Modification ownership

| Document | Normal owner during parallel work |
|---|---|
| `PROJECT-MANIFEST.json` | integration/coordinator agent |
| `checkpoints/CURRENT.md` | session or integration owner at handoff |
| immutable checkpoint | session owner |
| `WORK-QUEUE.md` | task owner for its row; coordinator for reprioritization |
| `TRACEABILITY.md` | task owner for affected IDs |
| `SPECIFICATION.md` | feature owner plus reviewer |
| `DECISIONS.md` | architecture/product decision owner |
| `CHANGELOG-FORK.md` | integration owner or task owner for its change |
| upstream inherited docs | modify only for necessary fork banners/corrections |

## Preventing stale duplicate plans

Do not create additional files named variations of:

- roadmap;
- plan;
- TODO;
- status;
- handoff;
- backlog;
- manifest;
- architecture decision record.

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
