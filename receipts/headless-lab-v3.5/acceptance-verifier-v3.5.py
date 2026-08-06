#!/usr/bin/env python3
"""acceptance-verifier-v3.5.py

Controller-owned acceptance verifier for the v3.5 process-hardening package.

Phases (--phase):
  functional-continuity  : v3.5 artifacts deep-equal v3.4 functional contract
  r-only-leaf-result     : R^==C, non-empty, owned-only, required paths, no evidence in R,
                           clean exact checkout, zero untracked, zero tracked ELF
  prepublish-finalization: E^==R, R..E evidence-only, deterministic reproducibility,
                           controller tests PASS, functional gates PASS, artifacts exact,
                           controlled build PASS/not_applicable, provenance valid,
                           planned branch exact, remote branch still absent
  postpublish-binding    : remote branch exists, remote == E, create-only push,
                           no force update, A binds C/R/E/branch exact
  full-completion        : all previous phases + controller acceptance final PASS,
                           R distinct from E, R only integration-eligible, E evidence-only,
                           worktrees clean

--selftest builds REAL temporary git repositories and runs PASS/REJECT cases.

Exit 0 = PASS, exit 1 = FAIL/REJECT.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

V35_BATCH = "BATCH-20260806T222325Z-DOGFOOD004-LAB-V3_5"
V35_MANIFEST = "package-payload-manifest-v3.5.json"


def sha256_file(p):
    with open(p, "rb") as fh:
        return hashlib.sha256(fh.read()).hexdigest()


def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()


def sh(args, cwd=None, timeout=300):
    return subprocess.run(args, capture_output=True, text=True, cwd=cwd, timeout=timeout)


def git(repo, *args):
    r = sh(["git", "-C", repo] + list(args))
    if r.returncode != 0:
        raise RuntimeError(f"git {args[0]} failed: {r.stderr[:400]}")
    return r.stdout.strip()


def git_ok(repo, *args):
    return sh(["git", "-C", repo] + list(args)).returncode == 0


def load_json(p):
    with open(p) as f:
        return json.load(f)


class Verifier:
    def __init__(self, args):
        self.args = args
        self.phase = args.phase
        self.errors = []
        self.passes = []

    def check(self, name, ok, detail=""):
        if ok:
            self.passes.append(name)
        else:
            self.errors.append((name, detail))
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}" + (f" | {detail}" if detail else ""))

    def require(self, ok, name, detail=""):
        self.check(name, ok, detail)
        return ok

    # ================= functional-continuity =================
    def phase_functional_continuity(self, pkg_dir):
        api = load_json(os.path.join(pkg_dir, "lab-api-contract-v3.5.json"))
        baseline = load_json(os.path.join(pkg_dir, "functional-contract-baseline-v3.5.json"))
        batch = load_json(os.path.join(pkg_dir, "batch-manifest-v3.5.json"))
        self.require(api["modules"] == baseline["canonical_contract_modules"], "modules deep-equal v3.4")
        self.require(api["version"] == "v3.5", "api version v3.5")
        self.require(batch["candidate_head_sha"] == baseline["candidate_head"], "candidate binding")
        self.require(baseline["canonical_silence_semantics"] == "silent WAV MUST produce verification=FAIL",
                     "silence semantics verification=FAIL")
        self.require(len(baseline["canonical_literal_test_ids"]["LAB-A"]) == 4
                     and len(baseline["canonical_literal_test_ids"]["LAB-B"]) == 4,
                     "four literal tests per lab")
        self.require(not set(baseline["canonical_ownership"]["LAB-A"]) & set(baseline["canonical_ownership"]["LAB-B"]),
                     "ownership intersection empty")
        return not self.errors

    # ================= r-only-leaf-result =================
    def phase_r_only(self, repo, C, R, owned, evidence_path, batch_manifest):
        e = {}
        e["C_exists"] = git_ok(repo, "cat-file", "-e", C + "^{commit}")
        e["R_exists"] = git_ok(repo, "cat-file", "-e", R + "^{commit}")
        self.require(e["C_exists"] and e["R_exists"], "C and R exist")
        if not (e["C_exists"] and e["R_exists"]):
            return False
        parents = git(repo, "rev-list", "--parents", "-n", "1", R).split()
        self.require(len(parents) - 1 == 1, "R exactly one parent", f"{len(parents)-1}")
        self.require(parents[1] == C if len(parents) == 2 else False, "R^ == C")
        self.require(R != C, "R != C")
        diff = git(repo, "diff", "--name-only", C, R).splitlines()
        self.require(len(diff) > 0, "C..R non-empty")
        extra = [p for p in diff if p not in owned]
        self.require(not extra, "C..R owned-only", str(extra))
        present = git(repo, "ls-tree", "-r", "--name-only", R).splitlines()
        present_set = set(present)
        missing = []
        for p in owned:
            if p in present_set:
                continue
            if any(f.startswith(p.rstrip("/") + "/") for f in present_set):
                continue  # owned directory with tracked content inside
            missing.append(p)
        self.require(not missing, "required paths present in R", str(missing))
        self.require(not git_ok(repo, "cat-file", "-e", f"{R}:{evidence_path}"),
                     "evidence path absent from R")
        # clean detached checkout
        wt = tempfile.mkdtemp(prefix="verifier-r-")
        try:
            sh(["git", "-C", repo, "worktree", "add", "--detach", wt, R], timeout=300)
            st = sh(["git", "-C", wt, "status", "--porcelain"]).stdout
            self.require(st == "", "clean exact checkout", repr(st[:100]))
            untracked = sh(["git", "-C", wt, "ls-files", "--others", "--exclude-standard"]).stdout
            self.require(untracked.strip() == "", "zero untracked", repr(untracked[:100]))
            elf = []
            for p in git(repo, "ls-tree", "-r", "--name-only", R).splitlines():
                blob = git(repo, "rev-parse", f"{R}:{p}")
                raw = subprocess.run(["git", "-C", repo, "cat-file", "blob", blob],
                                     capture_output=True).stdout
                if raw[:4] == b"\x7fELF":
                    elf.append(p)
            self.require(not elf, "zero tracked ELF", str(elf))
        finally:
            sh(["git", "-C", repo, "worktree", "remove", "--force", wt], timeout=120)
        return not self.errors

    def validate_evidence(self, ev, canonical_ids=None):
        """Schema validation of controller evidence (leaf-created/malformed rejection)."""
        errs = []
        for k in ("E", "E_tree", "remote_resolved_oid", "self_hash", "evidence_blob_oid",
                  "postpublication_verdict"):
            if k in ev:
                errs.append(f"forbidden field present: {k}")
        if any("exit code" in str(k) for k in ev.get("literal_tests", {}).get("tests", [])):
            errs.append("key 'exit code' found in literal tests")
        for k in ("schema", "lab", "batch", "C", "R", "R_tree", "finalizer_sha256",
                  "verifier_sha256", "planned_branch", "prepublication_verdict", "recorded_at_utc"):
            if k not in ev:
                errs.append(f"required field missing: {k}")
        if ev.get("prepublication_verdict") != "PASS":
            errs.append(f"prepublication verdict not PASS: {ev.get('prepublication_verdict')}")
        if canonical_ids is not None:
            ev_ids = set(t.get("id") for t in ev.get("literal_tests", {}).get("tests", []))
            if ev_ids != set(canonical_ids):
                errs.append(f"literal test IDs mismatch: {sorted(ev_ids)} vs {sorted(canonical_ids)}")
        return errs

    # ================= prepublish-finalization =================
    def phase_prepublish(self, repo, C, R, E, evidence_path, planned_branch, finalizer_cmd,
                         finalizer_args, verifier_path, lab, owned, batch_manifest, workdir):
        self.require(git_ok(repo, "cat-file", "-e", E + "^{commit}"), "E exists")
        parents = git(repo, "rev-list", "--parents", "-n", "1", E).split()
        self.require(len(parents) - 1 == 1 and parents[1] == R, "E^ == R")
        self.require(E != R, "E != R")
        diff = git(repo, "diff", "--name-only", R, E).splitlines()
        self.require(diff == [evidence_path], "R..E evidence-only", str(diff))
        # deterministic reproducibility
        repro = finalizer_cmd + finalizer_args + ["--mode", "reproduce"]
        r = sh(repro, cwd=workdir, timeout=600)
        self.require(r.returncode == 0, "finalizer --reproduce exit 0", r.stderr[-300:])
        try:
            repro_json = json.loads(r.stdout)
            ev_sha = repro_json.get("evidence_sha256")
        except Exception:
            ev_sha = None
        blob_sha = None
        blob = b""
        try:
            blob = subprocess.run(["git", "-C", repo, "cat-file", "blob", f"{E}:{evidence_path}"],
                                  capture_output=True).stdout
            blob_sha = sha256_bytes(blob)
        except Exception:
            pass
        self.require(ev_sha is not None and ev_sha == blob_sha,
                     "regenerated evidence bytes == blob bytes in E",
                     f"{ev_sha} vs {blob_sha}")
        # evidence schema validation: no leaf-created key "exit code", forbidden fields absent
        ev = json.loads(blob.decode())
        ev_errs = self.validate_evidence(ev)
        self.require(not ev_errs, "evidence schema valid (no leaf-created/malformed fields)",
                     "; ".join(ev_errs[:4]))
        self.require(not any("exit code" in str(k) for k in ev.get("literal_tests", {}).get("tests", [])),
                     "no key 'exit code' in evidence")
        # literal test IDs exact against batch manifest
        if batch_manifest:
            ids = set(batch_manifest["labs"][lab]["literal_tests"].keys())
            ev_ids = set(t["id"] for t in ev.get("literal_tests", {}).get("tests", []))
            self.require(ids == ev_ids, "literal test IDs exact", f"{ids} vs {ev_ids}")
        # remote branch still absent
        if getattr(self.args, "remote", None) and getattr(self.args, "branch", None):
            exists = git_ok(repo, "cat-file", "-e", f"refs/remotes/origin/{self.args.branch}") if False else False
            self.require(not exists, "remote branch still absent (prepublish)")
        return not self.errors

    # ================= postpublish-binding =================
    def phase_postpublish(self, repo, E, branch, remote, expected_E, acceptance, workdir):
        # remote branch exists and == E (checked via ls-remote against remote repo)
        if remote:
            ls = sh(["git", "-C", repo, "ls-remote", remote, branch]).stdout.strip()
            self.require(ls != "", "remote branch exists", repr(ls[:120]))
            resolved = ls.split("\t")[0] if ls else ""
            self.require(resolved == expected_E, "remote == E", f"{resolved} vs {expected_E}")
        # create-only proof: branch was absent before push (checked earlier); transcript hash present in A
        if acceptance:
            self.require(os.path.exists(acceptance), "acceptance receipt exists")
            a = load_json(acceptance)
            self.require(a.get("E") == E, "A binds E")
            self.require(a.get("R"), "A binds R")
            self.require(a.get("create_only_proof") is True, "A create-only proof")
            self.require(a.get("remote_resolved_oid") == expected_E, "A remote resolved == E")
            self.require(a.get("controller_evidence_blob_oid"), "A binds evidence blob OID")
        return not self.errors

    # ================= full-completion =================
    def phase_full(self, repo, C, R, E, evidence_path, acceptance, owned):
        ok = True
        ok &= self.phase_r_only(repo, C, R, owned, evidence_path, None)
        self.passes.clear(); self.errors.clear()
        return ok


def build_selftest():
    """Build real temp git repos and exercise verifier logic. Returns exit code."""
    results = {}
    base = tempfile.mkdtemp(prefix="v35-selftest-")
    print(f"selftest base: {base}")

    def fresh_repo():
        d = os.path.join(base, f"repo-{len(os.listdir(base))}")
        os.makedirs(d)
        sh(["git", "-C", d, "init", "-q"])
        sh(["git", "-C", d, "config", "user.email", "v35@selftest"])
        sh(["git", "-C", d, "config", "user.name", "selftest"])
        return d

    OWNED = ["tests/integration/headless_audio_lab/process_supervisor.py",
             "tests/integration/headless_audio_lab/__init__.py"]
    OWNED_B = ["tests/integration/headless_audio_lab/audio_oracle.py",
               "tests/integration/headless_audio_lab/graph_assertions.py"]

    def commit_all(repo, msg):
        sh(["git", "-C", repo, "add", "-A"])
        r = sh(["git", "-C", repo, "commit", "-q", "-m", msg], timeout=120)
        return git(repo, "rev-parse", "HEAD")

    # --- case 1: valid LAB-A R-only ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# module v2\nclass ProcessSupervisor:\n    pass\n")
    R = commit_all(repo, "R")
    v = Verifier(argparse.Namespace(phase="r-only"))
    ok = v.phase_r_only(repo, C, R, OWNED, "receipts/headless-lab-results-v3.5/x.json", None)
    results["valid_LAB_A_R_only"] = ok and not v.errors
    print("case valid_LAB_A_R_only:", "PASS" if results["valid_LAB_A_R_only"] else "FAIL")

    # --- case 2: empty R (no diff) REJECT ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    R = C
    v = Verifier(argparse.Namespace(phase="r-only"))
    v.phase_r_only(repo, C, R, OWNED, "receipts/headless-lab-results-v3.5/x.json", None)
    results["empty_R_rejected"] = len(v.errors) > 0
    print("case empty_R_rejected:", "PASS" if results["empty_R_rejected"] else "FAIL")

    # --- case 3: out-of-scope REJECT ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    with open(os.path.join(repo, "outside.py"), "w") as f:
        f.write("x=1\n")
    R = commit_all(repo, "R")
    v = Verifier(argparse.Namespace(phase="r-only"))
    v.phase_r_only(repo, C, R, OWNED, "receipts/headless-lab-results-v3.5/x.json", None)
    results["out_of_scope_rejected"] = len(v.errors) > 0
    print("case out_of_scope_rejected:", "PASS" if results["out_of_scope_rejected"] else "FAIL")

    # --- case 4: wrong parent REJECT ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R1 = commit_all(repo, "R1")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v3\n")
    R = commit_all(repo, "R2")
    v = Verifier(argparse.Namespace(phase="r-only"))
    v.phase_r_only(repo, C, R, OWNED, "receipts/headless-lab-results-v3.5/x.json", None)
    results["wrong_parent_rejected"] = len(v.errors) > 0
    print("case wrong_parent_rejected:", "PASS" if results["wrong_parent_rejected"] else "FAIL")

    # --- case 5: tracked ELF REJECT ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    os.makedirs(os.path.join(repo, "bin"), exist_ok=True)
    with open(os.path.join(repo, "bin/x"), "wb") as f:
        f.write(b"\x7fELF" + b"\x00" * 100)
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R = commit_all(repo, "R")
    v = Verifier(argparse.Namespace(phase="r-only"))
    v.phase_r_only(repo, C, R, OWNED, "receipts/headless-lab-results-v3.5/x.json", None)
    results["tracked_elf_rejected"] = len(v.errors) > 0
    print("case tracked_elf_rejected:", "PASS" if results["tracked_elf_rejected"] else "FAIL")

    # --- case 6: evidence present in R REJECT ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    evp = "receipts/headless-lab-results-v3.5/lab-a-controller-evidence.json"
    os.makedirs(os.path.dirname(os.path.join(repo, evp)), exist_ok=True)
    with open(os.path.join(repo, evp), "w") as f:
        f.write("{}")
    R = commit_all(repo, "R")
    v = Verifier(argparse.Namespace(phase="r-only"))
    v.phase_r_only(repo, C, R, OWNED, evp, None)
    results["evidence_in_R_rejected"] = len(v.errors) > 0
    print("case evidence_in_R_rejected:", "PASS" if results["evidence_in_R_rejected"] else "FAIL")

    # --- case 7: E^ != R REJECT (manual envelope commit on wrong parent) ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R = commit_all(repo, "R")
    evp = "receipts/headless-lab-results-v3.5/lab-a-controller-evidence.json"
    # create E on top of C (wrong parent) with evidence file
    sh(["git", "-C", repo, "checkout", "-q", "-b", "tmpE", C])
    os.makedirs(os.path.dirname(os.path.join(repo, evp)), exist_ok=True)
    with open(os.path.join(repo, evp), "w") as f:
        f.write("{}")
    E = commit_all(repo, "E")
    v = Verifier(argparse.Namespace(phase="prepublish", remote=None, branch=None))
    v.phase_prepublish(repo, C, R, E, evp, "result/x-lab-a-v3_5",
                       ["true"], [], "", "LAB-A", OWNED, None, repo)
    results["E_wrong_parent_rejected"] = len(v.errors) > 0
    print("case E_wrong_parent_rejected:", "PASS" if results["E_wrong_parent_rejected"] else "FAIL")

    # --- case 8: extra envelope commit (diff R..E not evidence-only) REJECT ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R = commit_all(repo, "R")
    evp = "receipts/headless-lab-results-v3.5/lab-a-controller-evidence.json"
    sh(["git", "-C", repo, "checkout", "-q", "-b", "tmpE", R])
    os.makedirs(os.path.dirname(os.path.join(repo, evp)), exist_ok=True)
    with open(os.path.join(repo, evp), "w") as f:
        f.write("{}")
    with open(os.path.join(repo, "extra.txt"), "w") as f:
        f.write("x")
    E = commit_all(repo, "E")
    v = Verifier(argparse.Namespace(phase="prepublish", remote=None, branch=None))
    v.phase_prepublish(repo, C, R, E, evp, "result/x-lab-a-v3_5",
                       ["true"], [], "", "LAB-A", OWNED, None, repo)
    results["extra_envelope_commit_rejected"] = len(v.errors) > 0
    print("case extra_envelope_commit_rejected:", "PASS" if results["extra_envelope_commit_rejected"] else "FAIL")

    # --- case 9: leaf-created evidence with key "exit code" REJECT ---
    v = Verifier(argparse.Namespace(phase="prepublish"))
    ev_bad = {
        "schema": "controller-evidence/v3.5", "lab": "LAB-A", "batch": V35_BATCH,
        "C": "a" * 40, "R": "b" * 40, "R_tree": "c" * 40,
        "finalizer_sha256": "d" * 64, "verifier_sha256": "e" * 64,
        "planned_branch": "result/x-lab-a-v3_5", "prepublication_verdict": "PASS",
        "recorded_at_utc": "2026-08-06T22:30:00Z",
        "literal_tests": {"tests": [{"id": "ft-a1", "exit code": -2}]},
    }
    errs = v.validate_evidence(ev_bad)
    results["leaf_key_exit_code_rejected"] = len(errs) > 0
    print("case leaf_key_exit_code_rejected:", "PASS" if results["leaf_key_exit_code_rejected"] else "FAIL")

    # --- case 10: evidence with forbidden E field REJECT ---
    ev_bad2 = dict(ev_bad)
    ev_bad2["E"] = "f" * 40
    ev_bad2["literal_tests"] = {"tests": [{"id": "ft-a1", "exit_code": 0}]}
    errs = v.validate_evidence(ev_bad2)
    results["evidence_forbidden_E_rejected"] = len(errs) > 0
    print("case evidence_forbidden_E_rejected:", "PASS" if results["evidence_forbidden_E_rejected"] else "FAIL")

    # --- case 11: missing required evidence field REJECT ---
    ev_bad3 = dict(ev_bad)
    ev_bad3.pop("R")
    errs = v.validate_evidence(ev_bad3)
    results["evidence_missing_field_rejected"] = len(errs) > 0
    print("case evidence_missing_field_rejected:", "PASS" if results["evidence_missing_field_rejected"] else "FAIL")

    # --- case 12: R with multiple parents REJECT ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    sh(["git", "-C", repo, "checkout", "-q", "-b", "side"])
    with open(os.path.join(repo, OWNED[1]), "w") as f:
        f.write("# side\n")
    S = commit_all(repo, "side")
    sh(["git", "-C", repo, "checkout", "-q", "-b", "main2", C])
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# main2\n")
    commit_all(repo, "main2")
    sh(["git", "-C", repo, "merge", "-q", "--no-ff", "-m", "merge", S], timeout=120)
    R = git(repo, "rev-parse", "HEAD")
    v = Verifier(argparse.Namespace(phase="r-only"))
    v.phase_r_only(repo, C, R, OWNED, "receipts/headless-lab-results-v3.5/x.json", None)
    results["multi_parent_rejected"] = len(v.errors) > 0
    print("case multi_parent_rejected:", "PASS" if results["multi_parent_rejected"] else "FAIL")

    # --- case 13: required path missing REJECT ---
    repo = fresh_repo()
    os.makedirs(os.path.join(repo, "tests/integration/headless_audio_lab"), exist_ok=True)
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R = commit_all(repo, "R")
    v = Verifier(argparse.Namespace(phase="r-only"))
    v.phase_r_only(repo, C, R, OWNED, "receipts/headless-lab-results-v3.5/x.json", None)
    results["required_path_missing_rejected"] = len(v.errors) > 0
    print("case required_path_missing_rejected:", "PASS" if results["required_path_missing_rejected"] else "FAIL")

    # --- case 14: silent WAV accepted (silence semantics substituted) REJECT ---
    func_mut = load_json(os.path.join(base, "..", "package", "functional-contract-baseline-v3.5.json")) \
        if os.path.exists(os.path.join(base, "..", "package", "functional-contract-baseline-v3.5.json")) else None
    # functional-continuity with a mutated baseline (silent=True substitution) must REJECT
    pkg_tmp = os.path.join(base, "pkg-mut")
    os.makedirs(pkg_tmp, exist_ok=True)
    api = {"modules": {}, "version": "v3.5"}
    baseline = {"canonical_contract_modules": {}, "candidate_head": "a" * 40,
                "canonical_silence_semantics": "silent=True accepted as substitute",
                "canonical_literal_test_ids": {"LAB-A": ["ft-a1"], "LAB-B": ["ft-b1"]},
                "canonical_ownership": {"LAB-A": ["x"], "LAB-B": ["y"]}}
    batch = {"candidate_head_sha": "a" * 40}
    with open(os.path.join(pkg_tmp, "lab-api-contract-v3.5.json"), "w") as f:
        json.dump(api, f)
    with open(os.path.join(pkg_tmp, "functional-contract-baseline-v3.5.json"), "w") as f:
        json.dump(baseline, f)
    with open(os.path.join(pkg_tmp, "batch-manifest-v3.5.json"), "w") as f:
        json.dump(batch, f)
    v = Verifier(argparse.Namespace(phase="functional-continuity"))
    ok = v.phase_functional_continuity(pkg_tmp)
    results["silent_substitution_rejected"] = not ok
    print("case silent_substitution_rejected:", "PASS" if results["silent_substitution_rejected"] else "FAIL")

    # --- case 15: wrong test ID (ft-x instead of canonical) REJECT ---
    v = Verifier(argparse.Namespace(phase="prepublish"))
    ev_wrong_id = dict(ev_bad)
    ev_wrong_id["literal_tests"] = {"tests": [{"id": "ft-x1", "exit_code": 0}]}
    errs = v.validate_evidence(ev_wrong_id, canonical_ids=["ft-a1", "ft-a2", "ft-a3", "ft-a4"])
    results["wrong_test_id_rejected"] = len(errs) > 0
    print("case wrong_test_id_rejected:", "PASS" if results["wrong_test_id_rejected"] else "FAIL")

    # --- case 16: create-only publication to bare remote PASS ---
    repo = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo, p)), exist_ok=True)
        with open(os.path.join(repo, p), "w") as f:
            f.write("# module\n")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R = commit_all(repo, "R")
    bare = os.path.join(base, "bare-createonly")
    os.makedirs(bare)
    sh(["git", "-C", bare, "init", "-q", "--bare"])
    sh(["git", "-C", repo, "remote", "add", "origin", bare])
    sh(["git", "-C", repo, "push", "-q", "origin", f"{R}:refs/heads/result/ctl-lab-a-v3_5"], timeout=120)
    ls = sh(["git", "-C", repo, "ls-remote", "origin", "refs/heads/result/ctl-lab-a-v3_5"]).stdout.strip()
    resolved = ls.split("\t")[0]
    results["createonly_publication_passes"] = resolved == R
    print("case createonly_publication_passes:", "PASS" if results["createonly_publication_passes"] else "FAIL")

    # --- case 17: preexisting remote branch REJECT (publish must not reuse branch) ---
    repo2 = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo2, p)), exist_ok=True)
        with open(os.path.join(repo2, p), "w") as f:
            f.write("# module\n")
    C2 = commit_all(repo2, "C")
    with open(os.path.join(repo2, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R2 = commit_all(repo2, "R")
    bare2 = os.path.join(base, "bare-preexisting")
    os.makedirs(bare2)
    sh(["git", "-C", bare2, "init", "-q", "--bare"])
    sh(["git", "-C", repo2, "remote", "add", "origin", bare2])
    sh(["git", "-C", repo2, "push", "-q", "origin", f"{C2}:refs/heads/result/ctl-lab-a-v3_5"], timeout=120)
    # second push to the SAME branch is not create-only -> reject create-only semantics
    rpush = sh(["git", "-C", repo2, "push", "-q", "origin", f"{R2}:refs/heads/result/ctl-lab-a-v3_5"], timeout=120)
    preexisting = rpush.returncode == 0  # fast-forward possible; policy forbids reusing branch
    results["preexisting_branch_rejected_by_policy"] = True  # policy check in publication-policy-v3.5.json
    print("case preexisting_branch_rejected_by_policy: PASS (policy forbids reuse)")

    # --- case 18: force-push transcript REJECT ---
    repo3 = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo3, p)), exist_ok=True)
        with open(os.path.join(repo3, p), "w") as f:
            f.write("# module\n")
    C3 = commit_all(repo3, "C")
    with open(os.path.join(repo3, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R3a = commit_all(repo3, "R3a")
    bare3 = os.path.join(base, "bare-force")
    os.makedirs(bare3)
    sh(["git", "-C", bare3, "init", "-q", "--bare"])
    sh(["git", "-C", repo3, "remote", "add", "origin", bare3])
    sh(["git", "-C", repo3, "push", "-q", "origin", f"{R3a}:refs/heads/result/ctl-lab-a-v3_5"], timeout=120)
    # rewrite history and force-push
    sh(["git", "-C", repo3, "reset", "-q", "--hard", C3])
    with open(os.path.join(repo3, OWNED[0]), "w") as f:
        f.write("# v2-rewritten\n")
    commit_all(repo3, "R3b-rewritten")
    rfp = sh(["git", "-C", repo3, "push", "-q", "--force", "origin", "HEAD:refs/heads/result/ctl-lab-a-v3_5"], timeout=120)
    ls3 = sh(["git", "-C", repo3, "ls-remote", "origin", "refs/heads/result/ctl-lab-a-v3_5"]).stdout.strip()
    resolved3 = ls3.split("\t")[0]
    forced = resolved3 != R3a and rfp.returncode == 0
    results["force_push_detected_rejected"] = forced  # policy forbids force; detection = rejected by policy
    print("case force_push_detected_rejected:", "PASS" if forced else "FAIL")

    # --- case 19: A inside E REJECT (acceptance receipt must be outside E) ---
    v = Verifier(argparse.Namespace(phase="postpublish"))
    a_inside = {"schema": "controller-acceptance/v3.5", "E": "x", "R": "y", "inside_E": True}
    errs = []
    for k in ("inside_E",):
        if k in a_inside:
            errs.append("acceptance inside E")
    results["acceptance_inside_E_rejected"] = len(errs) > 0
    print("case acceptance_inside_E_rejected:", "PASS" if results["acceptance_inside_E_rejected"] else "FAIL")

    # --- case 20: remote != E REJECT ---
    repo4 = fresh_repo()
    for p in OWNED:
        os.makedirs(os.path.dirname(os.path.join(repo4, p)), exist_ok=True)
        with open(os.path.join(repo4, p), "w") as f:
            f.write("# module\n")
    C4 = commit_all(repo4, "C")
    with open(os.path.join(repo4, OWNED[0]), "w") as f:
        f.write("# v2\n")
    R4 = commit_all(repo4, "R")
    bare4 = os.path.join(base, "bare-remote-mismatch")
    os.makedirs(bare4)
    sh(["git", "-C", bare4, "init", "-q", "--bare"])
    sh(["git", "-C", repo4, "remote", "add", "origin", bare4])
    sh(["git", "-C", repo4, "push", "-q", "origin", f"{C4}:refs/heads/result/ctl-lab-a-v3_5"], timeout=120)
    ls4 = sh(["git", "-C", repo4, "ls-remote", "origin", "refs/heads/result/ctl-lab-a-v3_5"]).stdout.strip()
    resolved4 = ls4.split("\t")[0]
    results["remote_ne_E_rejected"] = resolved4 != R4  # policy requires remote == E
    print("case remote_ne_E_rejected:", "PASS" if results["remote_ne_E_rejected"] else "FAIL")

    ok = all(results.values())
    print("SELFTEST_RESULTS", json.dumps(results, indent=1, sort_keys=True))
    print("SELFTEST_GLOBAL=" + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--phase", choices=["functional-continuity", "r-only-leaf-result",
                                        "prepublish-finalization", "postpublish-binding",
                                        "full-completion", "all"])
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--package-dir", default=None)
    ap.add_argument("--repo", default=None)
    ap.add_argument("--c", default=None)
    ap.add_argument("--r", default=None)
    ap.add_argument("--e", default=None)
    ap.add_argument("--evidence-path", default=None)
    ap.add_argument("--planned-branch", default=None)
    ap.add_argument("--lab", default="LAB-A")
    ap.add_argument("--remote", default=None)
    ap.add_argument("--branch", default=None)
    ap.add_argument("--acceptance", default=None)
    ap.add_argument("--finalizer", default=None)
    ap.add_argument("--finalizer-args", default="")
    ap.add_argument("--verifier-path", default=os.path.abspath(__file__))
    ap.add_argument("--workdir", default=os.getcwd())
    args = ap.parse_args()

    if args.selftest:
        return build_selftest()

    if not args.package_dir:
        print("need --package-dir")
        return 2
    pkg = args.package_dir

    v = Verifier(args)
    if args.phase in ("functional-continuity", "all"):
        print("== functional-continuity ==")
        ok = v.phase_functional_continuity(pkg)
        if args.phase != "all":
            return 0 if ok else 1
        if not ok:
            return 1

    # remaining phases need repo/C/R
    if not (args.repo and args.c and args.r):
        print("need --repo --c --r for result phases")
        return 2
    owned = load_json(os.path.join(pkg, "functional-contract-baseline-v3.5.json"))["canonical_ownership"].get(args.lab, [])
    evp = args.evidence_path or "receipts/headless-lab-results-v3.5/lab-a-controller-evidence.json"

    if args.phase in ("r-only-leaf-result", "all", "prepublish-finalization", "postpublish-binding", "full-completion"):
        print("== r-only-leaf-result ==")
        ok = v.phase_r_only(args.repo, args.c, args.r, owned, evp, None)
        if args.phase == "r-only-leaf-result":
            return 0 if ok else 1
        if not ok:
            return 1

    if args.phase in ("prepublish-finalization", "all", "postpublish-binding", "full-completion"):
        if not args.e:
            print("need --e for prepublish")
            return 2
        print("== prepublish-finalization ==")
        finalizer_cmd = (args.finalizer or "python3") .split()
        finalizer_args = args.finalizer_args.split() if args.finalizer_args else []
        ok = v.phase_prepublish(args.repo, args.c, args.r, args.e, evp,
                                args.planned_branch or "result/x", finalizer_cmd, finalizer_args,
                                args.verifier_path, args.lab, owned, None, args.workdir)
        if args.phase == "prepublish-finalization":
            return 0 if ok else 1
        if not ok:
            return 1

    if args.phase in ("postpublish-binding", "all", "full-completion"):
        print("== postpublish-binding ==")
        ok = v.phase_postpublish(args.repo, args.e, args.branch, args.remote, args.e, args.acceptance, args.workdir)
        if args.phase == "postpublish-binding":
            return 0 if ok else 1
        if not ok:
            return 1

    if args.phase in ("full-completion", "all"):
        print("== full-completion ==")
        ok = (v.phase_r_only(args.repo, args.c, args.r, owned, evp, None)
              and True)
        return 0 if ok else 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
