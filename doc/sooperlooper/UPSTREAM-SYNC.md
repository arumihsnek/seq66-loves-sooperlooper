# Upstream synchronization and branch policy

## Goal

Preserve a clean relationship with `ahlstromcj/seq66` so the fork can adopt
upstream fixes without mixing upstream history, fork product planning and active
agent work.

## Permanent branches

### `master` — upstream mirror

`master` is reserved as a clean mirror of `ahlstromcj/seq66:master`.

Rules:

- no fork-specific commits;
- no feature PRs target `master`;
- update only by verified fast-forward from upstream where possible;
- its head identifies the latest imported upstream state;
- if local and upstream `master` diverge unexpectedly, stop and checkpoint
  before rewriting history.

### `fork-main` — fork integration branch

`fork-main` is the stable integration line for Seq66 Loves SooperLooper.

Rules:

- feature and fix PRs target `fork-main`;
- it may contain fork code, documentation and compatibility adaptations;
- it receives upstream updates through explicit sync PRs;
- it must remain buildable according to the currently required workflows;
- direct pushes are avoided except repository-control repairs authorized by the
  project owner.

The GitHub default branch should eventually be `fork-main` once the first
integration milestone is merged and repository settings permit the switch.
Until then, the README and manifest must make the active product branch clear.

## Temporary branches

Use:

- `feature/<scope>` for coherent product work;
- `fix/<scope>` for fork defects;
- `docs/<scope>` for documentation/control-only work;
- `sync/upstream-YYYY-MM-DD` for importing an upstream update;
- `experiment/<scope>` for disposable investigations that do not merge without
  a reviewed conversion into a normal task.

Do not use one indefinitely growing feature branch after the bootstrap PR.
After PR #1 establishes the project foundation, subsequent roadmap tasks should
normally use smaller branches and PRs against `fork-main`.

## Normal upstream sync procedure

1. Verify `master` currently contains no fork-only commits.
2. Fetch/inspect `ahlstromcj/seq66:master` and record its new head.
3. Fast-forward fork `master` to that exact upstream commit.
4. Create `sync/upstream-YYYY-MM-DD` from current `fork-main`.
5. Merge upstream-mirror `master` into the sync branch. Do not rebase the shared
   `fork-main` history without explicit authorization.
6. Resolve conflicts while preserving fork invariants and upstream behaviour.
7. Update compatibility references in `PROJECT-MANIFEST.json`.
8. Review upstream `NEWS`, `RELNOTES`, `ChangeLog`, `ROADMAP.md`, `TODO`, build
   files and audio-adjacent source changes.
9. Run MIDI/regression checks plus all required audio workflows.
10. Record relevant imported fixes in `CHANGELOG-FORK.md` under an upstream-sync
    entry.
11. Write a checkpoint with old/new upstream SHAs, conflicts, tests and risks.
12. Merge the sync PR into `fork-main` only after review.

## Urgent upstream fixes

For one isolated critical fix, a `fix/upstream-<issue>` branch may cherry-pick a
specific upstream commit onto `fork-main`.

The commit/PR must record:

- upstream SHA and issue/PR;
- whether dependencies were omitted;
- why a full sync was not used;
- tests proving the cherry-pick is safe;
- whether the commit will be deduplicated during the next full sync.

Prefer full sync when the fix depends on surrounding refactors.

## Upstream documentation classification

Files inherited from Seq66 remain valuable but do not all govern this fork.

### Upstream reference, not active fork planning

- `/ROADMAP.md`;
- `/TODO`;
- `/ChangeLog`;
- `/NEWS`;
- upstream release content inside `/RELNOTES`;
- upstream manuals and planning notes under `/doc` unless the fork index says
  otherwise.

These files describe Seq66 upstream history, ideas, releases and outstanding
MIDI work. They may reveal useful fixes or conflicts, but agents MUST NOT select
fork tasks from them automatically.

### Canonical fork control and planning

- `/PROJECT-MANIFEST.json`;
- `/AGENTS.md`;
- `/CHANGELOG-FORK.md`;
- `/doc/sooperlooper/README.md`;
- `/doc/sooperlooper/WORKFLOW.md`;
- `/doc/sooperlooper/WORK-QUEUE.md`;
- `/doc/sooperlooper/ROADMAP.md`;
- `/doc/sooperlooper/SPECIFICATION.md`;
- `/doc/sooperlooper/TRACEABILITY.md`;
- `/doc/sooperlooper/DECISIONS.md`;
- `/doc/sooperlooper/checkpoints/CURRENT.md`.

## Avoiding documentation conflicts

Do not rename or relocate large upstream documentation trees merely to mark them
as upstream. That causes unnecessary conflicts on every import.

Instead:

- keep original paths;
- add a short fork banner to only the most ambiguous top-level planning files;
- maintain the canonical classification in this document and
  `DOCUMENTATION-MAP.md`;
- keep fork planning under `doc/sooperlooper/`;
- use `CHANGELOG-FORK.md` rather than rewriting upstream history;
- when an upstream file must be changed for product reasons, isolate the fork
  addition clearly and expect to review it during every sync.

## Upstream compatibility review

Every sync reviews at least:

- changes to `sequence`, `performer`, session/persistence and Qt grid classes;
- Meson options/dependencies and liblo/JACK detection;
- transport and JACK behaviour;
- configuration format and SeqSpec changes;
- thread ownership and shutdown;
- documentation claims about audio, PipeWire/JACK or headless operation.

Record compatibility findings in the sync checkpoint. Do not silently assume an
upstream build passing means the fork integration remains semantically correct.

## Release ancestry

Every fork release records:

- fork tag/version;
- exact `fork-main` commit;
- nearest imported upstream Seq66 commit/version;
- exact tested SooperLooper commit/version;
- supported backend matrix;
- required migration and persistence schema version.
