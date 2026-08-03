#!/usr/bin/env python3
"""Validate the autonomous-agent governance contract."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
POLICY_PATH = ROOT / "PROJECT-AUTONOMY.json"


class ValidationError(RuntimeError):
    pass


def read_text(relative: str, errors: list[str]) -> str:
    path = ROOT / relative
    if not path.is_file():
        errors.append(f"missing required file: {relative}")
        return ""
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        errors.append(f"file is not UTF-8: {relative}")
        return ""


def load_policy(errors: list[str]) -> dict[str, Any]:
    try:
        value = json.loads(POLICY_PATH.read_text(encoding="utf-8"))
    except FileNotFoundError:
        errors.append("missing PROJECT-AUTONOMY.json")
        return {}
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        errors.append(f"invalid PROJECT-AUTONOMY.json: {exc}")
        return {}
    if not isinstance(value, dict):
        errors.append("PROJECT-AUTONOMY.json must contain an object")
        return {}
    return value


def obj(value: Any, name: str, errors: list[str]) -> dict[str, Any]:
    if not isinstance(value, dict):
        errors.append(f"{name} must be an object")
        return {}
    return value


def items(value: Any, name: str, errors: list[str]) -> list[Any]:
    if not isinstance(value, list) or not value:
        errors.append(f"{name} must be a non-empty list")
        return []
    return value


def require_members(values: list[Any], required: set[str], name: str, errors: list[str]) -> None:
    actual = {value for value in values if isinstance(value, str)}
    missing = required - actual
    if missing:
        errors.append(f"{name} missing: {', '.join(sorted(missing))}")


def require_all(text: str, fragments: list[str], name: str, errors: list[str]) -> None:
    lowered = text.lower()
    for fragment in fragments:
        if fragment.lower() not in lowered:
            errors.append(f"{name} missing concept/text: {fragment}")


def forbid_any(text: str, fragments: list[str], name: str, errors: list[str]) -> None:
    lowered = text.lower()
    for fragment in fragments:
        if fragment.lower() in lowered:
            errors.append(f"{name} contains obsolete rule: {fragment}")


def validate() -> list[str]:
    errors: list[str] = []
    policy = load_policy(errors)
    if not policy:
        return errors

    if policy.get("schema_version") != 2:
        errors.append("schema_version must equal 2")
    if policy.get("policy_id") != "seq66-sl-autonomy-v2":
        errors.append("policy_id must equal seq66-sl-autonomy-v2")
    if policy.get("mode") != "autonomous_by_default":
        errors.append("mode must equal autonomous_by_default")

    roles = obj(policy.get("roles"), "roles", errors)
    required_roles = {
        "human_owner",
        "hermes_operator",
        "codex_senior_consult",
        "repository_and_ci",
    }
    missing_roles = required_roles - set(roles)
    if missing_roles:
        errors.append(f"roles missing: {', '.join(sorted(missing_roles))}")

    human = obj(roles.get("human_owner"), "roles.human_owner", errors)
    human_authority = items(human.get("authority"), "roles.human_owner.authority", errors)
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
        errors.append("blanket human milestone gate authority is obsolete")
    if human.get("normal_interaction") != "answer_bounded_selection_forms":
        errors.append("human normal_interaction must use bounded selection forms")

    hermes = obj(roles.get("hermes_operator"), "roles.hermes_operator", errors)
    require_members(
        items(hermes.get("authority"), "roles.hermes_operator.authority", errors),
        {
            "merge_ordinary_tasks_when_gate_passes",
            "close_clear_phase_and_open_preapproved_next_phase_when_senior_gate_passes",
            "continue_to_next_ready_task",
        },
        "roles.hermes_operator.authority",
        errors,
    )

    levels = obj(policy.get("decision_levels"), "decision_levels", errors)
    expected = {
        "L1_AUTONOMOUS": False,
        "L2_SENIOR_REQUIRED": False,
        "L3_HUMAN_REQUIRED": True,
        "L4_SAFETY_STOP": True,
    }
    for level, interrupt in expected.items():
        entry = obj(levels.get(level), f"decision_levels.{level}", errors)
        if entry.get("human_interrupt") is not interrupt:
            errors.append(f"{level}.human_interrupt must be {interrupt}")
        items(entry.get("examples"), f"{level}.examples", errors)

    require_members(
        items(
            obj(levels.get("L2_SENIOR_REQUIRED"), "L2", errors).get("examples"),
            "L2.examples",
            errors,
        ),
        {"clear_phase_close_and_preapproved_next_phase_open"},
        "L2.examples",
        errors,
    )

    merge = obj(policy.get("autonomous_merge"), "autonomous_merge", errors)
    if merge.get("enabled") is not True:
        errors.append("autonomous_merge.enabled must be true")
    require_members(
        items(merge.get("required"), "autonomous_merge.required", errors),
        {
            "all_required_checks_success_on_exact_head",
            "senior_exact_head_gate_accepts_without_blockers",
            "no_unresolved_review_threads",
            "expected_head_matches",
        },
        "autonomous_merge.required",
        errors,
    )
    require_members(
        items(merge.get("forbidden"), "autonomous_merge.forbidden", errors),
        {
            "force_push",
            "merge_with_red_or_missing_required_check",
            "phase_transition_with_unresolved_l3_or_l4_trigger",
        },
        "autonomous_merge.forbidden",
        errors,
    )

    senior = obj(policy.get("senior_consultation"), "senior_consultation", errors)
    require_members(
        items(senior.get("required_for"), "senior_consultation.required_for", errors),
        {"ordinary_functional_merge_gate", "phase_gate"},
        "senior_consultation.required_for",
        errors,
    )
    require_members(
        items(senior.get("valid_gate_verdicts"), "senior_consultation.valid_gate_verdicts", errors),
        {"accept", "accept_with_non_blocking_risks"},
        "senior_consultation.valid_gate_verdicts",
        errors,
    )

    phase = obj(policy.get("phase_transition"), "phase_transition", errors)
    if phase.get("default_authority") != "L2_SENIOR_REQUIRED":
        errors.append("phase transition default authority must be L2_SENIOR_REQUIRED")
    if phase.get("result") != "close_current_phase_open_preapproved_next_phase_and_continue":
        errors.append("phase transition result must continue into preapproved next phase")
    require_members(
        items(
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

    interaction = obj(policy.get("human_interaction"), "human_interaction", errors)
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
        errors.append("other option label must be 'Otra opción / Other'")
    if interaction.get("option_count_min") != 2:
        errors.append("option_count_min must equal 2")
    maximum = interaction.get("option_count_max")
    if not isinstance(maximum, int) or not 2 <= maximum <= 4:
        errors.append("option_count_max must be an integer from 2 to 4")

    required_documents = items(policy.get("required_documents"), "required_documents", errors)
    documents: dict[str, str] = {}
    for relative in required_documents:
        if isinstance(relative, str):
            documents[relative] = read_text(relative, errors)
        else:
            errors.append("required_documents entries must be strings")

    agents = read_text("AGENTS.md", errors)
    root_autonomy = documents.get("AUTONOMY.md", "")
    normative = documents.get("doc/sooperlooper/AUTONOMY.md", "")
    lifecycle = documents.get("doc/sooperlooper/MISSION-LIFECYCLE.md", "")
    senior_doc = documents.get("doc/sooperlooper/SENIOR-CONSULTATION.md", "")
    merge_doc = documents.get("doc/sooperlooper/AUTONOMOUS-MERGE.md", "")
    escalation = documents.get("doc/sooperlooper/HUMAN-ESCALATION.md", "")
    scenarios = documents.get("doc/sooperlooper/AUTONOMY-SCENARIOS.md", "")
    human_template = documents.get("doc/sooperlooper/templates/HUMAN-DECISION.md", "")

    for name, text in (
        ("AGENTS.md", agents),
        ("AUTONOMY.md", root_autonomy),
        ("doc/sooperlooper/AUTONOMY.md", normative),
        ("MISSION-LIFECYCLE.md", lifecycle),
        ("SENIOR-CONSULTATION.md", senior_doc),
        ("AUTONOMOUS-MERGE.md", merge_doc),
    ):
        require_all(text, ["phase", "senior", "L3", "expected-head"], name, errors)

    require_all(
        escalation,
        [
            "native interactive selection forms",
            "Otra opción / Other",
            "Plain chat",
            "phase transition is L3 only",
        ],
        "HUMAN-ESCALATION.md",
        errors,
    )
    require_all(
        human_template,
        ["Hermes selection form", "Otra opción / Other", "Senior recommendation"],
        "templates/HUMAN-DECISION.md",
        errors,
    )
    require_all(
        scenarios,
        ["clear phase complete", "autonomous transition", "Otra opción / Other"],
        "AUTONOMY-SCENARIOS.md",
        errors,
    )

    obsolete = [
        "milestone transitions still require a human decision",
        "final close/open transition requires an explicit human answer",
        "the human answer is the only normal interruption between milestones",
        "does not replace the explicit human decision to close one milestone",
    ]
    for name, text in (
        ("AGENTS.md", agents),
        ("AUTONOMY.md", root_autonomy),
        ("doc/sooperlooper/AUTONOMY.md", normative),
        ("MISSION-LIFECYCLE.md", lifecycle),
        ("SENIOR-CONSULTATION.md", senior_doc),
        ("AUTONOMOUS-MERGE.md", merge_doc),
        ("HUMAN-ESCALATION.md", escalation),
    ):
        forbid_any(text, obsolete, name, errors)

    workflow = read_text(".github/workflows/project-control.yml", errors)
    require_all(workflow, ["PROJECT-AUTONOMY.json", "validate-autonomy-policy.py"], "workflow", errors)

    pr_template = read_text(".github/PULL_REQUEST_TEMPLATE.md", errors)
    require_all(
        pr_template,
        [
            "## Autonomy classification",
            "## Autonomous merge and phase gate",
            "Otra opción / Other",
        ],
        "pull request template",
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
