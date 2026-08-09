#!/usr/bin/env python3
"""Real temporary-git selftests for transcript-bound v3.10 provenance."""
from __future__ import annotations
import hashlib, json, os, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
VERIFIER = HERE / "transcript-bound-verifier-v3.10.py"
CANDIDATE = "a" * 40
BATCH = "BATCH-SELFTEST-DOGFOOD004-LAB-V3_10"
LEAF = "LEAF-LAB-A-V3_10"
LEASE = "LEASE-SELFTEST-LAB-A-V3_10"


def run(cmd, cwd):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)


def commit(repo: Path, message: str, actor: str = "LEAF-LAB-A-V3_10", amend=False) -> str:
    run(["git", "config", "user.name", actor], repo)
    run(["git", "config", "user.email", "leaf@example.invalid"], repo)
    args = ["git", "commit"] + (["--amend"] if amend else []) + ["-m", message]
    p = run(args, repo)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return run(["git", "rev-parse", "HEAD"], repo).stdout.strip()


def init_repo(root: Path):
    repo = root / "repo"
    repo.mkdir()
    run(["git", "init", "-q"], repo)
    run(["git", "config", "user.name", "Seed"], repo)
    run(["git", "config", "user.email", "seed@example.invalid"], repo)
    (repo / "candidate.txt").write_text("candidate\n")
    run(["git", "add", "candidate.txt"], repo)
    run(["git", "commit", "-q", "-m", "candidate"], repo)
    return repo, run(["git", "rev-parse", "HEAD"], repo).stdout.strip()


def make_transcript(events):
    return "".join(json.dumps(e, sort_keys=True) + "\n" for e in events).encode()


def receipt(repo, transcript_path, r, parent, tree, final_head=None, status=None, counts=(0, 0, 0), execution="exec-selftest"):
    raw = transcript_path.read_bytes()
    return {
        "lab": "LAB-A", "batch": BATCH, "leaf_id": LEAF, "lease_id": LEASE,
        "candidate_commit": parent, "leaf_execution_id": execution,
        "leaf_start_utc": "1970-01-01T00:01:40Z", "leaf_end_utc": "1970-01-01T00:01:43Z",
        "terminal_status": status or "completed", "transcript_artifact": transcript_path.name,
        "transcript_sha256": hashlib.sha256(raw).hexdigest(), "R": r,
        "R_tree": tree, "R_parent": parent, "leaf_git_add_evidence": {"line": 2},
        "leaf_git_commit_evidence": {"line": 3}, "leaf_final_rev_parse_HEAD": final_head or r,
        "leaf_final_git_status": [], "controller_execution_id": "controller-selftest",
        "controller_implementation_edit_count_after_leaf_start": counts[0],
        "controller_git_add_count_in_leaf_worktree": counts[1],
        "controller_git_commit_count_in_leaf_worktree": counts[2],
    }


def execute_case(name, mode, expected_code=None):
    with tempfile.TemporaryDirectory(prefix="v310-prov-") as td:
        root = Path(td); repo, parent = init_repo(root)
        transcript_path = root / "transcript.jsonl"
        if mode == "P1":
            (repo / "owned.py").write_text("leaf=True\n"); run(["git", "add", "owned.py"], repo)
            r = commit(repo, "R", actor="LEAF-LAB-A-V3_10")
            tree = run(["git", "rev-parse", f"{r}^{{tree}}"], repo).stdout.strip()
            events = [{"event":"leaf_start","timestamp":100}, {"event":"command","timestamp":101,"actor":"leaf","command":["git","add","owned.py"]}, {"event":"command","timestamp":102,"actor":"leaf","command":["git","commit","-m","R"],"resulting_commit":r}, {"event":"leaf_termination","timestamp":103,"terminal_status":"completed","final_head":r,"git_status":[]}, {"event":"validation_start","timestamp":104,"actor":"controller"}]
            expected = "PASS"
        elif mode in {"N1", "N5"}:
            (repo / "owned.py").write_text("controller=True\n"); run(["git", "add", "owned.py"], repo)
            actor = "LEAF-LAB-A-V3_10" if mode == "N5" else "Hermes Controller"
            r = commit(repo, "R", actor=actor); tree = run(["git", "rev-parse", f"{r}^{{tree}}"], repo).stdout.strip()
            cmd = ["git","-c","user.name=LEAF-LAB-A-V3_10","commit","-m","R"] if mode == "N5" else ["git","commit","-m","R"]
            events = [{"event":"leaf_start","timestamp":100},{"event":"leaf_termination","timestamp":103,"terminal_status":"completed","final_head":r,"git_status":[]},{"event":"command","timestamp":104,"actor":"controller","command":cmd,"resulting_commit":r}]
            expected = "CONTROLLER_ACTED_AS_LEAF" if mode == "N5" else "LEAF_DID_NOT_CREATE_R"
        elif mode == "N2":
            (repo / "partial.py").write_text("partial=True\n");
            (repo / "finished.py").write_text("controller=True\n"); run(["git","add","partial.py","finished.py"],repo)
            r=commit(repo,"R",actor="Hermes Controller"); tree=run(["git","rev-parse",f"{r}^{{tree}}"],repo).stdout.strip()
            events=[{"event":"leaf_start","timestamp":100},{"event":"command","timestamp":101,"actor":"leaf","command":["git","add","partial.py"]},{"event":"leaf_termination","timestamp":103,"terminal_status":"completed","final_head":parent,"git_status":["?? finished.py"]},{"event":"command","timestamp":104,"actor":"controller","action":"implementation_edit","command":["write","finished.py"]},{"event":"command","timestamp":105,"actor":"controller","command":["git","commit","-m","R"],"resulting_commit":r}]
            expected="CONTROLLER_ACTED_AS_LEAF"
        elif mode == "N3":
            (repo / "owned.py").write_text("leaf=True\n"); run(["git","add","owned.py"],repo); r1=commit(repo,"R",actor="LEAF-LAB-A-V3_10"); tree1=run(["git","rev-parse",f"{r1}^{{tree}}"],repo).stdout.strip()
            (repo / "owned.py").write_text("amended=True\n"); run(["git","add","owned.py"],repo); r2=commit(repo,"R amend",actor="LEAF-LAB-A-V3_10",amend=True); tree=run(["git","rev-parse",f"{r2}^{{tree}}"],repo).stdout.strip()
            events=[{"event":"leaf_start","timestamp":100},{"event":"command","timestamp":101,"actor":"leaf","command":["git","add","owned.py"]},{"event":"command","timestamp":102,"actor":"leaf","command":["git","commit","-m","R"],"resulting_commit":r1},{"event":"leaf_termination","timestamp":103,"terminal_status":"completed","final_head":r1,"git_status":[]},{"event":"command","timestamp":104,"actor":"controller","action":"git_amend","command":["git","commit","--amend","-m","R amend"],"resulting_commit":r2}]
            r=r2; expected="CONTROLLER_AMEND_AFTER_LEAF"
        elif mode == "N4":
            (repo / "untracked.py").write_text("dirty=True\n"); r=parent; tree=run(["git","rev-parse",f"{parent}^{{tree}}"],repo).stdout.strip()
            events=[{"event":"leaf_start","timestamp":100},{"event":"leaf_termination","timestamp":103,"terminal_status":"max_iterations","final_head":parent,"git_status":["?? untracked.py"]}]; expected="LEAF_FAILED_TO_PRODUCE_R"
        elif mode == "N6":
            (repo / "owned.py").write_text("leaf=True\n"); run(["git","add","owned.py"],repo); r=commit(repo,"R",actor="LEAF-LAB-A-V3_10"); tree=run(["git","rev-parse",f"{r}^{{tree}}"],repo).stdout.strip(); events=[{"event":"leaf_start","timestamp":100},{"event":"command","timestamp":101,"actor":"leaf","command":["git","add","owned.py"]},{"event":"command","timestamp":102,"actor":"leaf","command":["git","commit","-m","R"],"resulting_commit":r},{"event":"leaf_termination","timestamp":103,"terminal_status":"completed","final_head":r,"git_status":[]}]; expected="TRANSCRIPT_HASH_MISMATCH"
        elif mode == "N7":
            (repo / "owned.py").write_text("leaf=True\n"); run(["git","add","owned.py"],repo); actual=commit(repo,"R",actor="LEAF-LAB-A-V3_10"); tree=run(["git","rev-parse",f"{actual}^{{tree}}"],repo).stdout.strip(); r=parent; events=[{"event":"leaf_start","timestamp":100},{"event":"command","timestamp":101,"actor":"leaf","command":["git","add","owned.py"]},{"event":"command","timestamp":102,"actor":"leaf","command":["git","commit","-m","R"],"resulting_commit":actual},{"event":"leaf_termination","timestamp":103,"terminal_status":"completed","final_head":actual,"git_status":[]}]; expected="DECLARED_R_NOT_LEAF_FINAL_HEAD"
        elif mode == "N8":
            (repo / "owned.py").write_text("leaf=True\n"); run(["git","add","owned.py"],repo); r=commit(repo,"R",actor="LEAF-LAB-A-V3_10"); tree=run(["git","rev-parse",f"{r}^{{tree}}"],repo).stdout.strip(); events=[{"event":"leaf_start","timestamp":100},{"event":"leaf_termination","timestamp":103,"terminal_status":"completed","final_head":r,"git_status":[]},{"event":"command","timestamp":104,"actor":"leaf","command":["git","add","owned.py"]},{"event":"command","timestamp":105,"actor":"leaf","command":["git","commit","-m","R"],"resulting_commit":r}]; expected="R_CREATED_AFTER_LEAF_TERMINATION"
        else: raise ValueError(mode)
        transcript_path.write_bytes(make_transcript(events))
        rec=receipt(repo,transcript_path,r,parent,tree,final_head=(r if mode not in {"N7"} else actual),status=("max_iterations" if mode=="N4" else "completed"),counts=((1,0,1) if mode=="N2" else (0,0,0)))
        if mode == "N6": rec["transcript_sha256"]="0"*64
        rec_path=root/"receipt.json"; rec_path.write_text(json.dumps(rec,sort_keys=True))
        p=subprocess.run([sys.executable,str(VERIFIER),"--receipt",str(rec_path),"--transcript",str(transcript_path),"--repository",str(repo),"--expected-batch",BATCH,"--expected-candidate",parent,"--json"],capture_output=True,text=True)
        out=json.loads(p.stdout)
        ok=(out["verdict"]==expected) if expected=="PASS" else any(x["code"]==expected for x in out.get("failures",[]))
        return {"case":name,"mode":mode,"expected":expected,"exit_code":p.returncode,"verdict":out.get("verdict"),"primary_codes":[x["code"] for x in out.get("failures",[])],"pass":ok}


def main():
    cases=[("P1","P1"),("N1","N1"),("N2","N2"),("N3","N3"),("N4","N4"),("N5","N5"),("N6","N6"),("N7","N7"),("N8","N8")]
    results=[execute_case(n,m) for n,m in cases]
    print(json.dumps({"schema":"actor-provenance-selftests/v3.10","cases":results,"all_pass":all(x["pass"] for x in results)},indent=1,sort_keys=True))
    return 0 if all(x["pass"] for x in results) else 1

if __name__ == "__main__": raise SystemExit(main())
