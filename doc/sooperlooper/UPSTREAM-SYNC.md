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

The GitHub default branch should become `fork-main` after the bootstrap PR is
merged. Until repository settings are switched, the manifest and PR base are
the authoritative indicators of the active product line.

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

On `master` and in `ahlstromcj/seq66`, these are upstream references:

- `/ROADMAP.md`;
- `/TODO`;
- `/ChangeLog`;
- `/NEWS`;
- upstream release content inside `/RELNOTES`;
- upstream manuals and planning notes under `/doc` unless the fork index says
  otherwise.

They describe Seq66 upstream history, ideas, releases and outstanding MIDI
work. They may reveal useful fixes or conflicts, but agents MUST NOT select fork
tasks from them automatically.

### Canonical fork control and planning

On `fork-main` and its feature branches:

- `/PROJECT-MANIFEST.json`;
- `/AGENTS.md`;
- `/CHANGELOG-FORK.md`;
- root `/ROADMAP.md` and `/TODO` as concise fork entry points;
- `/doc/sooperlooper/README.md`;
- `/doc/sooperlooper/WORKFLOW.md`;
- `/doc/sooperlooper/WORK-QUEUE.md`;
- `/doc/sooperlooper/ROADMAP.md`;
- `/doc/sooperlooper/SPECIFICATION.md`;
- `/doc/sooperlooper/TRACEABILITY.md`;
- `/doc/sooperlooper/DECISIONS.md`;
- `/doc/sooperlooper/checkpoints/CURRENT.md`.

## Intentional root planning-file divergence

The fork integration line intentionally replaces the inherited root `TODO` and
`ROADMAP.md` contents with short fork entry points. This is a deliberate
exception to the general preference for low-conflict inherited documentation.

Reasons:

- these two filenames are highly likely to be read automatically by agents;
- the upstream files contain a large unrelated backlog and speculative v2 plan;
- a warning banner above hundreds of upstream lines still wastes context and
  risks accidental task selection;
- the complete originals remain losslessly available on clean-mirror `master`
  and in `ahlstromcj/seq66`.

Consequences:

- every upstream sync must review conflicts in these two files;
- sync resolution normally keeps the fork entry-point versions on `fork-main`;
- relevant upstream roadmap/TODO changes are assessed as possible compatibility
  information, not copied into the fork queue automatically;
- an upstream item enters fork planning only through a stable task ID,
  requirements and acceptance tests.

Other inherited documentation should remain at original paths unless there is a
similarly explicit decision and checkpoint.

## Avoiding documentation conflicts

- Keep the clean upstream originals on `master`.
- Keep fork planning under `doc/sooperlooper/`.
- Use `CHANGELOG-FORK.md` rather than rewriting upstream history.
- Modify inherited manuals only for actual fork user-facing behaviour.
- Isolate fork additions in inherited release/install files and review them on
  every sync.
- Never bulk-rename the inherited documentation tree merely to label it.
- Record any new intentional divergence in `DOCUMENTATION-MAP.md`, decisions,
  changelog and a checkpoint.

## Upstream compatibility review

Every sync reviews at least:

- changes to `sequence`, `performer`, session/persistence and Qt grid classes;
- Meson options/dependencies and liblo/JACK detection;
- transport and JACK behaviour;
- configuration format and SeqSpec changes;
- thread ownership and shutdown;
- documentation claims about audio, PipeWire/JACK or headless operation;
- upstream changes to root `TODO` and `ROADMAP.md` for useful fixes or conflicts.

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
