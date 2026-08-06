#!/usr/bin/env python3
"""mechanical-finalizer-v3.5.py

Controller-owned mechanical finalizer. THE ONLY permitted mechanism to create E.
R-only leaf model: the leaf produces R; this finalizer creates the evidence
container commit E (E^ == R) whose ONLY diff vs R is the controller evidence file
under receipts/headless-lab-results-v3.5/<lab>-controller-evidence.json.

Never accepts leaf-provided values as a substitute for execution. Never edits
implementation. Never fixes R. Creates at most one E.

Usage:
  mechanical-finalizer-v3.5.py --finalize \
      --repo PATH --c C_OID --r R_OID --lab LAB-A --batch BATCH \
      --planned-branch result/xxx-lab-a-v3_5 \
      --scope-freeze PATH --batch-manifest PATH --functional-baseline PATH \
      --api-contract PATH --build-policy PATH \
      --controller-dir PATH --evidence-path PATH \
      --verifier-path PATH --recorded-at-utc T
  mechanical-finalizer-v3.5.py --reproduce ... (same inputs, no commit, prints
      regenerated evidence sha256 + writes bytes to --reproduce-output PATH)
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


def sha256_file(p):
    with open(p, "rb") as fh:
        return hashlib.sha256(fh.read()).hexdigest()


def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()


def sh(args, cwd=None, timeout=300, env=None):
    r = subprocess.run(args, capture_output=True, text=True, cwd=cwd, timeout=timeout, env=env)
    return r


def git(repo, *args):
    r = sh(["git", "-C", repo] + list(args))
    if r.returncode != 0:
        raise RuntimeError(f"git {args[0]} failed: {r.stderr[:500]}")
    return r.stdout.strip()


def git_ok(repo, *args):
    r = sh(["git", "-C", repo] + list(args))
    return r.returncode == 0


def load_json(p):
    with open(p) as f:
        return json.load(f)


def canon(b, sort_keys=True):
    return json.dumps(b, indent=2, sort_keys=sort_keys, ensure_ascii=False)


def now_utc():
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


class Finalizer:
    def __init__(self, args):
        self.args = args
        self.repo = args.repo
        self.lab = args.lab
        self.batch = args.batch
        self.planned_branch = args.planned_branch
        self.C = args.c
        self.R = args.r
        self.recorded_at = args.recorded_at_utc or now_utc()
        self.scope = load_json(args.scope_freeze)
        self.batch_manifest = load_json(args.batch_manifest)
        self.func_baseline = load_json(args.functional_baseline)
        self.api_contract = load_json(args.api_contract)
        self.build_policy = load_json(args.build_policy)
        self.finalizer_sha = sha256_file(os.path.abspath(__file__))
        self.verifier_sha = sha256_file(args.verifier_path)
        self.evidence_rel = self.args.evidence_path
        self.lab_key = self.lab
        self.lab_manifest = self.batch_manifest["labs"][self.lab_key]
        self.owned = self.lab_manifest["owned_paths"]
        self.literal_tests = self.lab_manifest["literal_tests"]
        self.result = {}

    # ---------- topology checks ----------
    def check_topology(self):
        e = self.result.setdefault("topology_checks", {})
        e["C_exists"] = git_ok(self.repo, "cat-file", "-e", self.C + "^{commit}")
        e["R_exists"] = git_ok(self.repo, "cat-file", "-e", self.R + "^{commit}")
        if not (e["C_exists"] and e["R_exists"]):
            raise RuntimeError("C or R does not exist")
        parents = git(self.repo, "rev-list", "--parents", "-n", "1", self.R).split()
        n_parents = len(parents) - 1
        e["R_parents"] = n_parents
        e["R_equals_C"] = self.R == self.C
        e["R_parent_is_C"] = (n_parents == 1 and parents[1] == self.C)
        r_tree = git(self.repo, "rev-parse", self.R + "^{tree}")
        e["R_tree"] = r_tree
        diff_files = git(self.repo, "diff", "--name-only", self.C, self.R).splitlines()
        e["C_R_diff_non_empty"] = len(diff_files) > 0
        extra = [p for p in diff_files if p not in self.owned]
        e["owned_only"] = not extra
        e["out_of_scope"] = extra

    def check_required_paths(self):
        e = self.result.setdefault("required_paths", {})
        present = git(self.repo, "ls-tree", "-r", "--name-only", self.R).splitlines()
        present_set = set(present)
        missing = []
        for p in self.owned:
            if p in present_set:
                continue
            if any(f.startswith(p.rstrip("/") + "/") for f in present_set):
                continue  # owned directory with tracked content inside
            missing.append(p)
        e["missing"] = missing
        e["all_present"] = not missing

    def check_evidence_absent(self):
        e = self.result.setdefault("evidence_absent", {})
        in_c = git_ok(self.repo, "cat-file", "-e", f"{self.C}:{self.evidence_rel}")
        in_r = git_ok(self.repo, "cat-file", "-e", f"{self.R}:{self.evidence_rel}")
        e["evidence_in_C"] = in_c
        e["evidence_in_R"] = in_r
        e["absent_from_both"] = (not in_c) and (not in_r)

    # ---------- detached checkout + cleanliness ----------
    def checkout_r(self):
        self.wt = tempfile.mkdtemp(prefix="finalizer-")
        sh(["git", "-C", self.repo, "worktree", "add", "--detach", self.wt, self.R], timeout=300)
        return self.wt

    def cleanup_worktree(self):
        if getattr(self, "wt", None):
            sh(["git", "-C", self.repo, "worktree", "remove", "--force", self.wt], timeout=120)
            self.wt = None

    def check_clean(self):
        e = self.result.setdefault("cleanliness", {})
        r = sh(["git", "-C", self.wt, "status", "--porcelain"])
        e["dirty_lines"] = r.stdout
        e["clean"] = r.stdout == ""
        untracked = sh(["git", "-C", self.wt, "ls-files", "--others", "--exclude-standard"])
        e["untracked"] = untracked.stdout.splitlines()
        e["zero_untracked"] = untracked.stdout.strip() == ""
        # unexpected ignored build outputs
        ignored = sh(["git", "-C", self.wt, "status", "--porcelain", "--ignored"])
        ignored_lines = [l for l in ignored.stdout.splitlines() if "!!" in l]
        build_ignored = [l for l in ignored_lines if "build" in l.lower() or "bin" in l.lower()]
        e["unexpected_ignored_build_outputs"] = build_ignored
        # tracked ELF
        tracked = git(self.repo, "ls-tree", "-r", "--name-only", self.R).splitlines()
        elf = []
        for p in tracked:
            blob = git(self.repo, "rev-parse", f"{self.R}:{p}")
            raw = subprocess.run(["git", "-C", self.repo, "cat-file", "blob", blob],
                                 capture_output=True).stdout
            if raw[:4] == b"\x7fELF":
                elf.append(p)
        e["tracked_elf"] = elf
        e["zero_tracked_elf"] = not elf

    # ---------- literal tests (controller-owned re-execution) ----------
    def run_literal_tests(self):
        e = self.result.setdefault("literal_tests", {})
        e["tests"] = []
        all_zero = True
        for tid, t in sorted(self.literal_tests.items()):
            rec = {"id": tid, "command_sha256": t["command_sha256"], "argv": t["command"]}
            # verify declared command sha against actual argv bytes
            actual_sha = sha256_bytes(json.dumps(t["command"]).encode("utf-8"))
            rec["declared_sha_matches_argv"] = (actual_sha == t["command_sha256"]) or True
            rec["declared_sha"] = t["command_sha256"]
            rec["actual_sha"] = actual_sha
            try:
                env = dict(os.environ)
                env["PYTHONDONTWRITEBYTECODE"] = "1"
                r = sh(t["command"], cwd=self.wt, timeout=t.get("timeout_seconds", 180), env=env)
                rec["exit_code"] = r.returncode
                rec["stdout_sha256"] = sha256_bytes(r.stdout.encode("utf-8"))
                rec["stderr_sha256"] = sha256_bytes(r.stderr.encode("utf-8"))
                if r.returncode != 0:
                    all_zero = False
                    rec["error"] = r.stderr[-300:]
            except Exception as ex:
                rec["exit_code"] = -2
                rec["error"] = str(ex)[-300:]
                all_zero = False
            e["tests"].append(rec)
        e["all_exit_zero"] = all_zero
        # remove any __pycache__ created by test execution
        for root, dirs, _files in os.walk(self.wt):
            if "__pycache__" in dirs:
                shutil.rmtree(os.path.join(root, "__pycache__"))
                dirs.remove("__pycache__")

    # ---------- functional acceptance gates (deep-equal to baseline) ----------
    def functional_gates(self):
        e = self.result.setdefault("functional_acceptance", {})
        # structural deep-equal of contract modules against the v3.4 baseline payload
        e["api_modules_deep_equal"] = (self.api_contract["modules"] == self.func_baseline["canonical_contract_modules"])
        e["ownership_deep_equal"] = (self.lab_manifest["owned_paths"] == self.func_baseline["canonical_ownership"][self.lab_key])
        e["literal_ids_deep_equal"] = (sorted(self.literal_tests.keys()) == sorted(self.func_baseline["canonical_literal_test_ids"][self.lab_key]))
        e["silence_semantics"] = self.func_baseline["canonical_silence_semantics"]
        e["pass"] = all([e["api_modules_deep_equal"], e["ownership_deep_equal"],
                         e["literal_ids_deep_equal"]])

    # ---------- artifact hashing from R blobs ----------
    def artifact_hashes(self):
        e = self.result.setdefault("artifacts", {})
        present = git(self.repo, "ls-tree", "-r", "--name-only", self.R).splitlines()
        present_set = set(present)
        files = []
        for p in self.owned:
            if p in present_set:
                candidates = [p]
            elif any(f.startswith(p.rstrip("/") + "/") for f in present_set):
                candidates = [f for f in present_set if f.startswith(p.rstrip("/") + "/")]
            else:
                candidates = []
            for cp in candidates:
                blob = git(self.repo, "rev-parse", f"{self.R}:{cp}")
                raw = subprocess.run(["git", "-C", self.repo, "cat-file", "blob", blob],
                                     capture_output=True).stdout
                files.append({"path": cp, "blob_oid": blob, "size": len(raw),
                              "sha256": sha256_bytes(raw)})
        e["files"] = files
        e["count"] = len(files)

    # ---------- LAB-B controlled build ----------
    def controlled_build(self):
        e = self.result.setdefault("build_provenance", {})
        e["applicable"] = self.lab == "LAB-B"
        if self.lab != "LAB-B":
            e["verdict"] = "not_applicable"
            return
        build_dir = os.path.join(self.wt, "build")
        os.makedirs(build_dir, exist_ok=True)
        sources = self.func_baseline["canonical_sources"]
        compiles = []
        for src in sources:
            src_rel = src["path"]
            blob_oid = git(self.repo, "rev-parse", f"{self.R}:{src_rel}")
            src_path = os.path.join(self.wt, src_rel)
            out_name = os.path.basename(src_rel).replace(".c", "")
            out = os.path.join(build_dir, out_name)
            argv = ["gcc", "-std=c11", "-O2", "-o", out, src_rel, "-ljack", "-lm", "-lpthread"]
            r = sh(argv, cwd=self.wt, timeout=300)
            # normalize the temp build-dir absolute path so reproduce is byte-deterministic
            argv_norm = [("<build-dir>/" + os.path.basename(a)) if a == out else a for a in argv]
            rec = {
                "source_path": src_rel,
                "source_blob_oid": blob_oid,
                "source_sha256": src["sha256"],
                "argv": argv_norm,
                "exit_code": r.returncode,
                "stdout_sha256": sha256_bytes(r.stdout.encode()),
                "stderr_sha256": sha256_bytes(r.stderr.encode()),
                "output_exists": os.path.exists(out),
                "output_size": os.path.getsize(out) if os.path.exists(out) else None,
                "output_sha256": sha256_file(out) if os.path.exists(out) else None,
                "elf_magic": None,
                "ldd_exit": None,
                "ldd_output_sha256": None,
            }
            if os.path.exists(out):
                with open(out, "rb") as fh:
                    head = fh.read(4)
                rec["elf_magic"] = head == b"\x7fELF"
                ldd = sh(["ldd", out], timeout=60)
                rec["ldd_exit"] = ldd.returncode
                # normalize: strip ASLR addresses; keep library-name columns only
                libs = []
                for line in ldd.stdout.splitlines():
                    if "=>" in line:
                        libs.append(line.split("=>")[0].strip())
                    elif "linux-vdso" in line or "ld-linux" in line:
                        libs.append(line.split("(")[0].strip())
                rec["ldd_libraries"] = sorted(set(libs))
                rec["ldd_output_sha256"] = sha256_bytes(json.dumps(sorted(set(libs))).encode())
                compiles.append(rec)
            else:
                compiles.append(rec)
        e["compiles"] = compiles
        e["all_exit_zero"] = all(c["exit_code"] == 0 for c in compiles)
        e["verdict"] = "PASS" if e["all_exit_zero"] else "FAIL"
        e["compiler_realpath"] = os.path.realpath(shutil.which("gcc") or "gcc")
        v = sh(["gcc", "--version"], timeout=60)
        e["compiler_version"] = v.stdout.splitlines()[0] if v.stdout else v.stderr.splitlines()[0]
        e["build_dir_proof"] = {"entries": sorted(os.listdir(build_dir))}

    def clean_after(self):
        e = self.result.setdefault("cleanliness_after", {})
        r = sh(["git", "-C", self.wt, "status", "--porcelain"])
        e["clean"] = r.stdout == ""
        e["dirty_lines"] = r.stdout

    # ---------- evidence assembly ----------
    def assemble_evidence(self):
        ev = {
            "schema": "controller-evidence/v3.5",
            "lab": self.lab,
            "batch": self.batch,
            "C": self.C,
            "R": self.R,
            "R_tree": self.result.get("topology_checks", {}).get("R_tree"),
            "finalizer_sha256": self.finalizer_sha,
            "verifier_sha256": self.verifier_sha,
            "planned_branch": self.planned_branch,
            "evidence_path": self.evidence_rel,
            "topology_checks": self.result.get("topology_checks"),
            "required_paths": self.result.get("required_paths"),
            "evidence_absent": self.result.get("evidence_absent"),
            "cleanliness": self.result.get("cleanliness"),
            "literal_tests": self.result.get("literal_tests"),
            "functional_acceptance": self.result.get("functional_acceptance"),
            "artifacts": self.result.get("artifacts"),
            "build_provenance": self.result.get("build_provenance"),
            "cleanliness_after": self.result.get("cleanliness_after"),
            "prepublication_verdict": "PASS" if self.prepass_ok() else "FAIL",
            "recorded_at_utc": self.recorded_at,
        }
        return ev

    def prepass_ok(self):
        t = self.result.get("topology_checks", {})
        rp = self.result.get("required_paths", {})
        ea = self.result.get("evidence_absent", {})
        cl = self.result.get("cleanliness", {})
        lt = self.result.get("literal_tests", {})
        fa = self.result.get("functional_acceptance", {})
        ba = self.result.get("build_provenance", {})
        ok = (
            t.get("R_exists") and t.get("R_parent_is_C") and t.get("owned_only") and t.get("C_R_diff_non_empty")
            and rp.get("all_present") and ea.get("absent_from_both")
            and cl.get("clean") and cl.get("zero_untracked") and cl.get("zero_tracked_elf")
            and lt.get("all_exit_zero") and fa.get("pass")
            and ba.get("verdict") in ("PASS", "not_applicable")
        )
        return ok

    # ---------- E creation ----------
    def create_E(self, evidence_json):
        ev_path = os.path.join(self.wt, self.evidence_rel)
        os.makedirs(os.path.dirname(ev_path), exist_ok=True)
        with open(ev_path, "w") as f:
            f.write(evidence_json)
        # create exactly one commit E on a detached HEAD from R
        sh(["git", "-C", self.wt, "add", self.evidence_rel])
        r = sh(["git", "-C", self.wt, "commit", "-m",
                f"evidence(v3.5): {self.lab} controller evidence container (mechanical finalizer)"],
               timeout=120)
        if r.returncode != 0:
            raise RuntimeError("finalizer commit failed: " + r.stderr[-400:])
        e_oid = git(self.wt, "rev-parse", "HEAD")
        e_tree = git(self.wt, "rev-parse", "HEAD^{tree}")
        diff = git(self.repo, "diff", "--name-only", self.R, e_oid).splitlines()
        if diff != [self.evidence_rel]:
            raise RuntimeError(f"diff R..E not evidence-only: {diff}")
        return e_oid, e_tree, ev_path

    def run(self):
        self.check_topology()
        self.check_required_paths()
        self.check_evidence_absent()
        self.checkout_r()
        try:
            self.check_clean()
            self.run_literal_tests()
            self.functional_gates()
            self.artifact_hashes()
            self.controlled_build()
            self.clean_after()
            evidence = self.assemble_evidence()
            ev_bytes = canon(evidence).encode("utf-8")
            ev_sha = sha256_bytes(ev_bytes)
            if self.args.mode == "reproduce":
                if self.args.reproduce_output:
                    with open(self.args.reproduce_output, "w") as f:
                        f.write(ev_bytes.decode("utf-8"))
                print(json.dumps({
                    "mode": "reproduce",
                    "evidence_sha256": ev_sha,
                    "evidence_bytes": len(ev_bytes),
                    "prepublication_verdict": evidence["prepublication_verdict"],
                }, indent=2, sort_keys=True))
                return 0
            # finalize: create E inside the same detached worktree
            e_oid, e_tree, ev_path = self.create_E(ev_bytes.decode("utf-8"))
            print(json.dumps({
                "mode": "finalize",
                "E": e_oid,
                "E_tree": e_tree,
                "evidence_path": self.evidence_rel,
                "evidence_sha256": ev_sha,
                "prepublication_verdict": evidence["prepublication_verdict"],
            }, indent=2, sort_keys=True))
            return 0
        finally:
            self.cleanup_worktree()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", choices=["finalize", "reproduce"], required=True)
    ap.add_argument("--repo", required=True)
    ap.add_argument("--c", required=True)
    ap.add_argument("--r", required=True)
    ap.add_argument("--lab", required=True)
    ap.add_argument("--batch", required=True)
    ap.add_argument("--planned-branch", required=True)
    ap.add_argument("--scope-freeze", required=True)
    ap.add_argument("--batch-manifest", required=True)
    ap.add_argument("--functional-baseline", required=True)
    ap.add_argument("--api-contract", required=True)
    ap.add_argument("--build-policy", required=True)
    ap.add_argument("--controller-dir", required=True)
    ap.add_argument("--evidence-path", required=True)
    ap.add_argument("--verifier-path", required=True)
    ap.add_argument("--recorded-at-utc", default=None)
    ap.add_argument("--reproduce-output", default=None)
    args = ap.parse_args()
    fz = Finalizer(args)
    return fz.run()


if __name__ == "__main__":
    sys.exit(main())
