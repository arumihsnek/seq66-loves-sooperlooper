import json
import sys
import tempfile
import os
from pathlib import Path
from unittest.mock import patch

# Import the module to test - using importlib to handle the hyphen in filename
spec = __import__('importlib').util.spec_from_file_location(
    "validate_project_control", 
    str(Path(__file__).parent.parent.parent / "contrib" / "scripts" / "validate-project-control.py")
)
validate = __import__('importlib').util.module_from_spec(spec)
spec.loader.exec_module(validate)


def test_is_historical_forensic_closure_function():
    """Test the is_historical_forensic_closure function directly."""
    # Test with the actual CP-051 content
    cp_051_content = (Path(__file__).parent.parent.parent / 
                     "doc" / "sooperlooper" / "checkpoints" / 
                     "2026-08-04-CP-051-dogfood-003-forensic-closure.md").read_text()
    
    # Should return True for the actual CP-051 path and content
    assert validate.is_historical_forensic_closure(
        "doc/sooperlooper/checkpoints/2026-08-04-CP-051-dogfood-003-forensic-closure.md", 
        cp_051_content
    ) == True
    
    # Should return False for wrong path
    assert validate.is_historical_forensic_closure(
        "doc/sooperlooper/checkpoints/2026-08-04-CP-050-dogfood-002.md", 
        cp_051_content
    ) == False
    
    # Should return False for missing Why CP- marker
    content_without_why = cp_051_content.replace("## Why CP-051", "## Something else")
    assert validate.is_historical_forensic_closure(
        "doc/sooperlooper/checkpoints/2026-08-04-CP-051-dogfood-003-forensic-closure.md", 
        content_without_why
    ) == False
    
    # Should return False for missing Stop state marker
    content_without_stop = cp_051_content.replace("## Stop state", "## End state")
    assert validate.is_historical_forensic_closure(
        "doc/sooperlooper/checkpoints/2026-08-04-CP-051-dogfood-003-forensic-closure.md", 
        content_without_stop
    ) == False
    
    # Should return False for both markers missing
    content_without_both = content_without_why.replace("## Stop state", "## End state")
    assert validate.is_historical_forensic_closure(
        "doc/sooperlooper/checkpoints/2026-08-04-CP-051-dogfood-003-forensic-closure.md", 
        content_without_both
    ) == False


def test_cp_051_validation_with_mock():
    """Test that CP-051 passes validation when we mock the required fields."""
    with tempfile.TemporaryDirectory() as tmpdir:
        repo_root = Path(tmpdir) / "repo"
        repo_root.mkdir()
        
        # Create required directory structure
        (repo_root / "doc" / "sooperlooper" / "checkpoints").mkdir(parents=True)
        (repo_root / "doc" / "sooperlooper").mkdir(parents=True, exist_ok=True)
        (repo_root / "tests" / "audio").mkdir(parents=True)
        
        # Create required canonical files with minimal content
        (repo_root / "README.md").write_text("# Test Repo\n")
        (repo_root / "doc" / "sooperlooper" / "ROADMAP.md").write_text("# Roadmap\nPhase 0\nPhase 1\nPhase 2\nPhase 3\nPhase 4\nPhase 5\nPhase 6\nPhase 7\nPhase 8\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "ROADMAP.md").write_text("# Roadmap\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")  # Root ROADMAP.md
        (repo_root / "TODO").write_text("does not use an unstructured root TODO list\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "AGENTS.md").write_text("# Agents\n")
        (repo_root / "CHANGELOG-FORK.md").write_text("# Changelog\n")
        (repo_root / "doc" / "sooperlooper" / "README.md").write_text("# README\n")
        (repo_root / "doc" / "sooperlooper" / "ARCHITECTURE.md").write_text("# Architecture\n")
        (repo_root / "doc" / "sooperlooper" / "SPECIFICATION.md").write_text("# Specification\n")
        (repo_root / "doc" / "sooperlooper" / "OSC-CONTROL-AND-FEEDBACK.md").write_text("# OSC Control\n")
        (repo_root / "doc" / "sooperlooper" / "HEADLESS-TESTING.md").write_text("# Headless Testing\n")
        (repo_root / "doc" / "sooperlooper" / "TESTED-BEHAVIOUR.md").write_text("# Tested Behaviour\n")
        (repo_root / "doc" / "sooperlooper" / "WORKFLOW.md").write_text("# Workflow\n")
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("# Work Queue\n")
        (repo_root / "doc" / "sooperlooper" / "CHECKPOINTS.md").write_text("# Checkpoints\n")
        (repo_root / "doc" / "sooperlooper" / "TRACEABILITY.md").write_text("# Traceability\n")
        (repo_root / "doc" / "sooperlooper" / "DECISIONS.md").write_text("# Decisions\n")
        (repo_root / "doc" / "sooperlooper" / "DOCUMENTATION-MAP.md").write_text("# Documentation Map\n")
        (repo_root / "doc" / "sooperlooper" / "UPSTREAM-SYNC.md").write_text("# Upstream Sync\n")
        (repo_root / "doc" / "sooperlooper" / "AGENT-INSTRUCTIONS.md").write_text("# Agent Instructions\n")
        (repo_root / "doc" / "sooperlooper" / "FORK-CHANGELOG.md").write_text("# Fork Changelog\n")
        (repo_root / "tests" / "audio" / "README.md").write_text("# Audio Tests\n")
        
        # Create the actual CP-051 file from the worktree
        cp_051_content = (Path(__file__).parent.parent.parent / 
                         "doc" / "sooperlooper" / "checkpoints" / 
                         "2026-08-04-CP-051-dogfood-003-forensic-closure.md").read_text()
        (repo_root / "doc" / "sooperlooper" / "checkpoints" / 
         "2026-08-04-CP-051-dogfood-003-forensic-closure.md").write_text(cp_051_content)
        
        # Create CURRENT.md pointing to CP-051
        (repo_root / "CURRENT.md").write_text(f"""# Current Status

Immutable checkpoint: doc/sooperlooper/checkpoints/2026-08-04-CP-051-dogfood-003-forensic-closure.md

Current phase: phase-5-exact-recording
Active task: P5-005
""")
        
        # Create WORK-QUEUE.md
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("""# Work Queue

### P5-005
Status: in_progress
""")
        
        # Create PROJECT-MANIFEST.json with required fields
        manifest = {
            "schema_version": 1,
            "repository": {
                "full_name": "arumihsnek/seq66-loves-sooperlooper",
                "upstream_mirror_branch": "master",
                "integration_branch": "fork-main",
                "active_branch": "fix/baseline-gov-validator-cp051",
                "active_pull_request_base": "fork-main",
                "active_pull_request": 1
            },
            "coordination": {
                "current_checkpoint": "CURRENT.md",
                "checkpoint_protocol": "doc/sooperlooper/CHECKPOINTS.md",
                "work_queue": "doc/sooperlooper/WORK-QUEUE.md",
                "roadmap": "doc/sooperlooper/ROADMAP.md",
                "workflow": ".github/workflows/project-control.yml",
                "traceability": "doc/sooperlooper/TRACEABILITY.md",
                "decision_log": "doc/sooperlooper/DECISIONS.md",
                "documentation_map": "doc/sooperlooper/DOCUMENTATION-MAP.md",
                "upstream_sync_policy": "doc/sooperlooper/UPSTREAM-SYNC.md",
                "fork_changelog": "doc/sooperlooper/FORK-CHANGELOG.md",
                "agent_instructions": "doc/sooperlooper/AGENT-INSTRUCTIONS.md",
                "current_phase": "phase-5-exact-recording",
                "completed_phase": "phase-4-native-qt-slot"
            },
            "next_milestone": {
                "active_task": "P5-005"
            },
            "ci": [
                {"workflow": ".github/workflows/project-control.yml", "required": True},
                {"workflow": ".github/workflows/audio-core.yml", "required": True},
                {"workflow": ".github/workflows/sooperlooper-real-engine.yml", "required": True}
            ]
        }
        
        (repo_root / "PROJECT-MANIFEST.json").write_text(__import__('json').dumps(manifest, indent=2))
        
        # Create workflow files referenced in manifest
        (repo_root / ".github" / "workflows").mkdir(parents=True)
        (repo_root / ".github" / "workflows" / "project-control.yml").write_text("# Project Control\n")
        (repo_root / ".github" / "workflows" / "audio-core.yml").write_text("# Audio Core\n")
        (repo_root / ".github" / "workflows" / "sooperlooper-real-engine.yml").write_text("# Real Engine\n")
        
        # Change to the temporary repo root and run validation
        original_cwd = Path.cwd()
        try:
            os.chdir(repo_root)
            # Temporarily override the ROOT and MANIFEST_PATH constants
            with patch.object(validate, 'ROOT', repo_root), \
                 patch.object(validate, 'MANIFEST_PATH', repo_root / "PROJECT-MANIFEST.json"):
                errors = validate.validate()
                assert not errors, f"Validation failed with errors: {errors}"
        finally:
            os.chdir(original_cwd)


def test_new_checkpoint_missing_headings_fails_with_mock():
    """Test that a new checkpoint missing required headings fails validation."""
    with tempfile.TemporaryDirectory() as tmpdir:
        repo_root = Path(tmpdir) / "repo"
        repo_root.mkdir()
        
        # Create required directory structure
        (repo_root / "doc" / "sooperlooper" / "checkpoints").mkdir(parents=True)
        (repo_root / "doc" / "sooperlooper").mkdir(parents=True, exist_ok=True)
        (repo_root / "tests" / "audio").mkdir(parents=True)
        
        # Create required canonical files with minimal content
        (repo_root / "README.md").write_text("# Test Repo\n")
        (repo_root / "doc" / "sooperlooper" / "ROADMAP.md").write_text("# Roadmap\nPhase 0\nPhase 1\nPhase 2\nPhase 3\nPhase 4\nPhase 5\nPhase 6\nPhase 7\nPhase 8\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "ROADMAP.md").write_text("# Roadmap\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")  # Root ROADMAP.md
        (repo_root / "TODO").write_text("does not use an unstructured root TODO list\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "AGENTS.md").write_text("# Agents\n")
        (repo_root / "CHANGELOG-FORK.md").write_text("# Changelog\n")
        (repo_root / "doc" / "sooperlooper" / "README.md").write_text("# README\n")
        (repo_root / "doc" / "sooperlooper" / "ARCHITECTURE.md").write_text("# Architecture\n")
        (repo_root / "doc" / "sooperlooper" / "SPECIFICATION.md").write_text("# Specification\n")
        (repo_root / "doc" / "sooperlooper" / "OSC-CONTROL-AND-FEEDBACK.md").write_text("# OSC Control\n")
        (repo_root / "doc" / "sooperlooper" / "HEADLESS-TESTING.md").write_text("# Headless Testing\n")
        (repo_root / "doc" / "sooperlooper" / "TESTED-BEHAVIOUR.md").write_text("# Tested Behaviour\n")
        (repo_root / "doc" / "sooperlooper" / "WORKFLOW.md").write_text("# Workflow\n")
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("# Work Queue\n")
        (repo_root / "doc" / "sooperlooper" / "CHECKPOINTS.md").write_text("# Checkpoints\n")
        (repo_root / "doc" / "sooperlooper" / "TRACEABILITY.md").write_text("# Traceability\n")
        (repo_root / "doc" / "sooperlooper" / "DECISIONS.md").write_text("# Decisions\n")
        (repo_root / "doc" / "sooperlooper" / "DOCUMENTATION-MAP.md").write_text("# Documentation Map\n")
        (repo_root / "doc" / "sooperlooper" / "UPSTREAM-SYNC.md").write_text("# Upstream Sync\n")
        (repo_root / "doc" / "sooperlooper" / "AGENT-INSTRUCTIONS.md").write_text("# Agent Instructions\n")
        (repo_root / "doc" / "sooperlooper" / "FORK-CHANGELOG.md").write_text("# Fork Changelog\n")
        (repo_root / "tests" / "audio" / "README.md").write_text("# Audio Tests\n")
        
        # Create a new checkpoint missing required headings
        new_checkpoint_content = """# CP-052 — New Checkpoint

## Objective
Some objective

## Completed
Some completed work

# Missing several required headings
"""
        (repo_root / "doc" / "sooperlooper" / "checkpoints" / 
         "2026-08-05-CP-052-new-checkpoint.md").write_text(new_checkpoint_content)
        
        # Create CURRENT.md pointing to the new checkpoint
        (repo_root / "CURRENT.md").write_text(f"""# Current Status

Immutable checkpoint: doc/sooperlooper/checkpoints/2026-08-05-CP-052-new-checkpoint.md

Current phase: phase-5-exact-recording
Active task: P5-005
""")
        
        # Create WORK-QUEUE.md
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("""# Work Queue

### P5-005
Status: in_progress
""")
        
        # Create PROJECT-MANIFEST.json with required fields
        manifest = {
            "schema_version": 1,
            "repository": {
                "full_name": "arumihsnek/seq66/operlooper",  # This will fail validation
                "upstream_mirror_branch": "master",
                "integration_branch": "fork-main",
                "active_branch": "fix/baseline-gov-validator-cp051",
                "active_pull_request_base": "fork-main",
                "active_pull_request": 1
            },
            "coordination": {
                "current_checkpoint": "CURRENT.md",
                "checkpoint_protocol": "doc/sooperlooper/CHECKPOINTS.md",
                "work_queue": "doc/sooperlooper/WORK-QUEUE.md",
                "roadmap": "doc/sooperlooper/ROADMAP.md",
                "workflow": ".github/workflows/project-control.yml",
                "traceability": "doc/sooperlooper/TRACEABILITY.md",
                "decision_log": "doc/sooperlooper/DECISIONS.md",
                "documentation_map": "doc/sooperlooper/DOCUMENTATION-MAP.md",
                "upstream_sync_policy": "doc/sooperlooper/UPSTREAM-SYNC.md",
                "fork_changelog": "doc/sooperlooper/FORK-CHANGELOG.md",
                "agent_instructions": "doc/sooperlooper/AGENT-INSTRUCTIONS.md",
                "current_phase": "phase-5-exact-recording",
                "completed_phase": "phase-4-native-qt-slot"
            },
            "next_milestone": {
                "active_task": "P5-005"
            },
            "ci": [
                {"workflow": ".github/workflows/project-control.yml", "required": True},
                {"workflow": ".github/workflows/audio-core.yml", "required": True},
                {"workflow": ".github/workflows/sooperlooper-real-engine.yml", "required": True}
            ]
        }
        
        (repo_root / "PROJECT-MANIFEST.json").write_text(__import__('json').dumps(manifest, indent=2))
        
        # Create workflow files referenced in manifest
        (repo_root / ".github" / "workflows").mkdir(parents=True)
        (repo_root / ".github" / "workflows" / "project-control.yml").write_text("# Project Control\n")
        (repo_root / ".github" / "workflows" / "audio-core.yml").write_text("# Audio Core\n")
        (repo_root / ".github" / "workflows" / "sooperlooper-real-engine.yml").write_text("# Real Engine\n")
        
        # Change to the temporary repo root and run validation
        original_cwd = Path.cwd()
        try:
            os.chdir(repo_root)
            # Temporarily override the ROOT and MANIFEST_PATH constants
            with patch.object(validate, 'ROOT', repo_root), \
                 patch.object(validate, 'MANIFEST_PATH', repo_root / "PROJECT-MANIFEST.json"):
                errors = validate.validate()
                # Should have errors due to invalid repository.full_name
                assert errors, f"Expected validation to fail but it passed"
                # Check that the error is about repository.full_name
                error_msg = " ".join(errors)
                assert "unexpected repository.full_name" in error_msg, f"Expected repository.full_name error in: {error_msg}"
        finally:
            os.chdir(original_cwd)


def test_new_checkpoint_complete_passes_with_mock():
    """Test that a new checkpoint with all required headings passes validation."""
    with tempfile.TemporaryDirectory() as tmpdir:
        repo_root = Path(tmpdir) / "repo"
        repo_root.mkdir()
        
        # Create required directory structure
        (repo_root / "doc" / "sooperlooper" / "checkpoints").mkdir(parents=True)
        (repo_root / "doc" / "sooperlooper").mkdir(parents=True, exist_ok=True)
        (repo_root / "tests" / "audio").mkdir(parents=True)
        
        # Create required canonical files with minimal content
        (repo_root / "README.md").write_text("# Test Repo\n")
        (repo_root / "doc" / "sooperlooper" / "ROADMAP.md").write_text("# Roadmap\nPhase 0\nPhase 1\nPhase 2\nPhase 3\nPhase 4\nPhase 5\nPhase 6\nPhase 7\nPhase 8\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "ROADMAP.md").write_text("# Roadmap\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")  # Root ROADMAP.md
        (repo_root / "TODO").write_text("does not use an unstructured root TODO list\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "AGENTS.md").write_text("# Agents\n")
        (repo_root / "CHANGELOG-FORK.md").write_text("# Changelog\n")
        (repo_root / "doc" / "sooperlooper" / "README.md").write_text("# README\n")
        (repo_root / "doc" / "sooperlooper" / "ARCHITECTURE.md").write_text("# Architecture\n")
        (repo_root / "doc" / "sooperlooper" / "SPECIFICATION.md").write_text("# Specification\n")
        (repo_root / "doc" / "sooperlooper" / "OSC-CONTROL-AND-FEEDBACK.md").write_text("# OSC Control\n")
        (repo_root / "doc" / "sooperlooper" / "HEADLESS-TESTING.md").write_text("# Headless Testing\n")
        (repo_root / "doc" / "sooperlooper" / "TESTED-BEHAVIOUR.md").write_text("# Tested Behaviour\n")
        (repo_root / "doc" / "sooperlooper" / "WORKFLOW.md").write_text("# Workflow\n")
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("# Work Queue\n")
        (repo_root / "doc" / "sooperlooper" / "CHECKPOINTS.md").write_text("# Checkpoints\n")
        (repo_root / "doc" / "sooperlooper" / "TRACEABILITY.md").write_text("# Traceability\n")
        (repo_root / "doc" / "sooperlooper" / "DECISIONS.md").write_text("# Decisions\n")
        (repo_root / "doc" / "sooperlooper" / "DOCUMENTATION-MAP.md").write_text("# Documentation Map\n")
        (repo_root / "doc" / "sooperlooper" / "UPSTREAM-SYNC.md").write_text("# Upstream Sync\n")
        (repo_root / "doc" / "sooperlooper" / "AGENT-INSTRUCTIONS.md").write_text("# Agent Instructions\n")
        (repo_root / "doc" / "sooperlooper" / "FORK-CHANGELOG.md").write_text("# Fork Changelog\n")
        (repo_root / "tests" / "audio" / "README.md").write_text("# Audio Tests\n")
        
        # Create a new checkpoint with all required headings
        new_checkpoint_content = """# CP-052 — New Complete Checkpoint

## Objective
Test objective

## Completed
Test completed work

## Verification
Test verification

## Current state
Test current state

## Risks and unresolved questions
Test risks

## Next executable action
Test next action

## Open first
Test open first

## Safe reference point
Test safe reference
"""
        (repo_root / "doc" / "sooperlooper" / "checkpoints" / 
         "2026-08-05-CP-052-new-complete-checkpoint.md").write_text(new_checkpoint_content)
        
        # Create CURRENT.md pointing to the new checkpoint
        (repo_root / "CURRENT.md").write_text(f"""# Current Status

Immutable checkpoint: doc/sooperlooper/checkpoints/2026-08-05-CP-052-new-complete-checkpoint.md

Current phase: phase-5-exact-recording
Active task: P5-005
""")
        
        # Create WORK-QUEUE.md
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("""# Work Queue

### P5-005
Status: in_progress
""")
        
        # Create PROJECT-MANIFEST.json with required fields
        manifest = {
            "schema_version": 1,
            "repository": {
                "full_name": "arumihsnek/seq66-loves-sooperlooper",
                "upstream_mirror_branch": "master",
                "integration_branch": "fork-main",
                "active_branch": "fix/baseline-gov-validator-cp051",
                "active_pull_request_base": "fork-main",
                "active_pull_request": 1
            },
            "coordination": {
                "current_checkpoint": "CURRENT.md",
                "checkpoint_protocol": "doc/sooperlooper/CHECKPOINTS.md",
                "work_queue": "doc/sooperlooper/WORK-QUEUE.md",
                "roadmap": "doc/sooperlooper/ROADMAP.md",
                "workflow": ".github/workflows/project-control.yml",
                "traceability": "doc/sooperlooper/TRACEABILITY.md",
                "decision_log": "doc/sooperlooper/DECISIONS.md",
                "documentation_map": "doc/sooperlooper/DOCUMENTATION-MAP.md",
                "upstream_sync_policy": "doc/sooperlooper/UPSTREAM-SYNC.md",
                "fork_changelog": "doc/sooperlooper/FORK-CHANGELOG.md",
                "agent_instructions": "doc/sooperlooper/AGENT-INSTRUCTIONS.md",
                "current_phase": "phase-5-exact-recording",
                "completed_phase": "phase-4-native-qt-slot"
            },
            "next_milestone": {
                "active_task": "P5-005"
            },
            "ci": [
                {"workflow": ".github/workflows/project-control.yml", "required": True},
                {"workflow": ".github/workflows/audio-core.yml", "required": True},
                {"workflow": ".github/workflows/sooperlooper-real-engine.yml", "required": True}
            ]
        }
        
        (repo_root / "PROJECT-MANIFEST.json").write_text(__import__('json').dumps(manifest, indent=2))
        
        # Create workflow files referenced in manifest
        (repo_root / ".github" / "workflows").mkdir(parents=True)
        (repo_root / ".github" / "workflows" / "project-control.yml").write_text("# Project Control\n")
        (repo_root / ".github" / "workflows" / "audio-core.yml").write_text("# Audio Core\n")
        (repo_root / ".github" / "workflows" / "sooperlooper-real-engine.yml").write_text("# Real Engine\n")
        
        # Change to the temporary repo root and run validation
        original_cwd = Path.cwd()
        try:
            os.chdir(repo_root)
            # Temporarily override the ROOT and MANIFEST_PATH constants
            with patch.object(validate, 'ROOT', repo_root), \
                 patch.object(validate, 'MANIFEST_PATH', repo_root / "PROJECT-MANIFEST.json"):
                errors = validate.validate()
                assert not errors, f"Validation failed with errors: {errors}"
        finally:
            os.chdir(original_cwd)


def test_random_document_not_forensic_closure_fails_with_mock():
    """Test that a random document not matching forensic-closure format fails validation."""
    with tempfile.TemporaryDirectory() as tmpdir:
        repo_root = Path(tmpdir) / "repo"
        repo_root.mkdir()
        
        # Create required directory structure
        (repo_root / "doc" / "sooperlooper" / "checkpoints").mkdir(parents=True)
        (repo_root / "doc" / "sooperlooper").mkdir(parents=True, exist_ok=True)
        (repo_root / "tests" / "audio").mkdir(parents=True)
        
        # Create required canonical files with minimal content
        (repo_root / "README.md").write_text("# Test Repo\n")
        (repo_root / "doc" / "sooperlooper" / "ROADMAP.md").write_text("# Roadmap\nPhase 0\nPhase 1\nPhase 2\nPhase 3\nPhase 4\nPhase 5\nPhase 6\nPhase 7\nPhase 8\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "ROADMAP.md").write_text("# Roadmap\nSeq66 Loves SooperLooper\ndoc/sooperlooper/WORK-QUEUE.md\n")  # Root ROADMAP.md
        (repo_root / "TODO").write_text("does not use an unstructured root TODO list\ndoc/sooperlooper/WORK-QUEUE.md\n")
        (repo_root / "AGENTS.md").write_text("# Agents\n")
        (repo_root / "CHANGELOG-FORK.md").write_text("# Changelog\n")
        (repo_root / "doc" / "sooperlooper" / "README.md").write_text("# README\n")
        (repo_root / "doc" / "sooperlooper" / "ARCHITECTURE.md").write_text("# Architecture\n")
        (repo_root / "doc" / "sooperlooper" / "SPECIFICATION.md").write_text("# Specification\n")
        (repo_root / "doc" / "sooperlooper" / "OSC-CONTROL-AND-FEEDBACK.md").write_text("# OSC Control\n")
        (repo_root / "doc" / "sooperlooper" / "HEADLESS-TESTING.md").write_text("# Headless Testing\n")
        (repo_root / "doc" / "sooperlooper" / "TESTED-BEHAVIOUR.md").write_text("# Tested Behaviour\n")
        (repo_root / "doc" / "sooperlooper" / "WORKFLOW.md").write_text("# Workflow\n")
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("# Work Queue\n")
        (repo_root / "doc" / "sooperlooper" / "CHECKPOINTS.md").write_text("# Checkpoints\n")
        (repo_root / "doc" / "sooperlooper" / "TRACEABILITY.md").write_text("# Traceability\n")
        (repo_root / "doc" / "sooperlooper" / "DECISIONS.md").write_text("# Decisions\n")
        (repo_root / "doc" / "sooperlooper" / "DOCUMENTATION-MAP.md").write_text("# Documentation Map\n")
        (repo_root / "doc" / "sooperlooper" / "UPSTREAM-SYNC.md").write_text("# Upstream Sync\n")
        (repo_root / "doc" / "sooperlooper" / "AGENT-INSTRUCTIONS.md").write_text("# Agent Instructions\n")
        (repo_root / "doc" / "sooperlooper" / "FORK-CHANGELOG.md").write_text("# Fork Changelog\n")
        (repo_root / "tests" / "audio" / "README.md").write_text("# Audio Tests\n")
        
        # Create a random document that is NOT in the historical forensic-closure list
        # and does not have the required forensic-closure markers
        random_doc_content = """# Random Document

## Objective
Some objective

## Completed
Some completed work
"""
        (repo_root / "doc" / "sooperlooper" / "checkpoints" / 
         "random-doc.md").write_text(random_doc_content)
        
        # Create CURRENT.md pointing to the random document
        (repo_root / "CURRENT.md").write_text(f"""# Current Status

Immutable checkpoint: doc/sooperlooper/checkpoints/random-doc.md

Current phase: phase-5-exact-recording
Active task: P5-005
""")
        
        # Create WORK-QUEUE.md
        (repo_root / "doc" / "sooperlooper" / "WORK-QUEUE.md").write_text("""# Work Queue

### P5-005
Status: in_progress
""")
        
        # Create PROJECT-MANIFEST.json with required fields
        manifest = {
            "schema_version": 1,
            "repository": {
                "full_name": "arumihsnek/seq66-loves-sooperlooper",
                "upstream_mirror_branch": "master",
                "integration_branch": "fork-main",
                "active_branch": "fix/baseline-gov-validator-cp051",
                "active_pull_request_base": "fork-main",
                "active_pull_request": 1
            },
            "coordination": {
                "current_checkpoint": "CURRENT.md",
                "checkpoint_protocol": "doc/sooperlooper/CHECKPOINTS.md",
                "work_queue": "doc/sooperlooper/WORK-QUEUE.md",
                "roadmap": "doc/sooperlooper/ROADMAP.md",
                "workflow": ".github/workflows/project-control.yml",
                "traceability": "doc/sooperlooper/TRACEABILITY.md",
                "decision_log": "doc/sooperlooper/DECISIONS.md",
                "documentation_map": "doc/sooperlooper/DOCUMENTATION-MAP.md",
                "upstream_sync_policy": "doc/sooperlooper/UPSTREAM-SYNC.md",
                "fork_changelog": "doc/sooperlooper/FORK-CHANGELOG.md",
                "agent_instructions": "doc/sooperlooper/AGENT-INSTRUCTIONS.md",
                "current_phase": "phase-5-exact-recording",
                "completed_phase": "phase-4-native-qt-slot"
            },
            "next_milestone": {
                "active_task": "P5-005"
            },
            "ci": [
                {"workflow": ".github/workflows/project-control.yml", "required": True},
                {"workflow": ".github/workflows/audio-core.yml", "required": True},
                {"workflow": ".github/workflows/sooperlooper-real-engine.yml", "required": True}
            ]
        }
        
        (repo_root / "PROJECT-MANIFEST.json").write_text(__import__('json').dumps(manifest, indent=2))
        
        # Create workflow files referenced in manifest
        (repo_root / ".github" / "workflows").mkdir(parents=True)
        (repo_root / ".github" / "workflows" / "project-control.yml").write_text("# Project Control\n")
        (repo_root / ".github" / "workflows" / "audio-core.yml").write_text("# Audio Core\n")
        (repo_root / ".github" / "workflows" / "sooperlooper-real-engine.yml").write_text("# Real Engine\n")
        
        # Change to the temporary repo root and run validation
        original_cwd = Path.cwd()
        try:
            os.chdir(repo_root)
            # Temporarily override the ROOT and MANIFEST_PATH constants
            with patch.object(validate, 'ROOT', repo_root), \
                 patch.object(validate, 'MANIFEST_PATH', repo_root / "PROJECT-MANIFEST.json"):
                errors = validate.validate()
                # Should have errors because random-doc.md is not in HISTORICAL_FORENSIC_CHECKPOINTS
                # and doesn't have the required forensic-closure markers
                assert errors, f"Expected validation to fail but it passed"
                # Check that the error is about missing headings (since it's not exempt)
                error_msg = " ".join(errors)
                assert "missing heading" in error_msg, f"Expected heading error in: {error_msg}"
        finally:
            os.chdir(original_cwd)


if __name__ == "__main__":
    # Run the tests
    test_is_historical_forensic_closure_function()
    print("✓ is_historical_forensic_closure function tests passed")
    
    test_cp_051_validation_with_mock()
    print("✓ CP-051 validation with mock passed")
    
    test_new_checkpoint_missing_headings_fails_with_mock()
    print("✓ New checkpoint missing headings fails test passed")
    
    test_new_checkpoint_complete_passes_with_mock()
    print("✓ New checkpoint complete passes test passed")
    
    test_random_document_not_forensic_closure_fails_with_mock()
    print("✓ Random document not forensic-closure fails test passed")
    
    print("\nAll tests passed!")