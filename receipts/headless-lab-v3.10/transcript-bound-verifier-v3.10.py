#!/usr/bin/env python3
"""Fail-closed v3.10 verifier for transcript-bound true-leaf provenance."""
from __future__ import annotations
import argparse, hashlib, json, subprocess, sys
from datetime import datetime, timezone
from pathlib import Path


def fail(code: str, detail: str, failures: list[dict]) -> None:
    failures.append({"code": code, "detail": detail})


def ts(value):
    if isinstance(value, (int, float)):
        return float(value)
    if not isinstance(value, str):
        return None
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00")).timestamp()
    except ValueError:
        return None


def git(repo: Path, *args: str) -> tuple[int, str, str]:
    p = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True)
    return p.returncode, p.stdout.strip(), p.stderr.strip()


def verify(receipt_path: Path, transcript_path: Path, repo: Path, expected_leaf_id: str | None = None,
           expected_batch: str | None = None, expected_candidate: str | None = None) -> dict:
    failures: list[dict] = []
    try:
        receipt = json.loads(receipt_path.read_text())
    except Exception as exc:
        return {"verdict": "REJECT", "failures": [{"code": "RECEIPT_PARSE_FAILED", "detail": str(exc)}]}
    raw = transcript_path.read_bytes()
    observed_hash = hashlib.sha256(raw).hexdigest()
    if receipt.get("transcript_sha256") != observed_hash:
        fail("TRANSCRIPT_HASH_MISMATCH", f"declared={receipt.get('transcript_sha256')} observed={observed_hash}", failures)
    events = []
    try:
        for line_no, line in enumerate(raw.decode().splitlines(), 1):
            if line.strip():
                event = json.loads(line)
                event["_line"] = line_no
                events.append(event)
    except Exception as exc:
        fail("TRANSCRIPT_PARSE_FAILED", str(exc), failures)
    leaf_start = next((e for e in events if e.get("event") == "leaf_start"), None)
    leaf_end = next((e for e in events if e.get("event") == "leaf_termination"), None)
    if not leaf_start or not leaf_end:
        fail("LEAF_BOUNDARIES_MISSING", "leaf_start and leaf_termination are required", failures)
    start_t = ts(leaf_start.get("timestamp")) if leaf_start else None
    end_t = ts(leaf_end.get("timestamp")) if leaf_end else None
    if start_t is None or end_t is None:
        fail("LEAF_BOUNDARIES_INVALID", "timestamps must be ISO-8601 or numeric", failures)
    if start_t is not None and end_t is not None and end_t < start_t:
        fail("LEAF_BOUNDARIES_REVERSED", "termination precedes start", failures)
    if expected_leaf_id and receipt.get("leaf_id") != expected_leaf_id:
        fail("LEAF_ID_MISMATCH", f"expected={expected_leaf_id} observed={receipt.get('leaf_id')}", failures)
    if expected_batch and receipt.get("batch") != expected_batch:
        fail("BATCH_ID_MISMATCH", f"expected={expected_batch} observed={receipt.get('batch')}", failures)
    if expected_candidate and receipt.get("candidate_commit") != expected_candidate:
        fail("CANDIDATE_MISMATCH", f"expected={expected_candidate} observed={receipt.get('candidate_commit')}", failures)
    if receipt.get("terminal_status") != "completed":
        fail("LEAF_FAILED_TO_PRODUCE_R", f"terminal_status={receipt.get('terminal_status')}", failures)
    commands = [e for e in events if e.get("event") == "command"]
    leaf_adds = [e for e in commands if e.get("actor") == "leaf" and e.get("command", [])[:2] == ["git", "add"]]
    leaf_commits = [e for e in commands if e.get("actor") == "leaf" and e.get("command", [])[:2] == ["git", "commit"]]
    controller_mutations = []
    forbidden_tokens = {"add", "commit", "amend", "cherry-pick", "rebase", "fixup"}
    for e in commands:
        cmd = e.get("command", [])
        joined = " ".join(str(x) for x in cmd)
        actor = e.get("actor")
        action = e.get("action", "")
        event_t = ts(e.get("timestamp"))
        is_mutation = (actor == "controller" and (action in {"implementation_edit", "git_add", "git_commit", "git_amend", "cherry_pick", "rebase", "fixup"}
                       or (cmd and cmd[0] == "git" and any(t in cmd[1:] for t in forbidden_tokens))))
        if is_mutation and (start_t is None or event_t is None or event_t >= start_t):
            controller_mutations.append(e)
    controller_edits = [e for e in controller_mutations if e.get("action") == "implementation_edit"]
    controller_git = [e for e in controller_mutations if e.get("command", [None])[0] == "git"]
    if not leaf_adds:
        fail("LEAF_GIT_ADD_MISSING", "transcript has no leaf git add", failures)
    if not leaf_commits:
        fail("LEAF_DID_NOT_CREATE_R", "transcript has no leaf git commit", failures)
    if len(leaf_commits) != 1:
        fail("LEAF_COMMIT_COUNT_INVALID", f"leaf commit events={len(leaf_commits)} expected=1", failures)
    if controller_mutations:
        for e in controller_mutations:
            cmd = " ".join(str(x) for x in e.get("command", []))
            if e.get("action") == "implementation_edit":
                fail("CONTROLLER_ACTED_AS_LEAF", f"controller implementation edit line={e.get('_line')}", failures)
            elif "amend" in cmd or e.get("action") == "git_amend":
                fail("CONTROLLER_AMEND_AFTER_LEAF", f"controller amend line={e.get('_line')}", failures)
            else:
                fail("CONTROLLER_ACTED_AS_LEAF", f"controller mutation line={e.get('_line')} command={cmd}", failures)
    if receipt.get("controller_implementation_edit_count_after_leaf_start") != 0:
        fail("CONTROLLER_IMPLEMENTATION_EDIT_COUNT_NONZERO", "receipt count is not zero", failures)
    if receipt.get("controller_git_add_count_in_leaf_worktree") != 0 or receipt.get("controller_git_commit_count_in_leaf_worktree") != 0:
        fail("CONTROLLER_GIT_MUTATION_COUNT_NONZERO", "receipt controller git counts are not zero", failures)
    leaf_commit_event = leaf_commits[0] if leaf_commits else None
    if leaf_commit_event and end_t is not None and ts(leaf_commit_event.get("timestamp")) is not None and ts(leaf_commit_event.get("timestamp")) > end_t:
        fail("R_CREATED_AFTER_LEAF_TERMINATION", "leaf commit event occurs after termination", failures)
    declared_r = receipt.get("R")
    final_head = receipt.get("leaf_final_rev_parse_HEAD")
    if declared_r != final_head:
        fail("DECLARED_R_NOT_LEAF_FINAL_HEAD", f"R={declared_r} final_head={final_head}", failures)
    if leaf_commit_event and leaf_commit_event.get("resulting_commit") != declared_r:
        fail("LEAF_COMMIT_NOT_DECLARED_R", f"event={leaf_commit_event.get('resulting_commit')} R={declared_r}", failures)
    if leaf_end and leaf_end.get("final_head") != declared_r:
        fail("TRANSCRIPT_FINAL_HEAD_MISMATCH", f"transcript={leaf_end.get('final_head')} R={declared_r}", failures)
    if leaf_end and leaf_end.get("git_status") not in ([], ""):
        fail("LEAF_WORKTREE_DIRTY", f"status={leaf_end.get('git_status')}", failures)
    if declared_r and len(declared_r) == 40:
        rc, _, err = git(repo, "cat-file", "-e", f"{declared_r}^{{commit}}")
        if rc != 0:
            fail("R_COMMIT_MISSING", err or declared_r, failures)
        rc, parent, _ = git(repo, "rev-parse", f"{declared_r}^")
        if rc == 0 and receipt.get("R_parent") != parent:
            fail("R_PARENT_MISMATCH", f"declared={receipt.get('R_parent')} observed={parent}", failures)
        rc, tree, _ = git(repo, "rev-parse", f"{declared_r}^{{tree}}")
        if rc == 0 and receipt.get("R_tree") != tree:
            fail("R_TREE_MISMATCH", f"declared={receipt.get('R_tree')} observed={tree}", failures)
        candidate = receipt.get("candidate_commit")
        if rc == 0 and candidate:
            rc2, parent2, _ = git(repo, "rev-parse", f"{declared_r}^")
            if rc2 == 0 and parent2 != candidate:
                fail("R_PARENT_NOT_CANDIDATE", f"parent={parent2} candidate={candidate}", failures)
    if leaf_commits and any("user.name=LEAF-" in " ".join(map(str,e.get("command",[]))) for e in controller_mutations):
        fail("FAKE_LEAF_GIT_IDENTITY", "controller command used LEAF identity", failures)
    return {"verdict": "PASS" if not failures else "REJECT", "failure_count": len(failures), "failures": failures,
            "observed": {"transcript_sha256": observed_hash, "leaf_commit_events": len(leaf_commits), "controller_mutations": len(controller_mutations)}}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--receipt", required=True)
    ap.add_argument("--transcript", required=True)
    ap.add_argument("--repository", required=True)
    ap.add_argument("--expected-leaf-id")
    ap.add_argument("--expected-batch")
    ap.add_argument("--expected-candidate")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()
    result = verify(Path(args.receipt), Path(args.transcript), Path(args.repository), args.expected_leaf_id, args.expected_batch, args.expected_candidate)
    print(json.dumps(result, indent=1, sort_keys=True))
    return 0 if result["verdict"] == "PASS" else 1

if __name__ == "__main__":
    raise SystemExit(main())
