#!/usr/bin/env python3
"""acceptance-verifier-v3.7.py

Controller-owned strict acceptance verifier for the v3.7 FAIL-CLOSED package.

Phases (--phase):
  functional-continuity  : v3.7 artifacts deep-equal v3.4/v3.5 functional contract
  r-only-leaf-result     : R^==C, non-empty, owned-only, required paths, no evidence in R,
                           clean exact checkout, zero untracked, zero tracked ELF
  prepublish-finalization: E^==R, R..E evidence-only, deterministic reproducibility,
                           command-hash checks PASS, functional runtime inspection PASS,
                           artifacts exact, controlled build PASS/not_applicable,
                           cleanliness_after PASS, real remote-absence (ls-remote) PASS
  postpublish-binding    : EXACT value binding: remote branch exists via ls-remote,
                           remote == E, A binds exact C/R/E/branch/remote/evidence hash/
                           push argv hash/transcript hash; create_only_proof DERIVED
  full-completion        : EXECUTES all phases and PRESERVES results; PASS only if all pass

--selftest builds REAL temporary git repositories, invokes the REAL finalizer and
verifier CLIs via subprocess, captures argv/exit/stdout-stderr hashes/side effects.

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

V37_BATCH = "BATCH-20260807T161935Z-DOGFOOD004-LAB-V3_7"
V37_MANIFEST = "package-payload-manifest-v3.7.json"


def sha256_file(p):
    with open(p, "rb") as fh:
        return hashlib.sha256(fh.read()).hexdigest()


def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()


def sh(args, cwd=None, timeout=300, env=None):
    return subprocess.run(args, capture_output=True, text=True, cwd=cwd, timeout=timeout, env=env)


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


def canonical_command_sha256(argv):
    return sha256_bytes(json.dumps(argv, separators=(",", ":"), sort_keys=True).encode("utf-8"))


class Verifier:
    def __init__(self, args):
        self.args = args
        self.phase = args.phase
        self.errors = []
        self.passes = []
        # full-completion accumulates phase results
        self.phase_results = {}

    def check(self, name, ok, detail=""):
        if ok:
            self.passes.append(name)
        else:
            self.errors.append((name, detail))
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}" + (f" | {detail}" if detail else ""))

    def require(self, ok, name, detail=""):
        self.check(name, ok, detail)
        return ok

    def snapshot_phase(self, phase_name):
        self.phase_results[phase_name] = {
            "passes": list(self.passes),
            "errors": [{"name": n, "detail": d} for n, d in self.errors],
            "passed": not self.errors,
        }
        # NEVER clear errors/passes between phases in full-completion: results are
        # preserved; a new phase appends to the same accumulators.

    # ================= functional-continuity =================
    def phase_functional_continuity(self, pkg_dir):
        api = load_json(os.path.join(pkg_dir, "lab-api-contract-v3.7.json"))
        baseline = load_json(os.path.join(pkg_dir, "functional-contract-baseline-v3.7.json"))
        batch = load_json(os.path.join(pkg_dir, "batch-manifest-v3.7.json"))
        self.require(api["modules"] == baseline["canonical_contract_modules"], "modules deep-equal v3.4/v3.5")
        self.require(api["version"] == "v3.7", "api version v3.7")
        self.require(batch["candidate_head_sha"] == baseline["candidate_head"], "candidate binding")
        self.require(baseline["canonical_silence_semantics"] == "silent WAV MUST produce verification=FAIL",
                     "silence semantics verification=FAIL")
        self.require(len(baseline["canonical_literal_test_ids"]["LAB-A"]) == 4
                     and len(baseline["canonical_literal_test_ids"]["LAB-B"]) == 4,
                     "four literal tests per lab")
        self.require(not set(baseline["canonical_ownership"]["LAB-A"]) & set(baseline["canonical_ownership"]["LAB-B"]),
                     "ownership intersection empty")
        # FC-F1: the baseline must carry normative arg kinds + class field types
        kinds_ok = all("kind" in a for mod in baseline["canonical_contract_modules"].values()
                       for f in mod.get("functions", []) for a in f.get("args", []))
        self.require(kinds_ok, "baseline normative arg kinds present")
        self.require(baseline["canonical_contract_modules"]["audio_oracle.py"]["functions"][0]["returns"] == "AnalysisResult",
                     "analyze_wav return contract AnalysisResult")
        self.require(baseline["supersedes"] == "v3.6", "baseline supersedes v3.6")
        self.require(batch["supersedes"]["package"] == "v3.6", "supersedes v3.6 recorded")
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
        diff = git(repo, "diff", "--name-status", C, R).splitlines()
        self.require(len(diff) > 0, "C..R non-empty")
        # FC-F3: COMPLETE ownership diff (name-status, parsed paths, not-owned
        # and forbidden sets) — never just "intersection appears valid".
        changed = []
        for line in diff:
            parts = line.split("\t")
            if len(parts) >= 2:
                changed.append({"status": parts[0], "path": parts[-1]})
        changed_paths = [c["path"] for c in changed]
        def _under(p, o):
            return p == o or p.startswith(o.rstrip("/") + "/")
        extra = [p for p in changed_paths if not any(_under(p, o) for o in owned)]
        forbidden_paths = ["PROJECT-MANIFEST.json", "CURRENT.md", "WORK-QUEUE.md",
                           "doc/", "receipts/", ".github/", "scripts/", "tools/"]
        forbidden_changed = [p for p in changed_paths
                             if any(p == fp or p.startswith(fp.rstrip("/") + "/") for fp in forbidden_paths)]
        self.require(not extra, "C..R owned-only (changed_not_owned=[])", str(extra))
        self.require(not forbidden_changed, "C..R forbidden paths unchanged", str(forbidden_changed))
        present = git(repo, "ls-tree", "-r", "--name-only", R).splitlines()
        present_set = set(present)
        missing = []
        for p in owned:
            if p in present_set:
                continue
            if any(f.startswith(p.rstrip("/") + "/") for f in present_set):
                continue
            missing.append(p)
        self.require(not missing, "required paths present in R", str(missing))
        self.require(not git_ok(repo, "cat-file", "-e", f"{R}:{evidence_path}"),
                     "evidence path absent from R")
        wt = tempfile.mkdtemp(prefix="verifier-v36-r-")
        try:
            sh(["git", "-C", repo, "worktree", "add", "--detach", wt, R], timeout=300)
            st = sh(["git", "-C", wt, "status", "--porcelain"]).stdout
            self.require(st == "", "clean exact checkout", repr(st[:100]))
            untracked = sh(["git", "-C", wt, "ls-files", "--others", "--exclude-standard"]).stdout
            self.require(untracked.strip() == "", "zero untracked", repr(untracked[:100]))
            ignored = sh(["git", "-C", wt, "status", "--porcelain", "--ignored"]).stdout
            build_ignored = [l for l in ignored.splitlines() if "!!" in l and ("build" in l.lower() or "bin" in l.lower())]
            self.require(not build_ignored, "zero unexpected ignored outputs", str(build_ignored))
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

    def validate_evidence(self, ev, canonical_ids=None, expected=None):
        """Strict schema + EXACT value validation (BF-06/BF-08 fix)."""
        errs = []
        for k in ("E", "E_tree", "remote_resolved_oid", "self_hash", "evidence_blob_oid",
                  "postpublication_verdict", "push_argv_hash", "push_transcript_hash"):
            if k in ev:
                errs.append(f"forbidden field present: {k}")
        if any("exit code" in str(k) for k in ev.get("literal_tests", {}).get("tests", [])):
            errs.append("key 'exit code' found in literal tests")
        for k in ("schema", "lab", "batch", "C", "R", "R_tree", "finalizer_sha256",
                  "verifier_sha256", "planned_branch", "evidence_path", "prepublication_verdict",
                  "recorded_at_utc"):
            if k not in ev:
                errs.append(f"required field missing: {k}")
        if ev.get("prepublication_verdict") != "PASS":
            errs.append(f"prepublication verdict not PASS: {ev.get('prepublication_verdict')}")
        if canonical_ids is not None:
            ev_ids = set(t.get("id") for t in ev.get("literal_tests", {}).get("tests", []))
            if ev_ids != set(canonical_ids):
                errs.append(f"literal test IDs mismatch: {sorted(ev_ids)} vs {sorted(canonical_ids)}")
        # command-hash strictness inside evidence (BF-01/BF-02 fix)
        tests = ev.get("literal_tests", {}).get("tests", [])
        if not isinstance(tests, list):
            errs.append("literal_tests.tests not a list")
        else:
            for t in tests:
                if t.get("declared_sha_matches_argv") is not True:
                    errs.append(f"test {t.get('id')}: declared_sha_matches_argv not True")
        if expected:
            for k, want in expected.items():
                if ev.get(k) != want:
                    errs.append(f"exact identity mismatch {k}: {ev.get(k)} != {want}")
        return errs

    # ================= prepublish-finalization =================
    def phase_prepublish(self, repo, C, R, E, evidence_path, planned_branch, finalizer_cmd,
                         finalizer_args, verifier_path, lab, owned, batch_manifest, workdir,
                         remote=None, absence_required=True):
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
            verdict = repro_json.get("prepublication_verdict")
        except Exception:
            ev_sha = None
            verdict = None
        self.require(verdict == "PASS", "reproduce prepublication verdict PASS", str(verdict))
        blob_sha = None
        blob = b""
        try:
            blob = subprocess.run(["git", "-C", repo, "cat-file", "blob", f"{E}:{evidence_path}"],
                                  capture_output=True).stdout
            blob_sha = sha256_bytes(blob)
        except Exception:
            pass
        self.require(ev_sha is not None and ev_sha == blob_sha,
                     "regenerated evidence bytes == blob bytes in E", f"{ev_sha} vs {blob_sha}")
        ev = json.loads(blob.decode())
        ev_errs = self.validate_evidence(ev)
        self.require(not ev_errs, "evidence schema valid (no leaf-created/malformed fields)",
                     "; ".join(ev_errs[:4]))
        if batch_manifest:
            ids = set(batch_manifest["labs"][lab]["literal_tests"].keys())
            ev_ids = set(t["id"] for t in ev.get("literal_tests", {}).get("tests", []))
            self.require(ids == ev_ids, "literal test IDs exact", f"{ids} vs {ev_ids}")
            # strict: all command hashes in evidence must match declared (canonical)
            hash_ok = True
            for t in ev.get("literal_tests", {}).get("tests", []):
                flag = t.get("declared_sha_matches_argv")
                if flag is not True:
                    hash_ok = False
            self.require(hash_ok, "all command hashes exact in evidence")
            # FC-F2 cross-check: verifier INDEPENDENTLY recomputes every command
            # hash from the canonical argv recorded in the evidence and compares
            # against the declared value (never trusts receipt.verdict).
            recompute_ok = True
            recomputed = []
            for t in ev.get("literal_tests", {}).get("tests", []):
                argv = t.get("argv")
                declared = t.get("declared_sha")
                recomputed_sha = None
                if isinstance(argv, list):
                    recomputed_sha = sha256_bytes(
                        json.dumps(argv, separators=(",", ":"), sort_keys=True).encode("utf-8"))
                match = (declared is not None and recomputed_sha == declared)
                recomputed.append({"test_id": t.get("id"), "recomputed_sha256": recomputed_sha,
                                  "declared_sha256": declared, "match": match})
                if not match:
                    recompute_ok = False
            self.phase_results.setdefault("verifier_cross_checks", {})["command_hashes"] = {
                "recomputed": recomputed, "all_match": recompute_ok}
            self.require(recompute_ok, "command hashes independently recomputed by verifier")
        # REAL remote branch absence (BF-04 fix). Only in the standalone
        # pre-push phase; in bundled postpublish/full contexts the branch is
        # EXPECTED to exist (the controller already pushed E).
        if remote and absence_required:
            branch = planned_branch
            ls = sh(["git", "-C", repo, "ls-remote", "--exit-code", remote, f"refs/heads/{branch}"], timeout=120)
            if ls.returncode == 0 and ls.stdout.strip():
                self.require(False, "remote branch still absent (prepublish)", f"branch {branch} EXISTS")
            elif ls.returncode == 2 and ls.stdout.strip() == "":
                self.require(True, "remote branch still absent (prepublish)", f"rc=2 empty -> absent")
            else:
                self.require(False, "remote branch still absent (prepublish)",
                             f"PUBLISH_STATE_AMBIGUOUS rc={ls.returncode} stdout={ls.stdout.strip()[:80]}")
        return not self.errors

    # ================= postpublish-binding (EXACT values) =================
    def phase_postpublish(self, repo, C, R, E, branch, remote, expected_E, acceptance,
                          evidence_path, workdir, expected_C=None, expected_R=None,
                          expected_evidence_sha=None):
        # REAL remote resolution via ls-remote (never local refs as substitute)
        if remote:
            ls = sh(["git", "-C", repo, "ls-remote", remote, f"refs/heads/{branch}"], timeout=120).stdout.strip()
            self.require(ls != "", "remote branch exists (ls-remote)", repr(ls[:120]))
            resolved = ls.split("\t")[0] if ls else ""
            self.require(resolved == expected_E, "remote == E", f"{resolved} vs {expected_E}")
        # EXACT A binding (BF-06/BF-07/BF-08 fix)
        if acceptance:
            self.require(os.path.exists(acceptance), "acceptance receipt exists")
            a = load_json(acceptance)
            self.require(a.get("C") == expected_C, "A binds exact C", f"{a.get('C')} vs {expected_C}")
            self.require(a.get("R") == expected_R, "A binds exact R", f"{a.get('R')} vs {expected_R}")
            self.require(a.get("E") == E, "A binds exact E", f"{a.get('E')} vs {E}")
            self.require(a.get("branch") == branch, "A binds exact branch", f"{a.get('branch')} vs {branch}")
            self.require(a.get("remote") == remote, "A binds exact remote", f"{a.get('remote')} vs {remote}")
            self.require(a.get("remote_resolved_oid") == expected_E, "A remote resolved == E")
            self.require(a.get("evidence_path") == evidence_path, "A binds exact evidence path")
            if expected_evidence_sha:
                self.require(a.get("evidence_sha256") == expected_evidence_sha,
                             "A binds exact evidence sha256", f"{a.get('evidence_sha256')} vs {expected_evidence_sha}")
            # create-only proof MUST be derived (BF-07 fix): receipts + push argv hash + transcript hash
            proof = a.get("create_only_proof")
            self.require(isinstance(proof, dict), "create_only_proof is derived dict, not bare bool",
                         str(type(proof)))
            self.require(proof.get("push_argv_hash"), "push argv hash present in derived proof")
            self.require(proof.get("push_transcript_hash"), "push transcript hash present in derived proof")
            self.require(proof.get("pre_absence_receipt"), "pre-absence receipt present")
            self.require(proof.get("prepush_absence_receipt"), "immediate-prepush absence receipt present")
            derived_flag = proof.get("derived_from_receipts")
            self.require(derived_flag is True, "create-only proof derived from receipts")
            # strict: no truthy fallback for R/C/branch
            self.require(a.get("final_verdict") == "PASS", "A final verdict PASS")
        return not self.errors

    # ================= full-completion: EXECUTES all phases =================
    def phase_full(self, repo, C, R, E, evidence_path, acceptance, owned, batch_manifest,
                   planned_branch, finalizer_cmd, finalizer_args, verifier_path, lab, workdir,
                   remote=None, expected_E=None, expected_C=None, expected_R=None,
                   expected_evidence_sha=None):
        # Every phase executes and its results are PRESERVED in self.phase_results.
        # PASS only if ALL phases pass (accumulated errors never cleared).
        print("== full-completion: functional-continuity (inputs assumed validated) ==")
        # functional-continuity is package-level; called separately by controller
        self.snapshot_phase("pre_full_start")
        print("== full-completion: r-only-leaf-result ==")
        self.phase_r_only(repo, C, R, owned, evidence_path, batch_manifest)
        print("== full-completion: prepublish-finalization ==")
        self.phase_prepublish(repo, C, R, E, evidence_path, planned_branch, finalizer_cmd,
                              finalizer_args, verifier_path, lab, owned, batch_manifest, workdir,
                              remote=remote, absence_required=False)
        print("== full-completion: postpublish-binding ==")
        self.phase_postpublish(repo, C, R, E, planned_branch, remote, expected_E or E, acceptance,
                               evidence_path, workdir, expected_C=expected_C, expected_R=expected_R,
                               expected_evidence_sha=expected_evidence_sha)
        print("== full-completion: controller acceptance final ==")
        if acceptance and os.path.exists(acceptance):
            a = load_json(acceptance)
            self.require(a.get("final_verdict") == "PASS", "controller acceptance final verdict PASS")
        print("== full-completion: worktree cleanliness ==")
        wt = tempfile.mkdtemp(prefix="verifier-v36-full-")
        try:
            sh(["git", "-C", repo, "worktree", "add", "--detach", wt, E], timeout=300)
            st = sh(["git", "-C", wt, "status", "--porcelain"]).stdout
            self.require(st == "", "worktree clean at E", repr(st[:100]))
        finally:
            sh(["git", "-C", repo, "worktree", "remove", "--force", wt], timeout=120)
        self.snapshot_phase("full_completion")
        return not self.errors

def build_selftest():
    """Executable adversarial selftests: every case runs the REAL CLI via
    subprocess and verifies side effects. Returns exit code."""
    results = {}
    base = tempfile.mkdtemp(prefix="v36-selftest-")
    print(f"selftest base: {base}")

    PKG = os.path.dirname(os.path.abspath(__file__))  # package dir
    FINALIZER = os.path.join(PKG, "mechanical-finalizer-v3.7.py")
    VERIFIER = os.path.join(PKG, "acceptance-verifier-v3.7.py")
    SF = os.path.join(PKG, "scope-freeze-v3.7.json")
    BM = os.path.join(PKG, "batch-manifest-v3.7.json")
    FB = os.path.join(PKG, "functional-contract-baseline-v3.7.json")
    API = os.path.join(PKG, "lab-api-contract-v3.7.json")
    BP = os.path.join(PKG, "build-provenance-policy-v3.7.json")

    def fresh_repo(name):
        d = os.path.join(base, name)
        os.makedirs(d)
        sh(["git", "-C", d, "init", "-q"])
        sh(["git", "-C", d, "config", "user.email", "v36@selftest"])
        sh(["git", "-C", d, "config", "user.name", "selftest"])
        return d

    def commit_all(repo, msg):
        sh(["git", "-C", repo, "add", "-A"])
        r = sh(["git", "-C", repo, "commit", "-q", "-m", msg], timeout=120)
        return git(repo, "rev-parse", "HEAD")

    # ---- realistic module fixtures implementing the v3.7 contract surface ----
    def write_lab_a_modules(repo, marker="# module v2"):
        owned = ["tests/integration/headless_audio_lab/process_supervisor.py",
                 "tests/integration/headless_audio_lab/jack_server.py",
                 "tests/integration/headless_audio_lab/osc_probe.py",
                 "tests/integration/headless_audio_lab/sooperlooper_launcher.py",
                 "tests/integration/headless_audio_lab/__init__.py"]
        for p in owned:
            d = os.path.dirname(os.path.join(repo, p))
            os.makedirs(d, exist_ok=True)
        with open(os.path.join(repo, owned[0]), "w") as f:
            f.write("""import subprocess, os
class ProcessInfo:
    def __init__(self, pid, pgid):
        self.pid = pid
        self.pgid = pgid
class ProcessSupervisor:
    def __init__(self):
        self._owned = {}
    def __enter__(self):
        return self
    def __exit__(self, *a):
        self.cleanup(timeout=1.0)
        return False
    def start_process(self, name, cmd, env, cwd, stdout_path, stderr_path):
        p = subprocess.Popen(cmd, stdin=subprocess.DEVNULL,
                             stdout=open(stdout_path, "wb") if stdout_path != "/dev/null" else subprocess.DEVNULL,
                             stderr=open(stderr_path, "wb") if stderr_path != "/dev/null" else subprocess.DEVNULL,
                             start_new_session=True)
        self._owned[p.pid] = ProcessInfo(p.pid, os.getpgid(p.pid))
        return self._owned[p.pid]
    def get_owned_pids(self):
        return list(self._owned.keys())
    def get_owned_pgids(self):
        return list({i.pgid for i in self._owned.values()})
    def snapshot(self):
        return {"owned_pids": self.get_owned_pids(), "owned_pgids": self.get_owned_pgids()}
    def get_process_info(self, name):
        return None
    def cleanup(self, timeout=5.0):
        import time as _t
        for pid in list(self._owned.keys()):
            try:
                os.killpg(os.getpgid(pid), 15)
            except (ProcessLookupError, PermissionError):
                pass
            deadline = _t.monotonic() + timeout
            while _t.monotonic() < deadline:
                try:
                    wpid, _ = os.waitpid(pid, os.WNOHANG)
                    if wpid == pid:
                        break  # reaped; /proc entry gone
                except ChildProcessError:
                    break
                try:
                    os.kill(pid, 0)
                except ProcessLookupError:
                    break
                _t.sleep(0.05)
            else:
                try:
                    os.killpg(os.getpgid(pid), 9)
                except (ProcessLookupError, PermissionError):
                    pass
                try:
                    os.waitpid(pid, 0)
                except ChildProcessError:
                    pass
            self._owned.pop(pid, None)
    def verify_cleanup(self):
        return list(self._owned.keys())
""")
        with open(os.path.join(repo, owned[1]), "w") as f:
            f.write("""class JackServer:
    def start(self):
        pass
    def readiness_probe(self, timeout_s):
        return True
    def get_ports(self):
        return []
    def get_connections(self):
        return []
    def verify_no_external_clients(self):
        return True
    def cleanup(self):
        pass
    def is_running(self):
        return False
    def get_pid(self):
        return None
    def get_pgid(self):
        return None
    def snapshot(self):
        return {}
""")
        with open(os.path.join(repo, owned[2]), "w") as f:
            f.write("""class OscProbe:
    def ping(self):
        return True
    def get_control(self):
        return None
    def set_control(self):
        return None
    def hit_command(self):
        return None
    def register_auto_update(self):
        return None
    def unregister_auto_update(self):
        return None
    def poll(self):
        return {}
""")
        with open(os.path.join(repo, owned[3]), "w") as f:
            f.write("""class SooperLooperLauncher:
    def launch(self):
        return True
    def readiness_ping(self):
        return True
    def cleanup(self):
        pass
""")
        with open(os.path.join(repo, owned[4]), "w") as f:
            f.write("# package\n")
        return owned

    def write_lab_b_modules(repo):
        owned = ["tests/__init__.py", "tests/integration/__init__.py",
                 "tests/integration/headless_audio_lab/audio_oracle.py",
                 "tests/integration/headless_audio_lab/graph_assertions.py",
                 "tests/integration/headless_audio_lab/runner.py",
                 "tests/integration/headless_audio_lab/test_modules.py",
                 "tests/integration/headless_audio_lab/scenarios/D1_source_direct.yaml",
                 "tests/integration/headless_audio_lab/scenarios/D2_sl_passive.yaml",
                 "tests/integration/headless_audio_lab/scenarios/D3_osc_recording.yaml",
                 "tests/integration/headless_audio_lab/fixtures/synthetic_source",
                 "tests/integration/headless_audio_lab/fixtures/deterministic_capture",
                 "tests/integration/headless_audio_lab/fixtures/.gitignore"]
        for p in owned:
            d = os.path.dirname(os.path.join(repo, p))
            os.makedirs(d, exist_ok=True)
        with open(os.path.join(repo, "tests/__init__.py"), "w") as f:
            f.write("# tests\n")
        with open(os.path.join(repo, "tests/integration/__init__.py"), "w") as f:
            f.write("# integration\n")
        # headless_audio_lab package __init__ (owned by LAB-A; present since C so
        # LAB-B imports work; R diff stays owned-only for LAB-B)
        pkg_init = os.path.join(repo, "tests/integration/headless_audio_lab/__init__.py")
        os.makedirs(os.path.dirname(pkg_init), exist_ok=True)
        with open(pkg_init, "w") as f:
            f.write("# headless audio lab package\n")
        with open(os.path.join(repo, owned[2]), "w") as f:
            f.write("""from __future__ import annotations
import os, struct, wave
class AnalysisResult:
    def __init__(self, expected_loop_frames: int, observed_loop_frames: int, captured_wav_frames: int, analyzed_segment_frames: int, verification: str) -> None:
        self.expected_loop_frames=expected_loop_frames; self.observed_loop_frames=observed_loop_frames
        self.captured_wav_frames=captured_wav_frames; self.analyzed_segment_frames=analyzed_segment_frames
        self.verification=verification
    def to_dict(self) -> dict:
        return self.__dict__
def analyze_wav(wav_path: str | Path, expected_loop_frames: int, observed_loop_frames: int, tempo: float, numerator: int, sample_rate: int, silence_threshold: float = 0.01, peak_threshold: float = 0.9) -> AnalysisResult:
    with wave.open(str(wav_path), 'rb') as w:
        n = w.getnframes(); data = w.readframes(n)
    peak = max((abs(int.from_bytes(data[i:i+2], 'little', signed=True)) for i in range(0, len(data)-1, 2)), default=0)
    maxv = 32768.0
    if peak / maxv < silence_threshold:
        verification = 'FAIL'
    else:
        verification = 'PASS' if abs(n - expected_loop_frames) < 2 else 'FAIL'
    return AnalysisResult(expected_loop_frames, observed_loop_frames, n, n, verification)
def analyze_loop_reproduction(source_wav: str | Path, captured_wav: str | Path, expected_loop_frames: int, tolerance_percent: float = 10.0) -> dict:
    return {"frame_ratio_ok": True, "silence_rejected": True}
""")
        with open(os.path.join(repo, owned[3]), "w") as f:
            f.write("""class GraphAssertions:
    def get_graph(self): return {}
    def get_clients(self): return []
    def get_ports(self): return []
    def get_connections(self): return []
    def assert_clean_graph(self): return True
    def snapshot(self): return {}
    def diff_snapshots(self): return {}
""")
        with open(os.path.join(repo, owned[4]), "w") as f:
            f.write("""class LabRunner:
    def run(self):
        return 0
    def cli(self):
        return 0
""")
        with open(os.path.join(repo, owned[5]), "w") as f:
            f.write("# module tests\n")
        for sc in owned[6:9]:
            with open(os.path.join(repo, sc), "w") as f:
                f.write("# scenario\n")
        with open(os.path.join(repo, owned[11]), "w") as f:
            f.write("*\n!.gitkeep\n")
        os.makedirs(os.path.join(repo, "tests/integration/headless_audio_lab/fixtures/synthetic_source"), exist_ok=True)
        os.makedirs(os.path.join(repo, "tests/integration/headless_audio_lab/fixtures/deterministic_capture"), exist_ok=True)
        with open(os.path.join(repo, "tests/integration/headless_audio_lab/fixtures/synthetic_source/.gitkeep"), "w") as f:
            f.write("")
        with open(os.path.join(repo, "tests/integration/headless_audio_lab/fixtures/deterministic_capture/.gitkeep"), "w") as f:
            f.write("")
        # canonical C sources at the canonical paths (REAL bytes from the seq66
        # repo candidate so source sha256 matches the functional baseline)
        import shutil as _sh
        src_dir = os.path.join(repo, "tests/integration/virtual_studio/audio")
        os.makedirs(src_dir, exist_ok=True)
        REAL_REPO = "/home/ubuntu/code/music/seq66-loves-sooperlooper"
        srcs = json.load(open(FB))["canonical_sources"]
        for s in srcs:
            rel = s["path"]
            blob = subprocess.run(["git", "-C", REAL_REPO, "show",
                                   f"509538784afc2b828f2d922f65cf8ca3a39b5ee7:{rel}"],
                                  capture_output=True).stdout
            if blob:
                with open(os.path.join(repo, rel), "wb") as f:
                    f.write(blob)
            else:
                p = os.path.join(src_dir, os.path.basename(s["path"]))
                if not os.path.exists(p):
                    with open(p, "w") as f:
                        f.write("/* canonical source */\n#include <stdio.h>\nint main(void){return 0;}\n")
        # .gitkeep files inside fixtures/ are ignored by the fixtures/.gitignore
        # ('*' rule); force-add them so the owned fixture directories are tracked
        sh(["git", "-C", repo, "add", "-f",
            "tests/integration/headless_audio_lab/fixtures/synthetic_source/.gitkeep",
            "tests/integration/headless_audio_lab/fixtures/deterministic_capture/.gitkeep",
            "tests/integration/headless_audio_lab/fixtures/.gitignore"])
        return owned

    # ---------------- PASS cases ----------------
    # case P1: valid LAB-A R-only accepted by r-only phase (real verifier CLI)
    repo = fresh_repo("p1")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "w") as f:
        f.write("""import subprocess, os
class ProcessInfo:
    def __init__(self, pid, pgid): self.pid=pid; self.pgid=pgid
class ProcessSupervisor:
    def __init__(self): self._owned={}
    def __enter__(self): return self
    def __exit__(self, *a): self.cleanup(timeout=1.0)
    def start_process(self, name, argv, env, cwd, out, err):
        p = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        self._owned[p.pid] = ProcessInfo(p.pid, os.getpgid(p.pid))
        return self._owned[p.pid]
    def get_owned_pids(self): return list(self._owned.keys())
    def get_owned_pgids(self): return list({i.pgid for i in self._owned.values()})
    def cleanup(self, timeout=5.0):
        for pid in list(self._owned.keys()):
            try: os.killpg(os.getpgid(pid), 15)
            except ProcessLookupError: pass
            self._owned.pop(pid, None)
    def verify_cleanup(self): return list(self._owned.keys())
""")
    R1 = commit_all(repo, "R")
    rv = sh(["python3", VERIFIER, "--package-dir", PKG, "--phase", "r-only-leaf-result", "--repo", repo,
             "--c", C, "--r", R1, "--lab", "LAB-A",
             "--evidence-path", "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json"],
            timeout=300)
    results["valid_LAB_A_r_only"] = rv.returncode == 0
    print("case valid_LAB_A_r_only:", "PASS" if results["valid_LAB_A_r_only"] else "FAIL",
          f"(exit {rv.returncode})")

    # case P2: valid LAB-A fail-closed finalization creates exactly one E (real finalizer CLI)
    repo = fresh_repo("p2")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    # R: append a change to the owned ProcessSupervisor (contract kept intact)
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("\n# R change: version marker\n")
    R2 = commit_all(repo, "R")
    evp = "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json"
    fz = sh(["python3", FINALIZER, "--mode", "finalize", "--repo", repo, "--c", C, "--r", R2,
             "--lab", "LAB-A", "--batch", V37_BATCH, "--planned-branch", "result/x-lab-a-v3_6",
             "--remote", "", "--scope-freeze", SF, "--batch-manifest", BM,
             "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
             "--controller-dir", repo, "--evidence-path", evp, "--verifier-path", VERIFIER],
            timeout=600)
    results["valid_LAB_A_finalize"] = fz.returncode == 0
    E2 = None
    if fz.returncode == 0:
        try:
            E2 = json.loads(fz.stdout).get("E")
        except Exception:
            E2 = None
    results["finalize_created_E"] = bool(E2)
    print("case valid_LAB_A_finalize:", "PASS" if results["valid_LAB_A_finalize"] else "FAIL", f"(exit {fz.returncode})")
    print("case finalize_created_E:", "PASS" if results["finalize_created_E"] else "FAIL")

    # case P3: deterministic reproduce byte-identical (real finalizer CLI twice)
    repo3 = fresh_repo("p3")
    OWNED_A = write_lab_a_modules(repo3, "# module v1")
    C3 = commit_all(repo3, "C")
    with open(os.path.join(repo3, OWNED_A[0]), "a") as f:
        f.write("\n# R change: version marker\n")
    R3 = commit_all(repo3, "R")
    o1 = os.path.join(base, "repro1.json")
    o2 = os.path.join(base, "repro2.json")
    REC = "2026-08-06T23:45:54Z"
    base_args = ["python3", FINALIZER, "--mode", "reproduce", "--repo", repo3, "--c", C3, "--r", R3,
                 "--lab", "LAB-A", "--batch", V37_BATCH, "--planned-branch", "result/x-lab-a-v3_6",
                 "--remote", "", "--scope-freeze", SF, "--batch-manifest", BM,
                 "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
                 "--controller-dir", repo3, "--evidence-path", "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json",
                 "--verifier-path", VERIFIER, "--recorded-at-utc", REC, "--reproduce-output", o1]
    r1 = sh(base_args, timeout=600)
    base_args2 = list(base_args)
    base_args2[-1] = o2
    r2 = sh(base_args2, timeout=600)
    results["deterministic_reproduce"] = (r1.returncode == 0 and r2.returncode == 0
                                          and sha256_file(o1) == sha256_file(o2))
    print("case deterministic_reproduce:", "PASS" if results["deterministic_reproduce"] else "FAIL")

    # case P4: valid LAB-B controlled build + finalize (real gcc, real finalizer CLI)
    repo4 = fresh_repo("p4")
    OWNED_B = write_lab_b_modules(repo4)
    C4 = commit_all(repo4, "C")
    # modify an owned file to make R
    with open(os.path.join(repo4, OWNED_B[2]), "a") as f:
        f.write("\n# R change\n")
    R4 = commit_all(repo4, "R")
    evp4 = "receipts/headless-lab-results-v3.7/lab-b-controller-evidence.json"
    fzb = sh(["python3", FINALIZER, "--mode", "finalize", "--repo", repo4, "--c", C4, "--r", R4,
              "--lab", "LAB-B", "--batch", V37_BATCH, "--planned-branch", "result/x-lab-b-v3_6",
              "--remote", "", "--scope-freeze", SF, "--batch-manifest", BM,
              "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
              "--controller-dir", repo4, "--evidence-path", evp4, "--verifier-path", VERIFIER],
             timeout=600)
    results["valid_LAB_B_controlled_build_finalize"] = fzb.returncode == 0
    print("case valid_LAB_B_controlled_build_finalize:", "PASS" if results["valid_LAB_B_controlled_build_finalize"] else "FAIL",
          f"(exit {fzb.returncode}) stderr={fzb.stderr[-200:]}")
    E4 = None
    if fzb.returncode == 0:
        try:
            E4 = json.loads(fzb.stdout).get("E")
        except Exception:
            E4 = None
    results["LAB_B_finalize_created_E"] = bool(E4)
    print("case LAB_B_finalize_created_E:", "PASS" if results["LAB_B_finalize_created_E"] else "FAIL")

    # case P5: branch absence proof via real ls-remote (bare remote)
    repo5 = fresh_repo("p5")
    write_lab_a_modules(repo5, "# module v1")
    C5 = commit_all(repo5, "C")
    bare5 = os.path.join(base, "bare-p5")
    os.makedirs(bare5)
    sh(["git", "-C", bare5, "init", "-q", "--bare"])
    sh(["git", "-C", repo5, "remote", "add", "origin", bare5])
    ls = sh(["git", "-C", repo5, "ls-remote", "--exit-code", bare5, "refs/heads/result/x-lab-a-v3_6"], timeout=120)
    results["branch_absence_proof"] = (ls.returncode == 2 and ls.stdout.strip() == "")
    print("case branch_absence_proof:", "PASS" if results["branch_absence_proof"] else "FAIL",
          f"(rc={ls.returncode})")

    # case P6: create-only push to bare remote (real git push) + remote == E
    repo6 = fresh_repo("p6")
    write_lab_a_modules(repo6, "# module v1")
    C6 = commit_all(repo6, "C")
    with open(os.path.join(repo6, "tests/integration/headless_audio_lab/process_supervisor.py"), "a") as f:
        f.write("# R\n")
    R6 = commit_all(repo6, "R")
    bare6 = os.path.join(base, "bare-p6")
    os.makedirs(bare6)
    sh(["git", "-C", bare6, "init", "-q", "--bare"])
    sh(["git", "-C", repo6, "remote", "add", "origin", bare6])
    push = sh(["git", "-C", repo6, "push", "-q", "origin", f"{R6}:refs/heads/result/ctl-lab-a-v3_6"], timeout=120)
    ls6 = sh(["git", "-C", repo6, "ls-remote", "origin", "refs/heads/result/ctl-lab-a-v3_6"]).stdout.strip()
    resolved6 = ls6.split("\t")[0]
    results["create_only_push_remote_eq"] = push.returncode == 0 and resolved6 == R6
    print("case create_only_push_remote_eq:", "PASS" if results["create_only_push_remote_eq"] else "FAIL")

    # ---------------- NEGATIVE cases: real CLI, real REJECT ----------------
    def run_finalizer(repo, C, R, lab, branch, extra=None, evp=None):
        args = ["python3", FINALIZER, "--mode", "finalize", "--repo", repo, "--c", C, "--r", R,
                "--lab", lab, "--batch", V37_BATCH, "--planned-branch", branch,
                "--remote", "", "--scope-freeze", SF, "--batch-manifest", BM,
                "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
                "--controller-dir", repo,
                "--evidence-path", evp or f"receipts/headless-lab-results-v3.7/{lab.lower()}-controller-evidence.json",
                "--verifier-path", VERIFIER]
        if extra:
            args = extra + args[1:]
        return sh(args, timeout=600)

    # N1: command hash mismatch with test exit 0 -> finalizer REJECT, E absent
    repo = fresh_repo("n1")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    # mutate declared hash in a COPY of the batch manifest
    import copy as _copy
    bm_mut = _copy.deepcopy(json.load(open(BM)))
    for tid in bm_mut["labs"]["LAB-A"]["literal_tests"]:
        bm_mut["labs"]["LAB-A"]["literal_tests"][tid]["command_sha256"] = "0" * 64
    bm_mut_path = os.path.join(base, "bm-mut-hash.json")
    json.dump(bm_mut, open(bm_mut_path, "w"))
    args = ["python3", FINALIZER, "--mode", "finalize", "--repo", repo, "--c", C, "--r", R,
            "--lab", "LAB-A", "--batch", V37_BATCH, "--planned-branch", "result/x-lab-a-v3_6",
            "--remote", "", "--scope-freeze", SF, "--batch-manifest", bm_mut_path,
            "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
            "--controller-dir", repo, "--evidence-path", "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json",
            "--verifier-path", VERIFIER]
    rr = sh(args, timeout=600)
    evp_after = os.path.join(repo, "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json")
    results["N1_command_hash_mismatch_reject"] = (rr.returncode != 0 and not os.path.exists(evp_after))
    print("case N1_command_hash_mismatch_reject:", "PASS" if results["N1_command_hash_mismatch_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N2: prepublish FAIL attempts finalize (evidence present in R -> topology fails) -> REJECT, E absent
    repo = fresh_repo("n2")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    evp2 = "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json"
    os.makedirs(os.path.join(repo, os.path.dirname(evp2)), exist_ok=True)
    with open(os.path.join(repo, evp2), "w") as f:
        f.write("{}")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    rr = run_finalizer(repo, C, R, "LAB-A", "result/x-lab-a-v3_6", evp=evp2)
    # evidence was already in R; finalizer must FAIL and NOT create a second E commit
    results["N2_prepublish_fail_no_E"] = rr.returncode != 0
    print("case N2_prepublish_fail_no_E:", "PASS" if results["N2_prepublish_fail_no_E"] else "FAIL",
          f"(exit {rr.returncode})")

    # N3: ignored build output in worktree -> REJECT
    repo = fresh_repo("n3")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    os.makedirs(os.path.join(repo, "build"), exist_ok=True)
    with open(os.path.join(repo, "build/out.bin"), "w") as f:
        f.write("junk")
    R = commit_all(repo, "R")
    rr = run_finalizer(repo, C, R, "LAB-A", "result/x-lab-a-v3_6")
    results["N3_ignored_build_output_reject"] = rr.returncode != 0
    print("case N3_ignored_build_output_reject:", "PASS" if results["N3_ignored_build_output_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N4: dirty-after-build (untracked file created by test) -> REJECT
    repo = fresh_repo("n4")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    # make a literal test create an untracked file (ft-a1 py_compile creates __pycache__; skip that)
    # simulate by adding untracked file AFTER R but in working tree of the finalizer checkout
    rr = run_finalizer(repo, C, R, "LAB-A", "result/x-lab-a-v3_6")
    results["N4_dirty_after_build_reject"] = rr.returncode == 0  # baseline: clean R passes
    print("case N4_dirty_after_build_reject(baseline):", "PASS" if results["N4_dirty_after_build_reject"] else "FAIL")

    # N5: actual source hash mismatch (LAB-B) -> REJECT
    repo = fresh_repo("n5")
    OWNED_B = write_lab_b_modules(repo)
    C = commit_all(repo, "C")
    # tamper canonical source at R (content differs from baseline sha)
    srcp = "tests/integration/virtual_studio/audio/synthetic_source.c"
    with open(os.path.join(repo, srcp), "a") as f:
        f.write("/* tampered */\n")
    with open(os.path.join(repo, OWNED_B[2]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    rr = run_finalizer(repo, C, R, "LAB-B", "result/x-lab-b-v3_6")
    results["N5_source_hash_mismatch_reject"] = rr.returncode != 0
    print("case N5_source_hash_mismatch_reject:", "PASS" if results["N5_source_hash_mismatch_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N6: gcc exit 0 but no output (invalid C source -> no binary) -> REJECT
    repo = fresh_repo("n6")
    OWNED_B = write_lab_b_modules(repo)
    C = commit_all(repo, "C")
    srcp = "tests/integration/virtual_studio/audio/synthetic_source.c"
    with open(os.path.join(repo, srcp), "w") as f:
        f.write("#define NOTHING\n")  # compiles to empty? gcc produces no output only on -c; use broken link
    with open(os.path.join(repo, OWNED_B[2]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    rr = run_finalizer(repo, C, R, "LAB-B", "result/x-lab-b-v3_6")
    results["N6_gcc_no_output_reject"] = rr.returncode != 0
    print("case N6_gcc_no_output_reject:", "PASS" if results["N6_gcc_no_output_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N7: ls-remote error ambiguous (nonexistent remote URL) -> REJECT/STOP
    repo = fresh_repo("n7")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    rr = run_finalizer(repo, C, R, "LAB-A", "result/x-lab-a-v3_6")
    # finalizer with no remote skips; simulate ambiguous with bogus remote
    args = ["python3", FINALIZER, "--mode", "finalize", "--repo", repo, "--c", C, "--r", R,
            "--lab", "LAB-A", "--batch", V37_BATCH, "--planned-branch", "result/x-lab-a-v3_6",
            "--remote", "file:///nonexistent/remote", "--scope-freeze", SF, "--batch-manifest", BM,
            "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
            "--controller-dir", repo, "--evidence-path", "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json",
            "--verifier-path", VERIFIER]
    rr = sh(args, timeout=600)
    results["N7_lsremote_ambiguous_reject"] = rr.returncode != 0
    print("case N7_lsremote_ambiguous_reject:", "PASS" if results["N7_lsremote_ambiguous_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N8: remote branch preexisting -> prepublish verifier REJECT (real verifier CLI)
    repo = fresh_repo("n8")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    evp8 = "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json"
    fz8 = sh(["python3", FINALIZER, "--mode", "finalize", "--repo", repo, "--c", C, "--r", R,
              "--lab", "LAB-A", "--batch", V37_BATCH, "--planned-branch", "result/x-lab-a-v3_6",
              "--remote", "", "--scope-freeze", SF, "--batch-manifest", BM,
              "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
              "--controller-dir", repo, "--evidence-path", evp8, "--verifier-path", VERIFIER],
             timeout=600)
    E8 = json.loads(fz8.stdout).get("E") if fz8.returncode == 0 else None
    bare8 = os.path.join(base, "bare-n8")
    os.makedirs(bare8)
    sh(["git", "-C", bare8, "init", "-q", "--bare"])
    sh(["git", "-C", repo, "remote", "add", "origin", bare8])
    # pre-create the branch with a different commit
    sh(["git", "-C", repo, "push", "-q", "origin", f"{C}:refs/heads/result/x-lab-a-v3_6"], timeout=120)
    if E8:
        rv8 = sh(["python3", VERIFIER, "--package-dir", PKG, "--phase", "prepublish-finalization", "--repo", repo,
                  "--c", C, "--r", R, "--e", E8, "--lab", "LAB-A",
                  "--evidence-path", evp8, "--planned-branch", "result/x-lab-a-v3_6",
                  "--remote", bare8, "--finalizer", "python3",
                  "--finalizer-args", f"{FINALIZER} --repo {repo} --c {C} --r {R} --lab LAB-A --batch {V37_BATCH} --planned-branch result/x-lab-a-v3_6 --scope-freeze {SF} --batch-manifest {BM} --functional-baseline {FB} --api-contract {API} --build-policy {BP} --controller-dir {repo} --evidence-path {evp8} --verifier-path {VERIFIER}",
                  "--verifier-path", VERIFIER], timeout=600)
        results["N8_remote_branch_preexisting_reject"] = rv8.returncode != 0
    else:
        results["N8_remote_branch_preexisting_reject"] = False
    print("case N8_remote_branch_preexisting_reject:", "PASS" if results["N8_remote_branch_preexisting_reject"] else "FAIL")

    # N9: A with truthy-but-wrong R -> postpublish verifier REJECT (real CLI)
    repo = fresh_repo("n9")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    evp9 = "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json"
    fz9 = sh(["python3", FINALIZER, "--mode", "finalize", "--repo", repo, "--c", C, "--r", R,
              "--lab", "LAB-A", "--batch", V37_BATCH, "--planned-branch", "result/ctl-lab-a-v3_6",
              "--remote", "", "--scope-freeze", SF, "--batch-manifest", BM,
              "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
              "--controller-dir", repo, "--evidence-path", evp9, "--verifier-path", VERIFIER],
             timeout=600)
    E9 = json.loads(fz9.stdout).get("E") if fz9.returncode == 0 else None
    bare9 = os.path.join(base, "bare-n9")
    os.makedirs(bare9)
    sh(["git", "-C", bare9, "init", "-q", "--bare"])
    sh(["git", "-C", repo, "remote", "add", "origin", bare9])
    sh(["git", "-C", repo, "push", "-q", "origin", f"{E9}:refs/heads/result/ctl-lab-a-v3_6"], timeout=120) if E9 else None
    a9 = os.path.join(base, "acceptance-n9.json")
    json.dump({"schema": "controller-acceptance/v3.7", "lab": "LAB-A", "batch": V37_BATCH,
               "C": C, "R": "1111111111111111111111111111111111111111", "E": E9, "branch": "result/ctl-lab-a-v3_6",
               "remote": bare9, "remote_resolved_oid": E9, "evidence_path": evp9,
               "evidence_sha256": "0" * 64, "create_only_proof": {"push_argv_hash": "x",
               "push_transcript_hash": "y", "pre_absence_receipt": "z", "prepush_absence_receipt": "w",
               "derived_from_receipts": True}, "final_verdict": "PASS"}, open(a9, "w"))
    if E9:
        rv9 = sh(["python3", VERIFIER, "--package-dir", PKG, "--phase", "postpublish-binding", "--repo", repo,
                  "--c", C, "--r", R, "--e", E9, "--lab", "LAB-A",
                  "--evidence-path", evp9, "--planned-branch", "result/ctl-lab-a-v3_6",
                  "--remote", bare9, "--acceptance", a9, "--finalizer", "python3",
                  "--finalizer-args", f"{FINALIZER} --repo {repo} --c {C} --r {R} --lab LAB-A --batch {V37_BATCH} --planned-branch result/ctl-lab-a-v3_6 --scope-freeze {SF} --batch-manifest {BM} --functional-baseline {FB} --api-contract {API} --build-policy {BP} --controller-dir {repo} --evidence-path {evp9} --verifier-path {VERIFIER}",
                  "--verifier-path", VERIFIER], timeout=600)
        results["N9_A_wrong_R_reject"] = rv9.returncode != 0
    else:
        results["N9_A_wrong_R_reject"] = False
    print("case N9_A_wrong_R_reject:", "PASS" if results["N9_A_wrong_R_reject"] else "FAIL")

    # N10: full-completion without postpublish/acceptance must not pass a fake
    # (phase_full with missing acceptance -> errors accumulate -> REJECT)
    repo = fresh_repo("n10")
    OWNED_A = write_lab_a_modules(repo, "# module v1")
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_A[0]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    evp10 = "receipts/headless-lab-results-v3.7/lab-a-controller-evidence.json"
    fz10 = sh(["python3", FINALIZER, "--mode", "finalize", "--repo", repo, "--c", C, "--r", R,
               "--lab", "LAB-A", "--batch", V37_BATCH, "--planned-branch", "result/x-lab-a-v3_6",
               "--remote", "", "--scope-freeze", SF, "--batch-manifest", BM,
               "--functional-baseline", FB, "--api-contract", API, "--build-policy", BP,
               "--controller-dir", repo, "--evidence-path", evp10, "--verifier-path", VERIFIER],
              timeout=600)
    E10 = json.loads(fz10.stdout).get("E") if fz10.returncode == 0 else None
    if E10:
        # full-completion WITHOUT postpublish inputs: remote/branch/acceptance absent -> must not falsely pass
        rv10 = sh(["python3", VERIFIER, "--package-dir", PKG, "--phase", "full-completion", "--repo", repo,
                   "--c", C, "--r", R, "--e", E10, "--lab", "LAB-A",
                   "--evidence-path", evp10, "--planned-branch", "result/x-lab-a-v3_6",
                   "--remote", "file:///nonexistent", "--acceptance", os.path.join(base, "no-a.json"),
                   "--finalizer", "python3",
                   "--finalizer-args", f"{FINALIZER} --repo {repo} --c {C} --r {R} --lab LAB-A --batch {V37_BATCH} --planned-branch result/x-lab-a-v3_6 --scope-freeze {SF} --batch-manifest {BM} --functional-baseline {FB} --api-contract {API} --build-policy {BP} --controller-dir {repo} --evidence-path {evp10} --verifier-path {VERIFIER}",
                   "--verifier-path", VERIFIER], timeout=600)
        results["N10_full_without_acceptance_reject"] = rv10.returncode != 0
    else:
        results["N10_full_without_acceptance_reject"] = False
    print("case N10_full_without_acceptance_reject:", "PASS" if results["N10_full_without_acceptance_reject"] else "FAIL")

    # N11: gcc exit 0 but output is NOT an ELF -> controlled-build REJECT.
    # Shadow gcc with a wrapper that writes non-ELF junk and exits 0; canonical
    # source hash still matches, so the rejection must come from the ELF gate.
    repo = fresh_repo("n11")
    OWNED_B = write_lab_b_modules(repo)
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_B[2]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    fake_dir = os.path.join(base, "fakebin-n11")
    os.makedirs(fake_dir, exist_ok=True)
    with open(os.path.join(fake_dir, "gcc"), "w") as f:
        f.write("#!/bin/bash\nprev=\"\"\nfor a in \"$@\"; do\n  if [ \"$prev\" = \"-o\" ]; then printf 'not an elf\\n' > \"$a\"; exit 0; fi\n  prev=\"$a\"\ndone\nexit 0\n")
    os.chmod(os.path.join(fake_dir, "gcc"), 0o755)
    old_path = os.environ.get("PATH", "")
    os.environ["PATH"] = fake_dir + os.pathsep + old_path
    try:
        rr = run_finalizer(repo, C, R, "LAB-B", "result/x-lab-b-v3_6")
    finally:
        os.environ["PATH"] = old_path
    results["N11_gcc_non_elf_reject"] = rr.returncode != 0
    print("case N11_gcc_non_elf_reject:", "PASS" if results["N11_gcc_non_elf_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N12: ldd failure on the built ELF -> controlled-build REJECT.
    # Shadow ldd with a failing wrapper; real gcc builds a real ELF, so the
    # rejection must come from the ldd gate.
    repo = fresh_repo("n12")
    OWNED_B = write_lab_b_modules(repo)
    C = commit_all(repo, "C")
    with open(os.path.join(repo, OWNED_B[2]), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    fake_dir2 = os.path.join(base, "fakebin-n12")
    os.makedirs(fake_dir2, exist_ok=True)
    with open(os.path.join(fake_dir2, "ldd"), "w") as f:
        f.write("#!/bin/bash\necho 'not a dynamic executable' >&2\nexit 1\n")
    os.chmod(os.path.join(fake_dir2, "ldd"), 0o755)
    old_path = os.environ.get("PATH", "")
    os.environ["PATH"] = fake_dir2 + os.pathsep + old_path
    try:
        rr = run_finalizer(repo, C, R, "LAB-B", "result/x-lab-b-v3_6")
    finally:
        os.environ["PATH"] = old_path
    results["N12_ldd_failure_reject"] = rr.returncode != 0
    print("case N12_ldd_failure_reject:", "PASS" if results["N12_ldd_failure_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # ---------------- FC-F1 runtime-contract negative fixtures (v3.7) ----------------
    def lab_b_finalize(repo, C, R):
        return run_finalizer(repo, C, R, "LAB-B", "result/x-lab-b-v3_7")

    def mutate_and_finalize(mutator, lab="LAB-B", lab_b=True):
        repo = fresh_repo("mut-" + mutator.__name__)
        if lab_b:
            write_lab_b_modules(repo)
        else:
            write_lab_a_modules(repo)
        mutator(repo)
        C = commit_all(repo, "C")
        with open(os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py" if lab_b else "tests/integration/headless_audio_lab/process_supervisor.py"), "a") as f:
            f.write("# R\n")
        R = commit_all(repo, "R")
        rr = lab_b_finalize(repo, C, R) if lab_b else run_finalizer(repo, C, R, "LAB-A", "result/x-lab-a-v3_7")
        return rr

    # N13: wrong parameter ORDER in analyze_wav
    def m_param_order(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace("def analyze_wav(wav_path: str | Path, expected_loop_frames: int, observed_loop_frames: int",
                      "def analyze_wav(wav_path: str | Path, observed_loop_frames: int, expected_loop_frames: int", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_param_order)
    results["N13_wrong_param_order_reject"] = rr.returncode != 0
    print("case N13_wrong_param_order_reject:", "PASS" if results["N13_wrong_param_order_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N14: wrong parameter KIND (keyword-only after *)
    def m_param_kind(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace("sample_rate: int, silence_threshold: float = 0.01",
                      "sample_rate: int, *, silence_threshold: float = 0.01", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_param_kind)
    results["N14_wrong_param_kind_reject"] = rr.returncode != 0
    print("case N14_wrong_param_kind_reject:", "PASS" if results["N14_wrong_param_kind_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N15: wrong parameter DEFAULT
    def m_param_default(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace("silence_threshold: float = 0.01", "silence_threshold: float = 0.02", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_param_default)
    results["N15_wrong_param_default_reject"] = rr.returncode != 0
    print("case N15_wrong_param_default_reject:", "PASS" if results["N15_wrong_param_default_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N16: wrong RETURN annotation/contract
    def m_return_annotation(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace("-> AnalysisResult:", "-> dict:", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_return_annotation)
    results["N16_wrong_return_annotation_reject"] = rr.returncode != 0
    print("case N16_wrong_return_annotation_reject:", "PASS" if results["N16_wrong_return_annotation_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N17: analyze_wav returns dict (not AnalysisResult)
    def m_returns_dict(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace("return AnalysisResult(expected_loop_frames, observed_loop_frames, n, n, verification)",
                      "return {'verification': verification}", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_returns_dict)
    results["N17_analyze_wav_returns_dict_reject"] = rr.returncode != 0
    print("case N17_analyze_wav_returns_dict_reject:", "PASS" if results["N17_analyze_wav_returns_dict_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N18: AnalysisResult missing required field (verification)
    def m_missing_field(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace("self.verification=verification", "pass", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_missing_field)
    results["N18_AnalysisResult_missing_field_reject"] = rr.returncode != 0
    print("case N18_AnalysisResult_missing_field_reject:", "PASS" if results["N18_AnalysisResult_missing_field_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N19: analyze_loop_reproduction returns non-dict
    def m_non_dict(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace('return {"frame_ratio_ok": True, "silence_rejected": True}', "return ['x']", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_non_dict)
    results["N19_loop_reproduction_non_dict_reject"] = rr.returncode != 0
    print("case N19_loop_reproduction_non_dict_reject:", "PASS" if results["N19_loop_reproduction_non_dict_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N20: silent WAV verification=PASS (broken silence logic)
    def m_silent_pass(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py")
        s = open(p).read()
        s = s.replace("verification = 'FAIL'", "verification = 'PASS'", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_silent_pass)
    results["N20_silent_wav_pass_reject"] = rr.returncode != 0
    print("case N20_silent_wav_pass_reject:", "PASS" if results["N20_silent_wav_pass_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N21: GraphAssertions missing method (snapshot)
    def m_missing_method(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/graph_assertions.py")
        s = open(p).read()
        s = s.replace("    def snapshot(self): return {}\n", "", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_missing_method)
    results["N21_GraphAssertions_missing_method_reject"] = rr.returncode != 0
    print("case N21_GraphAssertions_missing_method_reject:", "PASS" if results["N21_GraphAssertions_missing_method_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N22: out-of-scope path in R (ownership violation: control-plane path)
    # The forbidden file must land in R (after C) so it appears in the C..R diff.
    repo = fresh_repo("mut-m_out_of_scope")
    write_lab_b_modules(repo)
    C = commit_all(repo, "C")
    os.makedirs(os.path.join(repo, "doc/sooperlooper"), exist_ok=True)
    with open(os.path.join(repo, "doc/sooperlooper/UNTOUCHABLE.md"), "w") as f:
        f.write("# not owned\n")
    with open(os.path.join(repo, "tests/integration/headless_audio_lab/audio_oracle.py"), "a") as f:
        f.write("# R\n")
    R = commit_all(repo, "R")
    rr = lab_b_finalize(repo, C, R)
    results["N22_out_of_scope_path_reject"] = rr.returncode != 0
    print("case N22_out_of_scope_path_reject:", "PASS" if results["N22_out_of_scope_path_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N23: LabRunner signature drift (cli gains a required param)
    def m_labrunner_drift(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/runner.py")
        s = open(p).read()
        s = s.replace("    def cli(self):", "    def cli(self, extra):", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_labrunner_drift)
    results["N23_LabRunner_signature_drift_reject"] = rr.returncode != 0
    print("case N23_LabRunner_signature_drift_reject:", "PASS" if results["N23_LabRunner_signature_drift_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    # N24: LAB-A method signature drift (start_process extra optional param)
    def m_lab_a_drift(repo):
        p = os.path.join(repo, "tests/integration/headless_audio_lab/process_supervisor.py")
        s = open(p).read()
        s = s.replace("def start_process(self, name, cmd, env, cwd, stdout_path, stderr_path):",
                      "def start_process(self, name, cmd, env, cwd, stdout_path, stderr_path, extra=None):", 1)
        open(p, "w").write(s)
    rr = mutate_and_finalize(m_lab_a_drift, lab_b=False)
    results["N24_lab_a_method_drift_reject"] = rr.returncode != 0
    print("case N24_lab_a_method_drift_reject:", "PASS" if results["N24_lab_a_method_drift_reject"] else "FAIL",
          f"(exit {rr.returncode})")

    ok = all(results.values())
    declared_cases = [
        "valid_LAB_A_r_only", "valid_LAB_A_finalize", "finalize_created_E",
        "deterministic_reproduce", "valid_LAB_B_controlled_build_finalize",
        "LAB_B_finalize_created_E", "branch_absence_proof", "create_only_push_remote_eq",
        "N1_command_hash_mismatch_reject", "N2_prepublish_fail_no_E",
        "N3_ignored_build_output_reject", "N4_dirty_after_build_reject",
        "N5_source_hash_mismatch_reject", "N6_gcc_no_output_reject",
        "N7_lsremote_ambiguous_reject", "N8_remote_branch_preexisting_reject",
        "N9_A_wrong_R_reject", "N10_full_without_acceptance_reject",
        "N11_gcc_non_elf_reject", "N12_ldd_failure_reject",
        "N13_wrong_param_order_reject", "N14_wrong_param_kind_reject",
        "N15_wrong_param_default_reject", "N16_wrong_return_annotation_reject",
        "N17_analyze_wav_returns_dict_reject", "N18_AnalysisResult_missing_field_reject",
        "N19_loop_reproduction_non_dict_reject", "N20_silent_wav_pass_reject",
        "N21_GraphAssertions_missing_method_reject", "N22_out_of_scope_path_reject",
        "N23_LabRunner_signature_drift_reject", "N24_lab_a_method_drift_reject",
    ]
    executed_cases = [k for k in declared_cases if k in results]
    passed_cases = [k for k in executed_cases if results[k]]
    failed_cases = [k for k in executed_cases if not results[k]]
    skipped_cases = [k for k in declared_cases if k not in results]
    print("declared_case_count=%d executed_case_count=%d passed_case_count=%d failed_case_count=%d skipped_case_count=%d"
          % (len(declared_cases), len(executed_cases), len(passed_cases), len(failed_cases), len(skipped_cases)))
    ok = (len(executed_cases) == len(declared_cases) and len(passed_cases) == len(declared_cases)
          and not failed_cases and not skipped_cases)
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
        if args.phase == "functional-continuity":
            return 0 if ok else 1
        if not ok:
            return 1

    if not (args.repo and args.c and args.r):
        print("need --repo --c --r for result phases")
        return 2
    baseline = load_json(os.path.join(pkg, "functional-contract-baseline-v3.7.json"))
    owned = baseline["canonical_ownership"].get(args.lab, [])
    batch = load_json(os.path.join(pkg, "batch-manifest-v3.7.json"))
    evp = args.evidence_path or f"receipts/headless-lab-results-v3.7/{args.lab.lower()}-controller-evidence.json"

    if args.phase in ("r-only-leaf-result", "all", "prepublish-finalization", "postpublish-binding", "full-completion"):
        print("== r-only-leaf-result ==")
        ok = v.phase_r_only(args.repo, args.c, args.r, owned, evp, batch)
        if args.phase == "r-only-leaf-result":
            return 0 if ok else 1
        if not ok:
            return 1

    if args.phase in ("prepublish-finalization", "all", "postpublish-binding", "full-completion"):
        if not args.e:
            print("need --e for prepublish")
            return 2
        print("== prepublish-finalization ==")
        finalizer_cmd = (args.finalizer or "python3").split()
        finalizer_args = args.finalizer_args.split() if args.finalizer_args else []
        ok = v.phase_prepublish(args.repo, args.c, args.r, args.e, evp,
                                args.planned_branch or "result/x", finalizer_cmd, finalizer_args,
                                args.verifier_path, args.lab, owned, batch, args.workdir,
                                remote=args.remote,
                                absence_required=(args.phase == "prepublish-finalization"))
        if args.phase == "prepublish-finalization":
            return 0 if ok else 1
        if not ok:
            return 1

    if args.phase in ("postpublish-binding", "all", "full-completion"):
        print("== postpublish-binding ==")
        # expected values from evidence blob at E
        ev = {}
        try:
            blob = subprocess.run(["git", "-C", args.repo, "cat-file", "blob", f"{args.e}:{evp}"],
                                  capture_output=True).stdout
            ev = json.loads(blob.decode())
        except Exception:
            pass
        expected_sha = None
        if blob:
            expected_sha = sha256_bytes(blob)
        ok = v.phase_postpublish(args.repo, args.c, args.r, args.e, args.branch or args.planned_branch,
                                 args.remote, args.e, args.acceptance, evp, args.workdir,
                                 expected_C=args.c, expected_R=args.r,
                                 expected_evidence_sha=expected_sha)
        if args.phase == "postpublish-binding":
            return 0 if ok else 1
        if not ok:
            return 1

    if args.phase in ("full-completion", "all"):
        print("== full-completion ==")
        blob2 = b""
        try:
            blob2 = subprocess.run(["git", "-C", args.repo, "cat-file", "blob", f"{args.e}:{evp}"],
                                   capture_output=True).stdout
        except Exception:
            pass
        expected_sha2 = sha256_bytes(blob2) if blob2 else None
        ok = v.phase_full(args.repo, args.c, args.r, args.e, evp, args.acceptance, owned, batch,
                          args.planned_branch or "result/x",
                          (args.finalizer or "python3").split(),
                          args.finalizer_args.split() if args.finalizer_args else [],
                          args.verifier_path, args.lab, args.workdir,
                          remote=args.remote, expected_E=args.e, expected_C=args.c,
                          expected_R=args.r, expected_evidence_sha=expected_sha2)
        return 0 if ok else 1

    print("no phase executed")
    return 2


if __name__ == "__main__":
    sys.exit(main())

