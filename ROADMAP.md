# Roadmap — Seq66 Loves SooperLooper

This file is the root entry point for the fork roadmap.

The active phased roadmap is:

- [`doc/sooperlooper/ROADMAP.md`](doc/sooperlooper/ROADMAP.md)

The actionable task queue is:

- [`doc/sooperlooper/WORK-QUEUE.md`](doc/sooperlooper/WORK-QUEUE.md)

The current phase, branch, PR and task are recorded in:

- [`PROJECT-MANIFEST.json`](PROJECT-MANIFEST.json)
- [`doc/sooperlooper/checkpoints/CURRENT.md`](doc/sooperlooper/checkpoints/CURRENT.md)

## Important: upstream roadmap

The file previously at this path was the inherited **“ROADMAP for a Possible
Seq66 v. 2”** written for upstream Seq66. It is not the roadmap for this fork.

The unmodified upstream roadmap remains available on the upstream-mirror
`master` branch and in `ahlstromcj/seq66`.

Agents MUST NOT derive fork tasks from the upstream roadmap. An upstream idea
becomes fork work only after it receives a stable task ID in `WORK-QUEUE.md`,
requirements in `SPECIFICATION.md`/`TRACEABILITY.md`, and acceptance tests.

## Branch model

- `master`: clean mirror of Seq66 upstream;
- `fork-main`: stable integration line for this fork;
- `feature/*`, `fix/*`, `docs/*`: reviewed work targeting `fork-main`;
- `sync/upstream-YYYY-MM-DD`: explicit upstream import branches.

See [`doc/sooperlooper/UPSTREAM-SYNC.md`](doc/sooperlooper/UPSTREAM-SYNC.md).
