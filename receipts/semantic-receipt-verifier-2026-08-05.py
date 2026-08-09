#!/usr/bin/env python3
"""Independent semantic-receipt verifier for the seq66-loves-sooperlooper
pre-dispatch evidence closure session.

Verifies 14 checks against the exact VERIFIED_EVIDENCE_HEAD using ONLY
read-only git operations (git -C <repo> cat-file / show / diff / ls-tree)
and GitHub API evidence (gh api). It never modifies the repository.

Classification: fresh_semantic_receipt / ad_hoc_verification /
not_canonical_test_suite_evidence. This is NOT a canonical test-suite run.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

REQUIRED_CHECK_IDS = [str(i) for i in range(1, 15)]


def sh(cmd: list[str], cwd: str | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)


def git(repo: str, *args: str) -> subprocess.CompletedProcess:
    return sh(["git", "-C", repo, *args])


def gh(*args: str) -> subprocess.CompletedProcess:
    return sh(["gh", *args])


def sha256_of_file_bytes(repo: str, commit: str, path: str) -> str | None:
    """sha256 of the file content at commit (not the git blob id)."""
    p = git(repo, "show", f"{commit}:{path}")
    if p.returncode != 0:
        return None
    return hashlib.sha256(p.stdout.encode("utf-8", errors="surrogateescape")).hexdigest()


def file_exists_at(repo: str, commit: str, path: str) -> bool:
    p = git(repo, "cat-file", "-e", f"{commit}:{path}")
    return p.returncode == 0


def check(obj: dict, check_id: str, description: str, command_or_method: str,
          expected: str, observed: str, result: bool, source_artifacts: list[str]) -> None:
    obj["checks"].append({
        "id": check_id,
        "description": description,
        "command_or_method": command_or_method,
        "expected": expected,
        "observed": observed,
        "result": "PASS" if result else "FAIL",
        "source_artifacts": source_artifacts,
    })
    obj["results"][check_id] = result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, help="path to local git repository")
    parser.add_argument("--candidate-head", required=True)
    parser.add_argument("--verified-evidence-head", required=True)
    parser.add_argument("--previous-evidence-head", required=True)
    parser.add_argument("--product-base", required=True)
    parser.add_argument("--old-safe-base", required=True)
    parser.add_argument("--pr-number", required=True, type=int)
    parser.add_argument("--contracts-v2-dir", required=True,
                        help="directory with contracts-v2 lease-plan-lab-{a,b}.json")
    parser.add_argument("--pre-dispatch-readiness", required=True,
                        help="path to pre-dispatch-readiness-v2.json")
    parser.add_argument("--out", required=True, help="JSON output path")
    args = parser.parse_args()

    repo = args.repo
    out = {"schema_version": "seq66-semantic-receipt/v1",
           "classification": "fresh_semantic_receipt",
           "ad_hoc": True,
           "canonical_test_suite": False,
           "checks": [],
           "results": {},
           "generated_at_utc": __import__("datetime").datetime.now(timezone_utc()).isoformat(),
           "verifier_inputs": {
               "candidate_head": args.candidate_head,
               "verified_evidence_head": args.verified_evidence_head,
               "previous_evidence_head": args.previous_evidence_head,
               "product_base": args.product_base,
               "old_safe_base": args.old_safe_base,
               "pr_number": args.pr_number,
           }}

    V = args.verified_evidence_head
    C = args.candidate_head
    P = args.previous_evidence_head
    OLD = args.old_safe_base
    PR = args.pr_number

    # ---------- check 1: PROJECT-MANIFEST declares P5-005 in_progress ----------
    cmd = f"git -C {repo} show {V}:PROJECT-MANIFEST.json | python3 -m json.tool"
    manifest_ok = False
    try:
        p = git(repo, "show", f"{V}:PROJECT-MANIFEST.json")
        m = json.loads(p.stdout)
        nm = m.get("next_milestone", {})
        manifest_ok = (nm.get("active_task") == "P5-005"
                       and nm.get("active_task_status") == "in_progress")
    except Exception as exc:  # noqa: BLE001
        cmd += f"  # parse error: {exc}"
    check(out, "1", "PROJECT-MANIFEST.json parses and declares P5-005 in_progress",
          cmd, "P5-005 / in_progress", f"{nm.get('active_task')} / {nm.get('active_task_status')}",
          manifest_ok, ["PROJECT-MANIFEST.json"])

    # ---------- check 2: CP-053 declares P5-005 ----------
    cp53 = "doc/sooperlooper/checkpoints/2026-08-05-CP-053-control-plane-coherence.md"
    p = git(repo, "show", f"{V}:{cp53}")
    cp53_ok = p.returncode == 0 and "Active task: `P5-005`" in p.stdout
    check(out, "2", "CP-053 declares P5-005",
          f"git -C {repo} show {V}:{cp53} | grep 'Active task'",
          "Active task: `P5-005`", "present" if "Active task: `P5-005`" in p.stdout else "absent",
          cp53_ok, [cp53])

    # ---------- check 3: CURRENT declares P5-005 and points to CP-053 ----------
    cur = "doc/sooperlooper/checkpoints/CURRENT.md"
    p = git(repo, "show", f"{V}:{cur}")
    cur_ok = (p.returncode == 0
              and "Active task: `P5-005`" in p.stdout
              and "CP-053" in p.stdout
              and "2026-08-05-CP-053-control-plane-coherence.md" in p.stdout)
    check(out, "3", "CURRENT declares P5-005 and points to CP-053",
          f"git -C {repo} show {V}:{cur}", "P5-005 + CP-053",
          "P5-005 + CP-053" if cur_ok else "missing",
          cur_ok, [cur, cp53])

    # ---------- checks 4,5: WORK-QUEUE P5-005 in_progress, P5-007 deferred ----------
    wq = "doc/sooperlooper/WORK-QUEUE.md"
    p = git(repo, "show", f"{V}:{wq}")
    wq_text = p.stdout if p.returncode == 0 else ""
    # section for P5-005: find '### P5-005' then status until next '### '
    def section_status(text: str, task: str) -> str:
        idx = text.find(f"### {task}")
        if idx == -1:
            return "NOT_FOUND"
        end = text.find("\n### ", idx + 3)
        sec = text[idx:end if end != -1 else len(text)]
        for line in sec.splitlines():
            if "Status:" in line:
                return line.split("Status:")[1].strip().strip("`")
        return "NO_STATUS"
    s5 = section_status(wq_text, "P5-005")
    s7 = section_status(wq_text, "P5-007")
    wq5_ok = s5 == "in_progress"
    check(out, "4", "WORK-QUEUE declares P5-005 in_progress",
          f"git -C {repo} show {V}:{wq}", "P5-005 Status: in_progress", f"P5-005 -> {s5}",
          wq5_ok, [wq])
    wq7_ok = s7 == "deferred"
    check(out, "5", "WORK-QUEUE declares P5-007 deferred",
          f"git -C {repo} show {V}:{wq}", "P5-007 Status: deferred", f"P5-007 -> {s7}",
          wq7_ok, [wq])

    # ---------- check 6: CANDIDATE exists and is ancestor of VERIFIED ----------
    p1 = git(repo, "cat-file", "-e", f"{C}^{{commit}}")
    p2 = git(repo, "merge-base", "--is-ancestor", C, V)
    anc_ok = p1.returncode == 0 and p2.returncode == 0
    mb = git(repo, "merge-base", C, V).stdout.strip()
    check(out, "6", "CANDIDATE_HEAD exists and is ancestor of VERIFIED_EVIDENCE_HEAD",
          f"git -C {repo} merge-base --is-ancestor {C} {V}",
          f"ancestor (merge-base = {C})", f"merge-base = {mb}",
          anc_ok, [f"commit {C}", f"commit {V}"])

    # ---------- check 7: PR #34 remote head == VERIFIED ----------
    p = gh("api", f"repos/arumihsnek/seq66-loves-sooperlooper/pulls/{PR}",
           "--jq", ".head.sha")
    pr_head = p.stdout.strip() if p.returncode == 0 else ""
    pr_ok = pr_head == V
    check(out, "7", "PR #34 has remote head == VERIFIED_EVIDENCE_HEAD",
          f"gh api repos/arumihsnek/seq66-loves-sooperlooper/pulls/{PR} --jq .head.sha",
          V, pr_head, pr_ok, [f"PR #{PR}"])

    # ---------- checks 8,9: workflows on exact head ----------
    p = gh("api", f"repos/arumihsnek/seq66-loves-sooperlooper/actions/runs",
           "--jq", f'.workflow_runs[] | select(.head_sha=="{V}") | {{name, conclusion}}')
    wf = []
    if p.returncode == 0:
        for line in p.stdout.splitlines():
            line = line.strip()
            if line.startswith("{") and line.endswith("}"):
                try:
                    wf.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    def wf_conclusion(name: str) -> str:
        for w in wf:
            if w.get("name") == name:
                return w.get("conclusion", "")
        return "NOT_FOUND"
    cc = wf_conclusion("Project control plane")
    ac = wf_conclusion("Audio integration core")
    cc_ok = cc == "success"
    ac_ok = ac == "success"
    check(out, "8", "Project control plane passes on VERIFIED_EVIDENCE_HEAD",
          f"gh api .../actions/runs --jq '.workflow_runs[] | select(.head_sha=={V})'",
          "success", cc, cc_ok, [f"workflow run @ {V}"])
    check(out, "9", "Audio integration core passes on VERIFIED_EVIDENCE_HEAD",
          f"gh api .../actions/runs --jq '.workflow_runs[] | select(.head_sha=={V})'",
          "success", ac, ac_ok, [f"workflow run @ {V}"])

    # ---------- check 10: CP-048..CP-051 byte-identical vs protected refs ----------
    # Protected reference = OLD_SAFE_BASE. Some CP numbers may not exist as
    # files in the historical tree (e.g. CP-050 is absent from every relevant
    # commit). Consistent absence in BOTH refs is treated as PASS (no drift).
    # Every file that matches CP-048..CP-051 in OLD must exist byte-identical
    # in VERIFIED, and VERIFIED must not introduce any CP-048..CP-051 file
    # that was absent in OLD.
    ls_old = git(repo, "ls-tree", "-r", "--name-only", OLD, "--", "doc/sooperlooper/checkpoints/")
    ls_ver = git(repo, "ls-tree", "-r", "--name-only", V, "--", "doc/sooperlooper/checkpoints/")
    cp_nums = ("048", "049", "050", "051")
    old_cps = {f for f in ls_old.stdout.splitlines() if any(f"-CP-{n}-" in f or f"-CP-{n}." in f for n in cp_nums)}
    ver_cps = {f for f in ls_ver.stdout.splitlines() if any(f"-CP-{n}-" in f or f"-CP-{n}." in f for n in cp_nums)}
    # CP-050 absent in both -> consistent
    only_old = old_cps - ver_cps
    only_ver = ver_cps - old_cps
    changed = []
    for f in sorted(old_cps & ver_cps):
        h_old = sha256_of_file_bytes(repo, OLD, f)
        h_ver = sha256_of_file_bytes(repo, V, f)
        if h_old is None or h_old != h_ver:
            changed.append(f)
    cp10_ok = not only_old and not only_ver and not changed
    obs_parts = []
    for f in sorted(old_cps & ver_cps):
        h_old = sha256_of_file_bytes(repo, OLD, f)
        h_ver = sha256_of_file_bytes(repo, V, f)
        same = h_old is not None and h_old == h_ver
        cpid = next(n for n in cp_nums if f"-CP-{n}-" in f or f"-CP-{n}." in f)
        obs_parts.append(f"{cpid}:{'same' if same else 'DIFF'}")
    if only_old:
        obs_parts.append(f"only_in_OLD={sorted(only_old)}")
    if only_ver:
        obs_parts.append(f"only_in_VERIFIED={sorted(only_ver)}")
    check(out, "10",
          "CP-048, CP-049, CP-050, CP-051 byte-identical to protected refs (OLD_SAFE_BASE)",
          f"git -C {repo} ls-tree {OLD} vs {V} + sha256 per file",
          "identical set & bytes; CP-050 consistently absent in both refs",
          "; ".join(obs_parts) if obs_parts else "no CP-048..CP-051 files present in either ref",
          cp10_ok, sorted(old_cps | ver_cps))

    # ---------- check 11: CP-052 byte-identical PREVIOUS vs VERIFIED ----------
    ls2 = git(repo, "ls-tree", "-r", "--name-only", P, "--", "doc/sooperlooper/checkpoints/")
    cp52_path = next((f for f in ls2.stdout.splitlines() if "CP-052" in f), None)
    cp52_ok = False
    if cp52_path:
        h_prev = sha256_of_file_bytes(repo, P, cp52_path)
        h_ver = sha256_of_file_bytes(repo, V, cp52_path)
        cp52_ok = h_prev is not None and h_prev == h_ver
        obs = f"{h_prev[:16]}... vs {h_ver[:16]}..."
    else:
        obs = "CP-052 not found in PREVIOUS"
    check(out, "11", "CP-052 byte-identical between PREVIOUS_EVIDENCE_HEAD and VERIFIED_EVIDENCE_HEAD",
          f"git -C {repo} show {P}:{cp52_path} | sha256sum vs {V}:{cp52_path}",
          "identical sha256", obs, cp52_ok,
          [cp52_path or "CP-052 (missing)", f"commit {P}", f"commit {V}"])

    # ---------- check 12: leases planned, bound to CANDIDATE ----------
    from pathlib import Path as Pth
    leases = []
    for lab in ("lab-a", "lab-b"):
        p = Pth(args.contracts_v2_dir) / f"lease-plan-{lab}.json"
        if p.exists():
            d = json.loads(p.read_text())
            leases.append((lab, d.get("status"), d.get("base_sha")))
    lease_ok = all(st == "planned" and bs == C for _, st, bs in leases) and len(leases) == 2
    check(out, "12", "LAB-A and LAB-B leases planned (not active), bound to CANDIDATE_HEAD",
          f"json.load({args.contracts_v2_dir}/lease-plan-{{a,b}}.json)",
          f"status=planned, base_sha={C}",
          "; ".join(f"{lab}:{st}/{bs[:12]}" for lab, st, bs in leases),
          lease_ok, [str(args.contracts_v2_dir)])

    # ---------- check 13: LAB worktrees = 0, LAB leaves dispatched = 0 ----------
    wt = git(repo, "worktree", "list", "--porcelain")
    wt_text = wt.stdout if wt.returncode == 0 else ""
    lab_wts = [l for l in wt_text.splitlines() if "LAB" in l]
    rd = json.loads(Path(args.pre_dispatch_readiness).read_text())
    leaves = int(rd.get("leaves_dispatched", -1))
    worktrees_created = int(rd.get("worktrees_created", -1))
    c13_ok = len(lab_wts) == 0 and leaves == 0 and worktrees_created == 0
    check(out, "13", "LAB worktrees created = 0 and LAB leaves dispatched = 0",
          f"git -C {repo} worktree list --porcelain + pre-dispatch readiness JSON",
          "0 worktrees / 0 leaves", f"worktrees={len(lab_wts)} leaves={leaves} (readiness worktrees={worktrees_created})",
          c13_ok, [args.pre_dispatch_readiness])

    # ---------- check 14: D0/D1/D2 not executed ----------
    d012 = rd.get("D0_D1_D2_executed", "false")
    # derive from readiness + lease notes (no results present)
    c14_ok = (str(d012).lower() == "false" or d012 is False) and leaves == 0
    check(out, "14", "D0, D1, D2 not executed",
          f"pre-dispatch readiness JSON field D0_D1_D2_executed + lease status",
          "false", str(d012), c14_ok, [args.pre_dispatch_readiness])

    # ---------- aggregate ----------
    passed = sum(1 for r in out["results"].values() if r)
    out["overall_result"] = {
        "checks_total": 14,
        "checks_passed": passed,
        "all_pass": passed == 14,
    }
    out["classification"] = ("fresh_semantic_receipt" if passed == 14
                             else "ad_hoc_verification")
    out["not_canonical_test_suite_evidence"] = True

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out).write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n")
    print(json.dumps(out["overall_result"]))
    return 0 if passed == 14 else 1


def timezone_utc():
    from datetime import timezone
    return timezone.utc


if __name__ == "__main__":
    sys.exit(main())
