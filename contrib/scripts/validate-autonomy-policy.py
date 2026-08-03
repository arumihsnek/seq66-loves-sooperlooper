#!/usr/bin/env python3
"""Validate the repository's autonomous-agent governance contract.

The autonomy policy is deliberately independent of volatile project state such
as the active branch, task or checkpoint. This prevents governance work from
conflicting with an operator that is advancing the implementation in parallel.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
POLICY_PATH = ROOT / "PROJECT-AUTONOMY.json"


class PolicyError(RuntimeError):
    """Raised when the autonomy policy cannot be loaded."""


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise PolicyError(
            f"missing required JSON file: {path.relative_to(ROOT)}"
        ) from exc
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PolicyError(
            f"invalid JSON in {path.relative_to(ROOT)}: {exc}"
        ) from exc
    if not isinstance(value, dict):
        raise PolicyError(
            f"{path.relative_to(ROOT)} must contain a JSON object"
        )
    return value


def require_mapping(
    value: Any, name: str, errors: list[str]
) -> dict[str, Any]:
    if not isinstance(value, dict):
        errors.append(f"{name} must be an object")
        return {}
    return value


def require_list(value: Any, name: str, errors: list[str]) -> list[Any]:
    if not isinstance(value, list) or not value:
        errors.append(f"{name} must be a non-empty list")
        return []
    return value


def require_string(value: Any, name: str, errors: list[str]) -> str:
    if not isinstance(value, str) or not value.strip():
        errors.append(f"{name} must be a non-empty string")
        return ""
    return value


def require_members(
    container: list[Any],
    required: set[str],
    name: str,
    errors: list[str],
) -> None:
    strings = {item for item in container if isinstance(item, str)}
    missing = required - strings
    if missing:
        errors.append(
            f"{name} missing required values: {', '.join(sorted(missing))}"
        )


def require_file(relative: str, errors: list[str]) -> None:
    if not (ROOT / relative).is_file():
        errors.append(f"missing autonomy document/file: {relative}")


def require_text_tokens(
    relative: str,
    tokens: tuple[str, ...],
    errors: list[str],
) -> None:
    path = ROOT / relative
    if not path.is_file():
        errors.append(f"missing required integration file: {relative}")
        return
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        errors.append(f"not valid UTF-8: {relative}: {exc}")
        return
    for token in tokens:
        if token not in text:
            errors.append(f"{relative} missing required token: {token}")


def validate() -> list[str]:
    errors: list[str] = []

    try:
        policy = load_json(POLICY_PATH)
    except PolicyError as exc:
        return [str(exc)]

    if policy.get("schema_version") != 1:
        errors.append("PROJECT-AUTONOMY.json schema_version must equal 1")
    if policy.get("policy_id") != "seq66-sl-autonomy-v1":
        errors.append(
            "PROJECT-AUTONOMY.json policy_id must equal seq66-sl-autonomy-v1"
        )
    if policy.get("mode") != "autonomous_by_default":
        errors.append(
            "PROJECT-AUTONOMY.json mode must be autonomous_by_default"
        )

    roles = require_mapping(policy.get("roles"), "roles", errors)
    required_roles = {
        "human_owner",
        "hermes_operator",
        "codex_senior_consult",
        "repository_and_ci",
    }
    missing_roles = required_roles - set(roles)
    if missing_roles:
        errors.append(f"roles missing: {', '.join(sorted(missing_roles))}")

    for role in sorted(required_roles):
        entry = require_mapping(roles.get(role), f"roles.{role}", errors)
        require_list(entry.get("authority"), f"roles.{role}.authority", errors)
        require_string(
            entry.get("responsibility", entry.get("normal_interaction")),
            f"roles.{role}.responsibility_or_normal_interaction",
            errors,
        )

    levels = require_mapping(
        policy.get("decision_levels"), "decision_levels", errors
    )
    expected_levels = {
        "L1_AUTONOMOUS": False,
        "L2_SENIOR_REQUIRED": False,
        "L3_HUMAN_REQUIRED": True,
        "L4_SAFETY_STOP": True,
    }
    for level, human_interrupt in expected_levels.items():
        entry = require_mapping(
            levels.get(level), f"decision_levels.{level}", errors
        )
        if entry and entry.get("human_interrupt") is not human_interrupt:
            errors.append(
                f"decision_levels.{level}.human_interrupt must be "
                f"{human_interrupt}"
            )
        require_string(
            entry.get("actor"), f"decision_levels.{level}.actor", errors
        )
        require_list(
            entry.get("examples"), f"decision_levels.{level}.examples", errors
        )

    task_loop = require_list(policy.get("task_loop"), "task_loop", errors)
    require_members(
        task_loop,
        {
            "recover_state",
            "select_one_ready_task",
            "consult_senior_when_policy_requires",
            "implement_smallest_complete_unit",
            "repair_ci",
            "obtain_independent_merge_gate",
            "merge_if_authorized",
            "write_immutable_checkpoint",
            "continue_to_next_ready_task",
        },
        "task_loop",
        errors,
    )

    merge = require_mapping(
        policy.get("autonomous_merge"), "autonomous_merge", errors
    )
    if merge.get("enabled") is not True:
        errors.append("autonomous_merge.enabled must be true")
    if merge.get("ordinary_tasks_only") is not True:
        errors.append("autonomous_merge.ordinary_tasks_only must be true")

    merge_required = require_list(
        merge.get("required"), "autonomous_merge.required", errors
    )
    require_members(
        merge_required,
        {
            "all_required_checks_success_on_exact_head",
            "project_control_validation_success",
            "senior_merge_gate_accepts_without_blockers",
            "no_unresolved_review_threads",
            "expected_head_matches",
            "merge_commit_used_unless_versioned_policy_says_otherwise",
        },
        "autonomous_merge.required",
        errors,
    )

    merge_forbidden = require_list(
        merge.get("forbidden"), "autonomous_merge.forbidden", errors
    )
    require_members(
        merge_forbidden,
        {
            "force_push",
            "silent_requirement_relaxation",
            "merge_with_red_or_missing_required_check",
            "merge_after_head_changed_without_reverification",
            "milestone_transition_without_human_gate",
        },
        "autonomous_merge.forbidden",
        errors,
    )

    consult = require_mapping(
        policy.get("senior_consultation"),
        "senior_consultation",
        errors,
    )
    require_list(
        consult.get("required_for"),
        "senior_consultation.required_for",
        errors,
    )
    require_list(
        consult.get("minimum_packet"),
        "senior_consultation.minimum_packet",
        errors,
    )
    verdicts = require_list(
        consult.get("valid_merge_verdicts"),
        "senior_consultation.valid_merge_verdicts",
        errors,
    )
    require_members(
        verdicts,
        {"accept", "accept_with_non_blocking_risks"},
        "senior_consultation.valid_merge_verdicts",
        errors,
    )

    escalation = require_mapping(
        policy.get("human_escalation"), "human_escalation", errors
    )
    if escalation.get("ask_one_decision_at_a_time") is not True:
        errors.append(
            "human_escalation.ask_one_decision_at_a_time must be true"
        )
    fields = require_list(
        escalation.get("required_fields"),
        "human_escalation.required_fields",
        errors,
    )
    require_members(
        fields,
        {
            "decision_id",
            "question",
            "options",
            "senior_recommendation",
            "impact",
            "default_if_no_response",
            "next_action_after_answer",
            "expected_answer_format",
        },
        "human_escalation.required_fields",
        errors,
    )
    if escalation.get("option_count_min") != 2:
        errors.append("human_escalation.option_count_min must equal 2")
    maximum = escalation.get("option_count_max")
    if not isinstance(maximum, int) or maximum < 2 or maximum > 4:
        errors.append(
            "human_escalation.option_count_max must be an integer from 2 to 4"
        )

    milestone = require_mapping(
        policy.get("milestone_policy"), "milestone_policy", errors
    )
    if milestone.get("tasks_inside_approved_milestone") != "autonomous":
        errors.append("milestone tasks must be autonomous")
    if (
        milestone.get("ordinary_task_merges")
        != "autonomous_when_gate_passes"
    ):
        errors.append(
            "ordinary task merges must be autonomous_when_gate_passes"
        )
    if milestone.get("close_milestone_and_open_next") != "human_required":
        errors.append(
            "milestone close/open transition must remain human_required"
        )

    parallelism = require_mapping(
        policy.get("parallelism"), "parallelism", errors
    )
    if parallelism.get("allowed") is not True:
        errors.append("parallelism.allowed must be true")
    require_list(
        parallelism.get("requirements"), "parallelism.requirements", errors
    )
    require_list(
        parallelism.get("shared_files"), "parallelism.shared_files", errors
    )

    require_list(
        policy.get("evidence_levels"), "evidence_levels", errors
    )

    required_documents = require_list(
        policy.get("required_documents"), "required_documents", errors
    )
    for relative in required_documents:
        if isinstance(relative, str):
            require_file(relative, errors)
        else:
            errors.append("required_documents entries must be strings")

    validation = require_mapping(
        policy.get("validation"), "validation", errors
    )
    script = require_string(
        validation.get("script"), "validation.script", errors
    )
    workflow = require_string(
        validation.get("workflow"), "validation.workflow", errors
    )
    if script:
        require_file(script, errors)
    if workflow:
        require_file(workflow, errors)

    require_text_tokens(
        "AGENTS.md",
        (
            "PROJECT-AUTONOMY.json",
            "L1_AUTONOMOUS",
            "L2_SENIOR_REQUIRED",
            "L3_HUMAN_REQUIRED",
            "L4_SAFETY_STOP",
            "AUTONOMOUS-MERGE.md",
            "HUMAN-ESCALATION.md",
        ),
        errors,
    )
    require_text_tokens(
        "AUTONOMY.md",
        (
            "PROJECT-AUTONOMY.json",
            "MISSION-LIFECYCLE.md",
            "SENIOR-CONSULTATION.md",
            "AUTONOMOUS-MERGE.md",
            "HUMAN-ESCALATION.md",
        ),
        errors,
    )
    require_text_tokens(
        ".github/workflows/project-control.yml",
        (
            "PROJECT-AUTONOMY.json",
            "validate-autonomy-policy.py",
            "Validate autonomous governance",
        ),
        errors,
    )
    require_text_tokens(
        ".github/PULL_REQUEST_TEMPLATE.md",
        (
            "## Autonomy classification",
            "## Senior consultation",
            "## Autonomous merge gate",
            "Expected head:",
        ),
        errors,
    )
    require_text_tokens(
        "doc/sooperlooper/WORKFLOW.md",
        (
            "## Autonomous-by-default mode",
            "## Decision classification",
            "## Continuous task loop",
        ),
        errors,
    )
    require_text_tokens(
        "doc/sooperlooper/DOCUMENTATION-MAP.md",
        (
            "PROJECT-AUTONOMY.json",
            "AUTONOMY.md",
            "AUTONOMY-SCENARIOS.md",
            "AUTONOMY-ADOPTION.md",
        ),
        errors,
    )

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("Autonomy-policy validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    print("Autonomy-policy validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
