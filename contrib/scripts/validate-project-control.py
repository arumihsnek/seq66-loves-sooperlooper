#!/usr/bin/env python3
"""Validate the Seq66 Loves SooperLooper project-control plane.

This deliberately checks structure and a few high-value semantic invariants.
It cannot replace human review of whether a checkpoint accurately describes
implementation state.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = ROOT / "PROJECT-MANIFEST.json"


class ValidationError(RuntimeError):
    """Raised for one or more project-control validation failures."""


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def read_text(relative_path: str, errors: list[str]) -> str:
    path = ROOT / relative_path
    if not path.is_file():
        fail(errors, f"missing required file: {relative_path}")
        return ""
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        fail(errors, f"not valid UTF-8: {relative_path}: {exc}")
        return ""


def require_mapping(
    value: Any, name: str, errors: list[str]
) -> dict[str, Any]:
    if not isinstance(value, dict):
        fail(errors, f"{name} must be a JSON object")
        return {}
    return value


def require_string(
    mapping: dict[str, Any], key: str, context: str, errors: list[str]
) -> str:
    value = mapping.get(key)
    if not isinstance(value, str) or not value.strip():
        fail(errors, f"{context}.{key} must be a non-empty string")
        return ""
    return value


def extract_task_status(work_queue: str, task_id: str) -> str | None:
    task_pattern = re.compile(
        rf"^###\s+{re.escape(task_id)}\b.*?^Status:\s*`?([a-z_]+)`?\s*$",
        re.MULTILINE | re.DOTALL,
    )
    match = task_pattern.search(work_queue)
    return match.group(1) if match else None


def extract_current_checkpoint_target(current_text: str) -> str | None:
    explicit = re.search(
        r"^Immutable checkpoint:\s*`?([^`\n]+)`?\s*$",
        current_text,
        re.MULTILINE,
    )
    if explicit:
        return explicit.group(1).strip()

    link = re.search(
        r"\[[^\]]+\]\(([^)]+checkpoints/[^)]+\.md)\)", current_text
    )
    if link:
        return link.group(1).strip()
    return None


def validate() -> list[str]:
    errors: list[str] = []

    if not MANIFEST_PATH.is_file():
        return ["missing PROJECT-MANIFEST.json"]

    try:
        manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        return [f"invalid PROJECT-MANIFEST.json: {exc}"]

    if manifest.get("schema_version") != 1:
        fail(errors, "schema_version must currently equal 1")

    repository = require_mapping(
        manifest.get("repository"), "repository", errors
    )
    coordination = require_mapping(
        manifest.get("coordination"), "coordination", errors
    )
    next_milestone = require_mapping(
        manifest.get("next_milestone"), "next_milestone", errors
    )

    full_name = require_string(
        repository, "full_name", "repository", errors
    )
    if full_name and full_name != "arumihsnek/seq66-loves-sooperlooper":
        fail(errors, f"unexpected repository.full_name: {full_name}")

    mirror_branch = require_string(
        repository, "upstream_mirror_branch", "repository", errors
    )
    integration_branch = require_string(
        repository, "integration_branch", "repository", errors
    )
    active_branch = require_string(
        repository, "active_branch", "repository", errors
    )
    pr_base = require_string(
        repository, "active_pull_request_base", "repository", errors
    )

    if mirror_branch and mirror_branch != "master":
        fail(errors, "upstream_mirror_branch must be master")
    if integration_branch and integration_branch != "fork-main":
        fail(errors, "integration_branch must be fork-main")
    if pr_base and integration_branch and pr_base != integration_branch:
        fail(
            errors,
            "active_pull_request_base must equal repository.integration_branch",
        )
    if active_branch in {mirror_branch, integration_branch}:
        fail(
            errors,
            "active implementation work must use a feature/fix/docs/sync branch",
        )

    active_pr = repository.get("active_pull_request")
    if not isinstance(active_pr, int) or active_pr <= 0:
        fail(errors, "repository.active_pull_request must be a positive integer")

    path_keys = {
        "current_checkpoint",
        "checkpoint_protocol",
        "work_queue",
        "roadmap",
        "workflow",
        "traceability",
        "decision_log",
        "documentation_map",
        "upstream_sync_policy",
        "fork_changelog",
        "agent_instructions",
    }
    referenced_paths: dict[str, str] = {}
    for key in sorted(path_keys):
        relative = require_string(coordination, key, "coordination", errors)
        if relative:
            referenced_paths[key] = relative
            if not (ROOT / relative).is_file():
                fail(errors, f"coordination.{key} does not exist: {relative}")

    canonical_required = [
        "README.md",
        "ROADMAP.md",
        "TODO",
        "AGENTS.md",
        "CHANGELOG-FORK.md",
        "doc/sooperlooper/README.md",
        "doc/sooperlooper/ARCHITECTURE.md",
        "doc/sooperlooper/SPECIFICATION.md",
        "doc/sooperlooper/OSC-CONTROL-AND-FEEDBACK.md",
        "doc/sooperlooper/HEADLESS-TESTING.md",
        "doc/sooperlooper/TESTED-BEHAVIOUR.md",
        "doc/sooperlooper/WORKFLOW.md",
        "doc/sooperlooper/WORK-QUEUE.md",
        "doc/sooperlooper/CHECKPOINTS.md",
        "doc/sooperlooper/TRACEABILITY.md",
        "doc/sooperlooper/DECISIONS.md",
        "doc/sooperlooper/DOCUMENTATION-MAP.md",
        "doc/sooperlooper/UPSTREAM-SYNC.md",
        "tests/audio/README.md",
    ]
    for relative in canonical_required:
        if not (ROOT / relative).is_file():
            fail(errors, f"missing canonical control/document file: {relative}")

    current_path = referenced_paths.get("current_checkpoint", "")
    current_text = read_text(current_path, errors) if current_path else ""
    checkpoint_target = extract_current_checkpoint_target(current_text)
    if not checkpoint_target:
        fail(
            errors,
            "CURRENT.md must declare 'Immutable checkpoint: <path>' or link to it",
        )
    else:
        checkpoint_target = checkpoint_target.removeprefix("../../")
        checkpoint_target = checkpoint_target.removeprefix("../")
        if not checkpoint_target.startswith("doc/sooperlooper/checkpoints/"):
            fail(
                errors,
                "current immutable checkpoint must live under "
                "doc/sooperlooper/checkpoints/",
            )
        elif checkpoint_target.endswith("/CURRENT.md"):
            fail(errors, "CURRENT.md cannot point to itself")
        elif not (ROOT / checkpoint_target).is_file():
            fail(errors, f"immutable checkpoint does not exist: {checkpoint_target}")
        else:
            checkpoint_text = read_text(checkpoint_target, errors)
            required_checkpoint_headings = [
                "## Objective",
                "## Completed",
                "## Verification",
                "## Current state",
                "## Risks and unresolved questions",
                "## Next executable action",
                "## Open first",
                "## Safe reference point",
            ]
            for heading in required_checkpoint_headings:
                if heading not in checkpoint_text:
                    fail(
                        errors,
                        f"checkpoint {checkpoint_target} missing heading: {heading}",
                    )

    current_phase = require_string(
        coordination, "current_phase", "coordination", errors
    )
    completed_phase = require_string(
        coordination, "completed_phase", "coordination", errors
    )
    active_task = require_string(
        next_milestone, "active_task", "next_milestone", errors
    )

    work_queue_path = referenced_paths.get("work_queue", "")
    work_queue = read_text(work_queue_path, errors) if work_queue_path else ""
    if active_task:
        task_status = extract_task_status(work_queue, active_task)
        if task_status is None:
            fail(errors, f"active task not found in work queue: {active_task}")
        elif task_status not in {"ready", "in_progress", "review"}:
            fail(
                errors,
                f"active task {active_task} has non-active status: {task_status}",
            )

    if current_text:
        if current_phase and current_phase not in current_text:
            fail(errors, "CURRENT.md does not mention manifest current_phase")
        if active_task and active_task not in current_text:
            fail(errors, "CURRENT.md does not mention manifest active_task")

    roadmap_path = referenced_paths.get("roadmap", "")
    roadmap = read_text(roadmap_path, errors) if roadmap_path else ""
    phase_tokens = {
        "phase-0-project-contract": "Phase 0",
        "phase-1-protocol-core": "Phase 1",
        "phase-2-managed-engine-backend": "Phase 2",
        "phase-3-performer-integration": "Phase 3",
        "phase-4-native-qt-slot": "Phase 4",
        "phase-5-exact-recording": "Phase 5",
        "phase-6-persistence": "Phase 6",
        "phase-7-advanced-controls": "Phase 7",
        "phase-8-hardening-release": "Phase 8",
    }
    for phase_value, label in (
        (completed_phase, "completed_phase"),
        (current_phase, "current_phase"),
    ):
        token = phase_tokens.get(phase_value)
        if not token:
            fail(errors, f"unknown {label}: {phase_value}")
        elif token not in roadmap:
            fail(errors, f"roadmap does not contain {label} token: {token}")

    root_roadmap = read_text("ROADMAP.md", errors)
    if "Seq66 Loves SooperLooper" not in root_roadmap:
        fail(errors, "root ROADMAP.md is not the fork roadmap entry point")
    if "doc/sooperlooper/WORK-QUEUE.md" not in root_roadmap:
        fail(errors, "root ROADMAP.md must link the canonical work queue")

    root_todo = read_text("TODO", errors)
    if "does not use an unstructured root TODO list" not in root_todo:
        fail(errors, "root TODO must reject unstructured fork task tracking")
    if "doc/sooperlooper/WORK-QUEUE.md" not in root_todo:
        fail(errors, "root TODO must point to the canonical work queue")

    ci_entries = manifest.get("ci")
    if not isinstance(ci_entries, list) or not ci_entries:
        fail(errors, "ci must be a non-empty list")
    else:
        required_workflows: set[str] = set()
        for index, entry in enumerate(ci_entries):
            if not isinstance(entry, dict):
                fail(errors, f"ci[{index}] must be an object")
                continue
            workflow = entry.get("workflow")
            required = entry.get("required")
            if not isinstance(workflow, str) or not workflow:
                fail(errors, f"ci[{index}].workflow must be a non-empty string")
                continue
            if not (ROOT / workflow).is_file():
                fail(errors, f"manifest workflow does not exist: {workflow}")
            if required is True:
                required_workflows.add(workflow)
        expected = {
            ".github/workflows/project-control.yml",
            ".github/workflows/audio-core.yml",
            ".github/workflows/sooperlooper-real-engine.yml",
        }
        missing = expected - required_workflows
        if missing:
            fail(
                errors,
                "required workflow entries missing from manifest: "
                + ", ".join(sorted(missing)),
            )

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("Project-control validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("Project-control validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
