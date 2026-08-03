#!/usr/bin/env python3
"""Validate the repository autonomous-agent governance contract."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
POLICY_PATH = ROOT / "PROJECT-AUTONOMY.json"


class PolicyError(RuntimeError):
    pass


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise PolicyError(f"missing required JSON file: {path.relative_to(ROOT)}") from exc
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PolicyError(f"invalid JSON in {path.relative_to(ROOT)}: {exc}") from exc
    if not isinstance(value, dict):
        raise PolicyError(f"{path.relative_to(ROOT)} must contain a JSON object")
    return value


def mapping(value: Any, name: str, errors: list[str]) -> dict[str, Any]:
    if not isinstance(value, dict):
        errors.append(f"{name} must be an object")
        return {}
    return value


def nonempty_list(value: Any, name: str, errors: list[str]) -> list[Any]:
    if not isinstance(value, list) or not value:
        errors.append(f"{name} must be a non-empty list")
        return []
    return value


def require_members(items: list[Any], required: set[str], name: str, errors: list[str]) -> None:
    strings = {item for item in items if isinstance(item, str)}
    missing = required - strings
    if missing:
        errors.append(f"{name} missing: {', '.join(sorted(missing))}")


def require_file(relative: str, errors: list[str]) -> str:
    path = ROOT / relative
    if not path.is_file():
        errors.append(f"missing required autonomy file: {relative}")
        return ""
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        errors.append(f"autonomy file is not UTF-8: {relative}")
        return ""


def require_text(text: str, needles: list[str], name: str, errors: list[str]) -> None:
    for needle in needles:
        if needle not in text:
            errors.append(f"{name} missing required text: {needle}")


def forbid_text(text: str, needles: list[str], name: str, errors: list[str]) -> None:
    lowered = text.lower()
    for needle in needles:
        if needle.lower() in lowered:
            errors.append(f"{name} contains obsolete policy text: {needle}")


def validate() -> list[str]:
    errors: list[str] = []
    try:
        policy = load_json(POLICY_PATH)
    except PolicyError as exc:
        return [str(exc)]

    if policy.get("schema_version") != 2:
        errors.append("PROJECT-AUTONOMY.json schema_version must equal 2")
    if policy.get("policy_id") != "seq66-sl-autonomy-v2":
        errors.append("PROJECT-AUTONOMY.json policy_id must be seq66-sl-autonomy-v2")
    if policy.get("mode") != "autonomous_by_default":
        errors.append("mode must be autonomous_by_default")

    roles = mapping(policy.get("roles"), "roles", errors)
    required_roles = {
        "human_owner",
        "hermes_operator",
        "codex_senior_consult",
        "repository_and_ci",
    }
    missing_roles = required_roles - set(roles)
    if missing_roles:
        errors.append(f"roles missing: {', '.join(sorted(missing_roles))}")

    human_authority = nonempty_list(
        mapping(roles.get("human_owner"), "roles.human_owner", errors).get("authority"),
        "roles.human_owner.authority",
        errors,
    )
    require_members(
        human_authority,
        {
            "product_objectives",
            "subjective_musical_or_visual_choices",
            "phase_transition_only_when_l3_triggered",
        },
        "roles.human_owner.authority",
        errors,
    )
    if "milestone_gate_decisions" in human_authority:
        errors.append("blanket human milestone_gate_decisions authority is obsolete")

    hermes_authority = nonempty_list(
        mapping(roles.get("hermes_operator"), "roles.hermes_operator", errors).get("authority"),
        "roles.hermes_operator.authority",
        errors,
    )
    require_members(
        hermes_authority,
        {
            "merge_ordinary_tasks_when_gate_passes",
            "close_clear_phase_and_open_preapproved_next_phase_when_senior_gate_passes",
            "continue_to_next_ready_task",
        },
        "roles.hermes_operator.authority",
        errors,
    )

    levels = mapping(policy.get("decision_levels"), "decision_levels", errors)
    expected_interrupt = {
        "L1_AUTONOMOUS": False,
        "L2_SENIOR_REQUIRED": False,
        "L3_HUMAN_REQUIRED": True,
        "L4_SAFETY_STOP": True,
    }
    for level, expected in expected_interrupt.items():
        item = mapping(levels.get(level), f"decision_levels.{level}", errors)
        if item.get("human_interrupt") is not expected:
            errors.append(f"decision_levels.{level}.human_interrupt must be {expected}")
        nonempty_list(item.get("examples"), f"decision_levels.{level}.examples", errors)

    l2_examples = nonempty_list(
        mapping(levels.get("L2_SENIOR_REQUIRED"), "L2", errors).get("examples"),
        "decision_levels.L2_SENIOR_REQUIRED.examples",
        errors,
    )
    require_members(
        l2_examples,
        {"clear_phase_close_and_preapproved_next_phase_open"},
        "decision_levels.L2_SENIOR_REQUIRED.examples",
        errors,
    )

    task_loop = nonempty_list(policy.get("task_loop"), "task_loop", errors)
    require_members(
        task_loop,
        {
            "recover_state",
            "consult_senior_when_policy_requires",
            "obtain_independent_merge_gate",
            "merge_if_authorized",
            "write_immutable_checkpoint",
            "continue_to_next_ready_task",
        },
        "task_loop",
        errors,
    )

    merge = mapping(policy.get("autonomous_merge"), "autonomous_merge", errors)
    if merge.get("enabled") is not True:
        errors.append("autonomous_merge.enabled must be true")
    merge_required = nonempty_list(merge.get("required"), "autonomous_merge.required", errors)
    require_members(
        merge_required,
        {
            "all_required_checks_success_on_exact_head",
            "senior_exact_head_gate_accepts_without_blockers",
            "no_unresolved_review_threads",
            "expected_head_matches",
        },
        "autonomous_merge.required",
        errors,
    )
    merge_forbidden = nonempty_list(merge.get("forbidden"), "autonomous_merge.forbidden", errors)
    require_members(
        merge_forbidden,
        {
            "force_push",
            "merge_with_red_or_missing_required_check",
            "phase_transition_with_unresolved_l3_or_l4_trigger",
        },
        "autonomous_merge.forbidden",
        errors,
    )

    senior = mapping(policy.get("senior_consultation"), "senior_consultation", errors)
    require_members(
        nonempty_list(senior.get("required_for"), "senior_consultation.required_for", errors),
        {"ordinary_functional_merge_gate", "phase_gate"},
        "senior_consultation.required_for",
        errors,
    )
    require_members(
        nonempty_list(senior.get("valid_gate_verdicts"), "senior_consultation.valid_gate_verdicts", errors),
        {"accept", "accept_with_non_blocking_risks"},
        "senior_consultation.valid_gate_verdicts",
        errors,
    )

    phase = mapping(policy.get("phase_transition"), "phase_transition", errors)
    if phase.get("default_authority") != "L2_SENIOR_REQUIRED":
        errors.append("phase_transition.default_authority must be L2_SENIOR_REQUIRED")
    if phase.get("result") != "close_current_phase_open_preapproved_next_phase_and_continue":
        errors.append("phase_transition.result must authorize continuing into the preapproved next phase")
    require_members(
        nonempty_list(
            phase.get("autonomous_transition_required"),
            "phase_transition.autonomous_transition_required",
            errors,
        ),
        {
            "current_phase_definition_of_done_satisfied",
            "global_senior_phase_review_accepts_without_blockers",
            "next_phase_already_defined_in_approved_roadmap",
            "no_L3_or_L4_trigger_present",
            "post_transition_checkpoint_published",
        },
        "phase_transition.autonomous_transition_required",
        errors,
    )

    interaction = mapping(policy.get("human_interaction"), "human_interaction", errors)
    for key in (
        "ask_one_decision_at_a_time",
        "prefer_native_hermes_selection_form",
        "selection_form_required_when_supported",
        "free_text_chat_fallback_only_when_form_unavailable_or_question_not_representable",
        "include_other_option_when_safe",
        "other_option_requires_free_text",
    ):
        if interaction.get(key) is not True:
            errors.append(f"human_interaction.{key} must be true")
    if interaction.get("other_option_label") != "Otra opción / Other":
        errors.append("human_interaction.other_option_label must be 'Otra opción / Other'")
    if interaction.get("option_count_min") != 2:
        errors.append("human_interaction.option_count_min must equal 2")
    maximum = interaction.get("option_count_max")
    if not isinstance(maximum, int) or maximum < 2 or maximum > 4:
        errors.append("human_interaction.option_count_max must be 2..4")

    required_documents = nonempty_list(policy.get("required_documents"), "required_documents", errors)
    documents: dict[str, str] = {}
    for relative in required_documents:
        if isinstance(relative, str):
            documents[relative] = require_file(relative, errors)
        else:
            errors.append("required_documents entries must be strings")

    root_agents = require_file("AGENTS.md", errors)
    root_autonomy = documents.get("AUTONOMY.md", "")
    normative = documents.get("doc/sooperlooper/AUTONOMY.md", "")
    lifecycle = documents.get("doc/sooperlooper/MISSION-LIFECYCLE.md", "")
    escalation = documents.get("doc/sooperlooper/HUMAN-ESCALATION.md", "")
    merge_doc = documents.get("doc/sooperlooper/AUTONOMOUS-MERGE.md", "")
    human_template = documents.get("doc/sooperlooper/templates/HUMAN-DECISION.md", "")

    for name, text in (
        ("AGENTS.md", root_agents),
        ("AUTONOMY.md", root_autonomy),
        ("doc/sooperlooper/AUTONOMY.md", normative),
        ("MISSION-LIFECYCLE.md", lifecycle),
        ("AUTONOMOUS-MERGE.md", merge_doc),
    ):
        require_text(
            text,
            [
                "phase boundary is not",
                "senior",
                "expected-head",
            ],
            name,
            errors,
        )

    require_text(
        escalation,
        [
            "native interactive selection forms",
            "Otra opción / Other",
            "Plain chat",
            "A phase transition is L3 only",
        ],
        "HUMAN-ESCALATION.md",
        errors,
    )
    require_text(
        human_template,
        [
            "Hermes selection form",
            "Otra opción / Other",
            "Senior recommendation",
        ],
        "templates/HUMAN-DECISION.md",
        errors,
    )

    obsolete = [
        "closing a milestone and opening the next remains a short explicit human decision",
        "the human answer is the only normal interruption between milestones",
        "final close/open transition requires an explicit human answer",
        "closing one milestone and opening the next",
    ]
    for name, text in (
        ("AGENTS.md", root_agents),
        ("AUTONOMY.md", root_autonomy),
        ("doc/sooperlooper/AUTONOMY.md", normative),
        ("MISSION-LIFECYCLE.md", lifecycle),
        ("HUMAN-ESCALATION.md", escalation),
        ("AUTONOMOUS-MERGE.md", merge_doc),
    ):
        forbid_text(text, obsolete, name, errors)

    workflow = require_file(".github/workflows/project-control.yml", errors)
    require_text(
        workflow,
        ["PROJECT-AUTONOMY.json", "validate-autonomy-policy.py"],
        ".github/workflows/project-control.yml",
        errors,
    )

    pr_template = require_file(".github/PULL_REQUEST_TEMPLATE.md", errors)
    require_text(
        pr_template,
        ["## Autonomy classification", "## Autonomous merge and phase gate"],
        ".github/PULL_REQUEST_TEMPLATE.md",
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
