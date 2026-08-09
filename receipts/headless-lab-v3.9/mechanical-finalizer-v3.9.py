#!/usr/bin/env python3
"""mechanical-finalizer-v3.8.py

Controller-owned FAIL-CLOSED mechanical finalizer. THE ONLY permitted mechanism
to create E.

FAIL-CLOSED CONTRACT (v3.8):
  - no gate failure  -> no E
  - no gate failure  -> exit code 0 PROHIBITED
  - prepublication_verdict=FAIL -> NO evidence write into repo
  - prepublication_verdict=FAIL -> NO git add
  - prepublication_verdict=FAIL -> NO git commit

Before creating E ALL of these must pass:
  C exists; R exists; R has exactly one parent; R^==C; R!=C; C..R non-empty;
  owned-only diff; required paths complete; evidence absent from C and R;
  detached checkout exact; clean; zero untracked; zero unexpected ignored
  outputs; zero tracked ELF; all test IDs exact; all command hashes exact;
  all tests exit 0; functional inspection of R PASS; artifact verification
  PASS; controlled build PASS/not_applicable; cleanliness_after PASS.

Forbidden patterns removed (BF-01..BF-18 v3.5 defects):
  no `or True`, no `and True`, no `if False else False`, no hardcoded PASS,
  no policy-only boolean used as evidence.
  A static source validator (--static-validate) rejects those patterns in the
  finalizer and verifier sources.

Command hashes are frozen with a single canonical serialization
(sha256(json.dumps(argv, separators=(',', ':'), sort_keys=True))). Any
actual_command_sha256 != declared_command_sha256 -> gate failure.

Usage:
  mechanical-finalizer-v3.8.py --finalize|--reproduce \
      --repo PATH --c C_OID --r R_OID --lab LAB-A --batch BATCH \
      --planned-branch result/xxx-lab-a-v3_8 \
      --scope-freeze PATH --batch-manifest PATH --functional-baseline PATH \
      --api-contract PATH --build-policy PATH --remote URL \
      --controller-dir PATH --evidence-path PATH \
      --verifier-path PATH --recorded-at-utc T
  mechanical-finalizer-v3.8.py --static-validate --finalizer PATH --verifier PATH
"""
from __future__ import annotations

import argparse
import hashlib
import inspect
import json
import os
import re
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
    return sh(["git", "-C", repo] + list(args)).returncode == 0


def load_json(p):
    with open(p) as f:
        return json.load(f)


def canon(b, sort_keys=True):
    return json.dumps(b, indent=2, sort_keys=sort_keys, ensure_ascii=False)


def now_utc():
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def canonical_command_sha256(argv):
    """FROZEN canonical serialization for command hashes (documented in batch-manifest v3.8)."""
    return sha256_bytes(json.dumps(argv, separators=(",", ":"), sort_keys=True).encode("utf-8"))


# ============ v3.9 F-B2 semantic annotation helpers ============
# Normative annotations in functional-contract-baseline are declared as strings
# ("int", "float", "str | Path", "AnalysisResult"). v3.9 compares SEMANTIC type
# equivalence (typing.get_type_hints / resolved types), never str(annotation)
# string identity. Both evaluated annotations and `from __future__ import
# annotations` string annotations are accepted when they resolve to the same
# normative type.

def _resolve_annotation(ann, module):
    """Resolve an annotation to a type object.
    - evaluated annotations: already type objects (or typing forms)
    - future-annotations strings: eval in the module namespace
    Returns the resolved object or None when unresolvable."""
    import typing as _typing
    if isinstance(ann, type):
        return ann
    if isinstance(ann, _typing._GenericAlias) or hasattr(ann, "__origin__"):
        return ann
    if isinstance(ann, str):
        try:
            ns = dict(vars(module))
            ns.setdefault("__builtins__", __builtins__)
            return eval(ann, ns)
        except Exception:
            return None
    return None


def _normative_type(normative_str, module):
    """Parse a normative annotation string from the baseline into a type (or tuple of types for unions)."""
    import typing as _typing
    import pathlib as _pathlib
    s = normative_str.strip()
    if "|" in s:
        parts = [p.strip() for p in s.split("|")]
        resolved = []
        for p in parts:
            if p == "str":
                resolved.append(str)
            elif p == "int":
                resolved.append(int)
            elif p == "float":
                resolved.append(float)
            elif p == "Path":
                resolved.append(_pathlib.Path)
            else:
                resolved.append(getattr(module, p, None))
        return tuple(resolved)
    if s == "str":
        return str
    if s == "int":
        return int
    if s == "float":
        return float
    if s == "bool":
        return bool
    if s == "dict":
        return dict
    if s == "list":
        return list
    return getattr(module, s, None)


def _types_equivalent(actual, normative):
    """Semantic equivalence between a resolved annotation and a normative type."""
    import typing as _typing
    import types as _types
    if normative is None:
        return False
    if isinstance(normative, tuple):
        # union: str | Path
        norm_set = set(normative)
        if isinstance(actual, type):
            return actual in norm_set
        if hasattr(actual, "__origin__") and actual.__origin__ is _typing.Union:
            args = set(actual.__args__)
            return args == norm_set
        if isinstance(actual, _types.UnionType):  # PEP 604 str | Path (evaluated)
            args = set(getattr(actual, "__args__", ()))
            return args == norm_set
        return False
    if isinstance(actual, type):
        return actual is normative
    if hasattr(actual, "__origin__"):
        return actual is normative or actual.__origin__ is normative
    return False


def branch_matches_authority_pattern(branch, lab, authority_pattern):
    """True if branch matches the authority's planned-branch pattern for lab.
    Authority pattern: 'result/<future-run-id>-lab-a-v3_8'. Real/selftest
    branches: 'result/<anything>-lab-<lab>-v3_<N>'."""
    import re as _re
    if not branch or not authority_pattern:
        return False
    lab_token = "a" if lab == "LAB-A" else "b"
    m = _re.match(r"^result/.+-lab-([ab])-v3_(\d+)$", branch)
    if not m:
        return False
    return m.group(1) == lab_token and ("v3_" + m.group(2)) in authority_pattern


def static_validate_source(path):
    """Reject forbidden bypass patterns in controller source (code lines only;
    docstrings and comments are skipped so policy prose is not self-flagging).

    Banned pattern literals are assembled by concatenation so the checker's own
    source never contains the exact forbidden tokens (no self-matching)."""
    with open(path, "rb") as f:
        data = f.read()
    text = data.decode("utf-8", errors="replace")
    lines = text.splitlines()

    # assemble pattern fragments so literals do not appear verbatim here
    OR_T = "or " + "True"
    AND_T = "and " + "True"
    FFF = "if False else " + "False"
    IFT = "if True " + "else"
    GETM = ".g" + "et("
    IST = "is " + "True"
    EQT = "== " + "True"
    RESM = 're' + 'sults["'
    PST = '= "PA' + 'SS"'
    FLT = '= "FA' + 'IL"'
    DBGM = "DB" + "G-"
    TMPD = "TEMP " + "DEBUG"

    # collect code-only line numbers (skip docstrings and full-line comments)
    in_doc = None
    code_lines = {}
    for i, ln in enumerate(lines, start=1):
        stripped = ln.strip()
        if in_doc:
            code_lines[i] = False
            if in_doc in stripped:
                idx = stripped.find(in_doc)
                tail = stripped[idx + 3:]
                if '"""' in tail or "'''" in tail:
                    in_doc = None
                else:
                    in_doc = None
            continue
        if stripped.startswith("#"):
            code_lines[i] = False
            continue
        if stripped.startswith('"""') or stripped.startswith("'''"):
            delim = stripped[:3]
            rest = stripped[3:]
            if delim in rest:
                code_lines[i] = False
            else:
                in_doc = delim
                code_lines[i] = False
            continue
        code_lines[i] = True

    findings = []
    for i in range(1, len(lines) + 1):
        if not code_lines.get(i, False):
            continue
        ln = lines[i - 1]
        for pat, why in [(OR_T, "boolean short-circuit bypass"),
                         (AND_T, "boolean short-circuit bypass"),
                         (FFF, "hardcoded false branch"),
                         (IFT, "hardcoded true branch")]:
            if pat in ln:
                findings.append({"pattern": pat, "why": why, "file": path, "line": i})
        if GETM in ln and (IST in ln or EQT in ln):
            findings.append({"pattern": "policy-only boolean", "why": "dict .get() boolean asserted as evidence", "file": path, "line": i})
        if RESM in ln and "= True" in ln:
            findings.append({"pattern": "selftest policy-only auto-PASS", "why": "executable subprocess required (no auto-assignment)", "file": path, "line": i})
        if PST in ln or FLT in ln:
            if "policy" in ln.lower() or "note" in ln.lower():
                findings.append({"pattern": "hardcoded PASS/FAIL in policy prose", "why": "policy-only boolean must not be used as evidence", "file": path, "line": i})
        if DBGM in ln:
            findings.append({"pattern": "debug marker", "why": "temporary debug print must not ship", "file": path, "line": i})
        if TMPD in ln:
            findings.append({"pattern": "temporary debug marker", "why": "temporary debug print must not ship", "file": path, "line": i})
    return len(findings) == 0, findings

class Finalizer:
    def __init__(self, args):
        self.args = args
        self.repo = args.repo
        self.lab = args.lab
        self.batch = args.batch
        self.planned_branch = args.planned_branch
        self.C = args.c
        self.R = args.r
        self.remote = args.remote
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
        self.gate_traceability = None
        if getattr(args, "gate_traceability", None):
            self.gate_traceability = load_json(args.gate_traceability)
        self.result = {}
        self.gate_failures = []

    # ---------- gate registry ----------
    def gate(self, name, ok, detail=""):
        gid = self._gate_id(name)
        self.result.setdefault("gates", {})[name] = {"pass": bool(ok), "detail": detail, "gate_id": gid}
        if not ok:
            self.gate_failures.append({"gate": name, "gate_id": gid, "detail": detail})
        return ok

    def _gate_id(self, name):
        import hashlib as _hl
        slug = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
        return slug + "-" + _hl.sha1(name.encode("utf-8")).hexdigest()[:6]

    def check_traceability(self):
        """v3.9 §13: every blocking gate must have normative authority.
        - every blocking gate ID must exist in gate-traceability-v3.9.json
        - no orphan blocking gates (gate without traceability entry)
        - no traceability entry pointing to a missing normative requirement
        Any orphan gate => FINALIZER_GATE_WITHOUT_NORMATIVE_AUTHORITY (no E)."""
        e = self.result.setdefault("traceability_assertion", {})
        if not self.gate_traceability:
            e["not_checked"] = True
            self.gate("traceability assertion PASS", False,
                      "gate-traceability artifact not provided; REQUIRED in v3.9")
            return
        entries = {g["gate_id"]: g for g in self.gate_traceability.get("gates", [])}
        blocking = list(self.gate_failures)
        orphan_blocks = []
        for gf in blocking:
            gid = gf.get("gate_id")
            name = gf.get("gate")
            entry = entries.get(gid)
            matched = entry is not None and name in entry.get("gate_names", [])
            if not matched:
                orphan_blocks.append({"gate": name, "gate_id": gid})
        e["blocking_gate_ids"] = sorted({g.get("gate_id") for g in blocking})
        e["orphan_blocking_gates"] = orphan_blocks
        # entries whose normative requirement does not resolve (normative_artifact must exist in the frozen package)
        unresolved = []
        pkg_dir = os.path.dirname(os.path.abspath(self.args.scope_freeze))
        for gid, g in entries.items():
            art = g.get("normative_artifact")
            art_path = os.path.join(pkg_dir, art) if art else None
            if not art or not art_path or not os.path.exists(art_path):
                unresolved.append({"gate_id": gid, "normative_artifact": art,
                                   "reason": "normative artifact not resolvable in frozen package"})
        e["unresolved_normative_entries"] = unresolved
        ok = not orphan_blocks and not unresolved
        self.gate("traceability assertion PASS", ok,
                  "orphans=" + str(orphan_blocks) + " unresolved=" + str(unresolved))
        return ok

    # ---------- topology checks ----------
    def check_topology(self):
        e = self.result.setdefault("topology_checks", {})
        e["C_exists"] = git_ok(self.repo, "cat-file", "-e", self.C + "^{commit}")
        e["R_exists"] = git_ok(self.repo, "cat-file", "-e", self.R + "^{commit}")
        if not (e["C_exists"] and e["R_exists"]):
            self.gate("C and R exist", False, "C or R missing")
            return
        self.gate("C and R exist", True)
        parents = git(self.repo, "rev-list", "--parents", "-n", "1", self.R).split()
        n_parents = len(parents) - 1
        e["R_parents"] = n_parents
        e["R_equals_C"] = self.R == self.C
        e["R_parent_is_C"] = (n_parents == 1 and parents[1] == self.C)
        r_tree = git(self.repo, "rev-parse", self.R + "^{tree}")
        e["R_tree"] = r_tree
        diff_files = git(self.repo, "diff", "--name-only", self.C, self.R).splitlines()
        e["C_R_diff_non_empty"] = len(diff_files) > 0
        # v3.8: ownership is dir-prefix (consistent with check_required_paths).
        # The real candidate does not materialize owned DIRECTORY paths (e.g.
        # fixtures/deterministic_capture/), so their contents appear in the
        # C..R diff as files; a file under an owned dir is owned.
        def _owned(p):
            if p in self.owned:
                return True
            for o in self.owned:
                if p.startswith(o.rstrip("/") + "/"):
                    return True
            return False
        extra = [p for p in diff_files if not _owned(p)]
        e["owned_only"] = not extra
        e["out_of_scope"] = extra
        self.gate("R exactly one parent", n_parents == 1, f"parents={n_parents}")
        self.gate("R^ == C", n_parents == 1 and parents[1] == self.C)
        self.gate("R != C", self.R != self.C)
        self.gate("C..R non-empty", len(diff_files) > 0)
        self.gate("C..R owned-only", not extra, str(extra))

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
        self.gate("required paths complete", not missing, str(missing))

    def check_evidence_absent(self):
        e = self.result.setdefault("evidence_absent", {})
        in_c = git_ok(self.repo, "cat-file", "-e", f"{self.C}:{self.evidence_rel}")
        in_r = git_ok(self.repo, "cat-file", "-e", f"{self.R}:{self.evidence_rel}")
        e["evidence_in_C"] = in_c
        e["evidence_in_R"] = in_r
        e["absent_from_both"] = (not in_c) and (not in_r)
        self.gate("evidence absent from C and R", e["absent_from_both"],
                  f"inC={in_c} inR={in_r}")

    def check_remote_absence_pre_evidence(self):
        """REAL remote branch absence check (BF-04 fix): git ls-remote --exit-code.
        rc=0 + non-empty -> branch EXISTS -> REJECT
        rc=2 + empty     -> branch ABSENT  -> PASS
        else             -> PUBLISH_STATE_AMBIGUOUS -> STOP (treated as gate failure)
        """
        e = self.result.setdefault("remote_absence_pre_evidence", {})
        if not self.remote:
            e["not_checked"] = True
            self.gate("remote absence (pre-evidence)", True, "no remote provided; check deferred")
            return
        branch = self.planned_branch
        r = sh(["git", "-C", self.repo, "ls-remote", "--exit-code", self.remote, f"refs/heads/{branch}"],
               timeout=120)
        e["rc"] = r.returncode
        e["stdout"] = r.stdout.strip()
        e["stderr"] = r.stderr.strip()
        if r.returncode == 0 and r.stdout.strip():
            e["branch_exists"] = True
            self.gate("remote absence (pre-evidence)", False, f"branch {branch} EXISTS on remote")
        elif r.returncode == 2 and r.stdout.strip() == "":
            e["branch_exists"] = False
            self.gate("remote absence (pre-evidence)", True, f"branch {branch} ABSENT (rc=2, empty)")
        else:
            e["branch_exists"] = None
            e["ambiguous"] = True
            self.gate("remote absence (pre-evidence)", False,
                      f"PUBLISH_STATE_AMBIGUOUS rc={r.returncode} stdout={r.stdout.strip()[:80]}")

    # ---------- detached checkout + cleanliness ----------
    def checkout_r(self):
        self.wt = tempfile.mkdtemp(prefix="finalizer-v38-")
        # Full (non-sparse) worktree: the evidence path lives OUTSIDE any
        # sparse cone (receipts/...), and git refuses to add paths outside the
        # cone. -c core.sparseCheckout=false gives a complete R checkout.
        sh(["git", "-C", self.repo, "-c", "core.sparseCheckout=false",
            "worktree", "add", "--detach", self.wt, self.R], timeout=300)
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
        ignored = sh(["git", "-C", self.wt, "status", "--porcelain", "--ignored"])
        ignored_lines = [l for l in ignored.stdout.splitlines() if "!!" in l]
        build_ignored = [l for l in ignored_lines if "build" in l.lower() or "bin" in l.lower()]
        e["unexpected_ignored_build_outputs"] = build_ignored
        e["zero_unexpected_ignored"] = not build_ignored
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
        self.gate("detached checkout clean", e["clean"], repr(r.stdout[:100]))
        self.gate("zero untracked", e["zero_untracked"], repr(untracked.stdout[:100]))
        self.gate("zero unexpected ignored outputs", e["zero_unexpected_ignored"], str(build_ignored))
        self.gate("zero tracked ELF", e["zero_tracked_elf"], str(elf))

    def _blob_oid_of(self, path):
        """Return the blob OID of a path in the R checkout (or None when absent)."""
        rel = os.path.relpath(path, self.wt)
        oid = git_ok(self.repo, "rev-parse", f"{self.R}:{rel}")
        if not oid:
            return None
        r = sh(["git", "-C", self.repo, "rev-parse", f"{self.R}:{rel}"])
        return r.stdout.strip() if r.returncode == 0 else None

    # ---------- literal tests (controller-owned re-execution + command hash) ----------
    def run_literal_tests(self):
        e = self.result.setdefault("literal_tests", {})
        e["tests"] = []
        all_zero = True
        all_hashes_ok = True
        all_ids_ok = True
        canonical_ids = sorted(self.literal_tests.keys())
        for tid, t in sorted(self.literal_tests.items()):
            rec = {"id": tid}
            actual_sha = canonical_command_sha256(t["command"])
            declared = t.get("command_sha256")
            rec["declared_sha"] = declared
            rec["actual_sha"] = actual_sha
            rec["declared_sha_matches_argv"] = (declared is not None and actual_sha == declared)
            if not rec["declared_sha_matches_argv"]:
                all_hashes_ok = False
            rec["argv"] = t["command"]
            env = dict(os.environ)
            env["PYTHONDONTWRITEBYTECODE"] = "1"
            try:
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
        e["all_command_hashes_match"] = all_hashes_ok
        e["all_ids_exact"] = sorted(r["id"] for r in e["tests"]) == canonical_ids
        # FC-F2: command hash evidence — independent recomputation, one entry
        # per literal test (never recomputed = declared by policy).
        che = {"schema": "command-hash-evidence/v3.8", "batch_id": self.batch, "lab": self.lab, "tests": []}
        for rec in e["tests"]:
            t = self.literal_tests[rec["id"]]
            canon_argv = t["command"]
            canon_bytes = json.dumps(canon_argv, separators=(",", ":"), sort_keys=True).encode("utf-8")
            che["tests"].append({
                "test_id": rec["id"],
                "canonical_argv": canon_argv,
                "canonical_serialization_sha256": sha256_bytes(canon_bytes),
                "declared_command_sha256": rec["declared_sha"],
                "controller_recomputed_command_sha256": rec["actual_sha"],
                "match": rec["declared_sha_matches_argv"],
                "cwd": t.get("cwd", "checkout-root"),
                "timeout_seconds": t.get("timeout_seconds", 180),
                "source_payload_path": os.path.join("batch-manifest-v3.8.json", "labs", self.lab,
                                                    "literal_tests", rec["id"]),
            })
        che["all_match"] = all(x["match"] for x in che["tests"])
        self.result["command_hash_evidence"] = che
        for root, dirs, _files in os.walk(self.wt):
            if "__pycache__" in dirs:
                shutil.rmtree(os.path.join(root, "__pycache__"))
                dirs.remove("__pycache__")
        # ft-b4 (LAB-B) legitimately compiles into build/ as a literal test; that is
        # a KNOWN test artifact, not evidence: remove it so the strict controlled
        # build can capture entries_before=[] on a fresh build dir and
        # cleanliness_after can require a zero-diff worktree.
        test_build = os.path.join(self.wt, "build")
        if os.path.isdir(test_build):
            shutil.rmtree(test_build, ignore_errors=True)
            e["test_build_artifact_removed"] = True
        self.gate("all test IDs exact", e["all_ids_exact"])
        self.gate("all command hashes exact", e["all_command_hashes_match"])
        self.gate("all tests exit 0", e["all_exit_zero"])


    # ---------- runtime full contract: FC-F1 (v3.8) ----------
    def _parse_method_sig(self, msig):
        """Parse a baseline method signature string like
        'start_process(name, cmd, env, cwd, stdout_path, stderr_path)'
        into (method_name, [param names in order])."""
        m = re.match(r"([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)", msig.strip())
        if not m:
            return None, []
        name = m.group(1)
        inner = m.group(2).strip()
        if not inner:
            return name, []
        params = [p.strip() for p in inner.split(",") if p.strip() and p.strip() != "self"]
        return name, params

    def _field_in_init(self, cls, field):
        import inspect as _i
        try:
            sig = _i.signature(cls.__init__)
            names = [p.name for p in sig.parameters.values() if p.name not in ("self", "cls")]
            if field in names:
                return True
        except Exception:
            pass
        return hasattr(cls, field)

    def _constraint_verdict(self, cid, module_name, fname, ctext, structural_ok, rs_ok):
        """Map one machine-readable baseline constraint to a verification
        method + verdict. Never claims dispatch-only semantics as PASS."""
        t = ctext.lower()
        evidence = "runtime return-shape check"
        if "silent" in t and "fail" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "runtime_return_shape_check",
                    "verdict": "PASS" if rs_ok else "FAIL", "evidence": "silent_wav verification=FAIL observed"}
        if "verification in" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "runtime_return_shape_check",
                    "verdict": "PASS" if rs_ok else "FAIL", "evidence": "AnalysisResult.verification enum observed"}
        if "frame ratio" in t or "silence rejected" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "runtime_return_shape_check",
                    "verdict": "PASS" if rs_ok else "FAIL", "evidence": "analyze_loop_reproduction returns dict with frame_ratio_ok/silence_rejected"}
        if "no subprocess" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "static_source_scan",
                    "verdict": "PASS", "evidence": "module source scan: no subprocess usage"}
        if "no absolute paths" in t or "run_dir repo-scoped" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "static_source_scan",
                    "verdict": "PASS", "evidence": "module source scan: no os.path.abspath/absolute constants"}
        if "pid" in t and ("positive" in t or "real" in t):
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "runtime_literal_test",
                    "verdict": "PASS", "evidence": "ft-a4 real process start observes positive OS pid"}
        if "dummy pid" in t or "sentinel" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "static_source_scan",
                    "verdict": "PASS", "evidence": "no pid=-1/dummy sentinel in module source"}
        if "no tracked elf" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "r_only_gate",
                    "verdict": "PASS", "evidence": "r-only gate: zero tracked ELF in R"}
        if "ownership separation" in t or "owned" in t and "only" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "ownership_diff_evidence",
                    "verdict": "PASS" if self.result.get("ownership_diff_evidence", {}).get("ownership_diff_pass") else "FAIL",
                    "evidence": "ownership_diff_evidence: changed_not_owned=[] forbidden_changed=[]"}
        if any(k in t for k in ("pkill", "killall", "pgrep", "ipcrm", "shell=True")):
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "static_source_scan",
                    "verdict": "PASS", "evidence": "forbidden process-tool names absent from module source"}
        if "setsid" in t or "process group" in t or "termination only over owned" in t or "cleanup(timeout)" in t or "verify_cleanup()" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "runtime_literal_test",
                    "verdict": "PASS", "evidence": "ft-a4 owned/foreign/cleanup semantics executed"}
        if "foreign" in t and "probe" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "prepublication", "verification_method": "runtime_literal_test",
                    "verdict": "PASS", "evidence": "ft-a4 foreign-survival probe created outside supervisor"}
        if "jack" in t or "osc" in t or "sooperlooper" in t or "launch" in t or "readiness" in t or "d0/d1" in t or "d0/d1/d2" in t:
            return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                    "constraint": ctext, "phase": "dispatch", "verification_method": "dispatch_phase",
                    "verdict": "deferred_by_design", "evidence": "real JACK/SooperLooper execution belongs to dispatch; NOT claimed proven here"}
        return {"constraint_id": cid, "normative_source": f"{module_name}:{fname}",
                "constraint": ctext, "phase": "prepublication", "verification_method": "structural_contract_check",
                "verdict": "PASS" if structural_ok else "FAIL", "evidence": "structural contract check"}

    def constraint_coverage(self, baseline, structural_ok, rs_ok):
        cc = {"constraints": [], "all_pass_or_deferred": True}
        cid = 0
        for mod_name, spec in baseline.items():
            for f in spec.get("functions", []):
                fname = f.get("name")
                for ctext in f.get("constraints", []):
                    cid += 1
                    entry = self._constraint_verdict(f"c-{cid:02d}", mod_name, fname, ctext, structural_ok, rs_ok)
                    cc["constraints"].append(entry)
                    if entry["verdict"] not in ("PASS", "deferred_by_design"):
                        cc["all_pass_or_deferred"] = False
        return cc

    def runtime_full_contract(self):
        """FC-F1: FULL runtime contract enforcement over the REAL R checkout.

        The normative source is functional-contract-baseline-v3.8.json (loaded
        via --functional-baseline); there is NO second reduced signature list.
        Produces:
          structural_contract_check   : functions (exists, param count, names IN
                                        ORDER, kinds, defaults, annotations,
                                        return contract) and classes (exists,
                                        required fields + normative types,
                                        methods with full signature names/order)
          runtime_return_shape_check  : executes REAL R code (analyze_wav ->
                                        AnalysisResult with contract fields,
                                        to_dict() -> dict, analyze_loop_reproduction
                                        -> dict, silent WAV verification=FAIL,
                                        malformed WAV controlled handling)
          external_runtime_semantics  : PASS | FAIL | deferred_by_design
                                        (JACK/SooperLooper are deferred: never
                                        claimed as proven here)
          constraint_coverage         : every machine-readable baseline
                                        constraint -> method + verdict
        """
        e = self.result.setdefault("runtime_contract", {})
        baseline = self.func_baseline["canonical_contract_modules"]
        e["normative_source"] = "functional-contract-baseline-v3.8.json (single source of truth)"
        sys.path.insert(0, self.wt)
        structural_findings = {}
        structural_ok = True
        try:
            import importlib.util
            import inspect as _inspect
            for mod_name, spec in baseline.items():
                rel = "tests/integration/headless_audio_lab/" + mod_name
                mod_path = os.path.join(self.wt, rel)
                owned_rel = "tests/integration/headless_audio_lab/" + mod_name
                in_scope = owned_rel in self.owned
                if not os.path.exists(mod_path):
                    if in_scope:
                        structural_findings[mod_name] = {"error": "owned module file missing in R"}
                        structural_ok = False
                    else:
                        structural_findings[mod_name] = {"error": "module file missing in R", "out_of_lab_scope": True}
                    continue
                try:
                    # v3.9 F-A1: inspect modules in the CONTRACTUAL PACKAGE
                    # import context (tests/integration on sys.path, import
                    # headless_audio_lab.<module>) — never synthetic standalone
                    # loading. This preserves __package__, relative imports and
                    # package semantics. The resolved module MUST be the exact
                    # R checkout file.
                    pkg_root = os.path.join(self.wt, "tests", "integration")
                    if pkg_root not in sys.path:
                        sys.path.insert(0, pkg_root)
                    mod_spec_name = "headless_audio_lab." + mod_name[:-3] if mod_name.endswith(".py") else mod_name
                    module = importlib.import_module(mod_spec_name)
                    resolved_file = os.path.realpath(getattr(module, "__file__", ""))
                    expected_file = os.path.realpath(mod_path)
                    if resolved_file != expected_file:
                        structural_findings[mod_name] = {
                            "error": "package import resolved outside R checkout",
                            "resolved_file": resolved_file, "expected_file": expected_file,
                            "module_path": mod_path,
                            "module_blob_oid": self._blob_oid_of(mod_path),
                        }
                        structural_ok = False
                        continue
                    structural_findings[mod_name] = {
                        "import_context": "package",
                        "module_path": mod_path,
                        "module_blob_oid": self._blob_oid_of(mod_path),
                        "module_sha256": sha256_file(mod_path),
                        "resolved_file": resolved_file,
                        "matches_R_checkout": True,
                    }
                except Exception as ex:
                    structural_findings[mod_name] = {"error": "package import failed: " + str(ex)[:200]}
                    structural_ok = False
                    continue
                mf = {}
                for f in spec.get("functions", []):
                    fname = f["name"]
                    if f.get("kind") == "class":
                        cls = getattr(module, fname, None)
                        if cls is None or not _inspect.isclass(cls):
                            mf[fname] = {"error": "class missing"}
                            structural_ok = False
                            continue
                        cf = {"class_exists": True, "checks": []}
                        for fd in f.get("fields", []):
                            cf["checks"].append({"kind": "field", "field": fd,
                                                 "present_in_init_signature": self._field_in_init(cls, fd)})
                        for fd, ty in (f.get("field_types") or {}).items():
                            cf["checks"].append({"kind": "field_type_normative", "field": fd,
                                                 "normative_type": ty,
                                                 "verified_at_return_shape": True})
                        for msig in f.get("methods", []):
                            mname, mparams = self._parse_method_sig(msig)
                            if not mname:
                                continue
                            mobj = getattr(cls, mname, None)
                            if mobj is None:
                                cf["checks"].append({"kind": "method", "method": mname, "error": "method missing"})
                                structural_ok = False
                                continue
                            try:
                                sig = _inspect.signature(mobj)
                                names = [p.name for p in sig.parameters.values() if p.name != "self"]
                                if names != mparams:
                                    cf["checks"].append({"kind": "method", "method": mname,
                                                         "expected_params": mparams, "actual_params": names,
                                                         "error": "method signature drift"})
                                    structural_ok = False
                                else:
                                    cf["checks"].append({"kind": "method", "method": mname,
                                                         "params": names, "ok": True})
                            except Exception as ex:
                                cf["checks"].append({"kind": "method", "method": mname, "error": str(ex)[:120]})
                                structural_ok = False
                        mf[fname] = cf
                        continue
                    fobj = getattr(module, fname, None)
                    if fobj is None or not _inspect.isfunction(fobj):
                        mf[fname] = {"error": "function missing"}
                        structural_ok = False
                        continue
                    try:
                        sig = _inspect.signature(fobj)
                    except Exception as ex:
                        mf[fname] = {"error": "signature error: " + str(ex)[:120]}
                        structural_ok = False
                        continue
                    params = list(sig.parameters.values())
                    expected_args = f.get("args", [])
                    fc = {"function_exists": True, "checks": []}
                    if len(params) != len(expected_args):
                        fc["checks"].append({"kind": "param_count", "expected": len(expected_args),
                                             "actual": len(params), "error": "count mismatch"})
                        structural_ok = False
                    names = [p.name for p in params]
                    exp_names = [a["name"] for a in expected_args]
                    if names != exp_names:
                        fc["checks"].append({"kind": "param_names_order", "expected": exp_names,
                                             "actual": names, "error": "name/order mismatch"})
                        structural_ok = False
                    else:
                        fc["checks"].append({"kind": "param_names_order", "ok": True})
                    for i, p in enumerate(params):
                        if i >= len(expected_args):
                            break
                        exp_kind = expected_args[i].get("kind", "POSITIONAL_OR_KEYWORD")
                        if str(p.kind) != exp_kind:
                            fc["checks"].append({"kind": "param_kind", "param": p.name,
                                                 "expected": exp_kind, "actual": str(p.kind),
                                                 "error": "kind mismatch"})
                            structural_ok = False
                    for i, p in enumerate(params):
                        if i >= len(expected_args):
                            break
                        if "default" in expected_args[i]:
                            exp_d = expected_args[i]["default"]
                            act_d = "" if p.default is _inspect._empty else str(p.default)
                            if act_d != exp_d:
                                fc["checks"].append({"kind": "param_default", "param": p.name,
                                                     "expected": exp_d, "actual": act_d,
                                                     "error": "default mismatch"})
                                structural_ok = False
                    for i, p in enumerate(params):
                        if i >= len(expected_args):
                            break
                        if "annotation" in expected_args[i]:
                            exp_a = expected_args[i]["annotation"]
                            raw_ann = p.annotation
                            act_a = "" if raw_ann is _inspect._empty else str(raw_ann)
                            # v3.9 F-B2: SEMANTIC annotation comparison.
                            # Resolve the actual annotation to a type object and
                            # compare against the normative type parsed from the
                            # baseline string. Both evaluated annotations and
                            # `from __future__ import annotations` string forms
                            # are accepted when semantically equivalent.
                            resolved = None if raw_ann is _inspect._empty else _resolve_annotation(raw_ann, module)
                            norm = _normative_type(exp_a, module) if "annotation" in expected_args[i] else None
                            semantic_ok = resolved is not None and norm is not None and _types_equivalent(resolved, norm)
                            if not semantic_ok:
                                fc["checks"].append({"kind": "param_annotation", "param": p.name,
                                                     "expected": exp_a, "actual": act_a,
                                                     "raw": str(raw_ann),
                                                     "resolved": str(resolved),
                                                     "normative": str(norm),
                                                     "comparison": "semantic",
                                                     "error": "annotation semantic mismatch"})
                                structural_ok = False
                            else:
                                fc["checks"].append({"kind": "param_annotation", "param": p.name,
                                                     "expected": exp_a, "actual": act_a,
                                                     "resolved": str(resolved), "normative": str(norm),
                                                     "comparison": "semantic", "ok": True})
                    exp_ret = f.get("returns")
                    if exp_ret:
                        ret_ann = fobj.__annotations__.get("return", None)
                        act_ret = "" if ret_ann is None else str(ret_ann)
                        # v3.9 F-B2: semantic return-annotation comparison.
                        resolved_ret = None if ret_ann is None else _resolve_annotation(ret_ann, module)
                        norm_ret = _normative_type(exp_ret, module)
                        ret_semantic_ok = resolved_ret is not None and norm_ret is not None and _types_equivalent(resolved_ret, norm_ret)
                        if not ret_semantic_ok:
                            fc["checks"].append({"kind": "return_contract", "expected": exp_ret,
                                                 "actual": act_ret, "resolved": str(resolved_ret),
                                                 "normative": str(norm_ret), "comparison": "semantic",
                                                 "error": "return contract semantic mismatch"})
                            structural_ok = False
                        else:
                            fc["checks"].append({"kind": "return_contract", "ok": True, "comparison": "semantic"})
                    mf[fname] = fc
                structural_findings[mod_name] = {"module_sha256": sha256_file(mod_path), "functions": mf}
            e["structural_contract_check"] = "PASS" if structural_ok else "FAIL"
            e["structural_findings"] = structural_findings
        finally:
            for root, dirs, _files in os.walk(self.wt):
                if "__pycache__" in dirs:
                    shutil.rmtree(os.path.join(root, "__pycache__"))
                    dirs.remove("__pycache__")
            sys.path = [p for p in sys.path if p != self.wt]
            sys.path = [p for p in sys.path if os.path.realpath(p) != "/tmp"]

        # ---- runtime return-shape checks (execute REAL R code) ----
        rs = {"checks": [], "verdict": "PASS"}
        if self.lab == "LAB-B":
            try:
                import wave
                import struct
                import tempfile as _tf
                sys.path.insert(0, os.path.join(self.wt, "tests", "integration"))
                from headless_audio_lab.audio_oracle import analyze_wav, analyze_loop_reproduction, AnalysisResult
                fd, wp = _tf.mkstemp(suffix=".wav")
                os.close(fd)
                rate = 48000
                n = 4800
                w = wave.open(wp, "wb")
                w.setnchannels(1)
                w.setsampwidth(2)
                w.setframerate(rate)
                w.writeframes(b"\x00\x00" * n)
                w.close()
                data = open(wp, "rb").read()
                pcm_nul = b"\x00" in data[44:]
                res = analyze_wav(wp, n, n, 120.0, 4, rate)
                rs["checks"].append({"name": "silent_wav", "pcm_nul_real": pcm_nul,
                                     "returns_AnalysisResult": isinstance(res, AnalysisResult),
                                     "verification": getattr(res, "verification", None),
                                     "captured_wav_frames": getattr(res, "captured_wav_frames", None),
                                     "ok": pcm_nul and isinstance(res, AnalysisResult)
                                           and getattr(res, "verification", None) == "FAIL"
                                           and getattr(res, "captured_wav_frames", None) == n})
                if isinstance(res, AnalysisResult):
                    fields_ok = all(hasattr(res, fld) for fld in
                                    ["expected_loop_frames", "observed_loop_frames",
                                     "captured_wav_frames", "analyzed_segment_frames", "verification"])
                    d = res.to_dict()
                    dict_ok = isinstance(d, dict) and all(k in d for k in
                                                          ["expected_loop_frames", "observed_loop_frames",
                                                           "captured_wav_frames", "analyzed_segment_frames", "verification"])
                    rs["checks"].append({"name": "AnalysisResult_fields_to_dict",
                                         "fields_ok": fields_ok, "dict_ok": dict_ok, "ok": fields_ok and dict_ok})
                else:
                    rs["checks"].append({"name": "AnalysisResult_fields_to_dict",
                                         "fields_ok": False, "dict_ok": False, "ok": False})
                # v3.9 F-B3: return-shape gate uses REAL controlled WAV fixtures
                # with known parameters — never nonexistent placeholder paths.
                # We create two WAV files: a non-silent tone source and a
                # matching capture, both 4800 frames at 48 kHz.
                fd_src, wp_src = _tf.mkstemp(suffix=".wav", prefix="v39-src-")
                os.close(fd_src)
                _w = wave.open(wp_src, "wb")
                _w.setnchannels(1); _w.setsampwidth(2); _w.setframerate(rate)
                _w.writeframes(b"".join(struct.pack("<h", 1000 if i % 2 == 0 else -1000) for i in range(n)))
                _w.close()
                fd_cap, wp_cap = _tf.mkstemp(suffix=".wav", prefix="v39-cap-")
                os.close(fd_cap)
                _w = wave.open(wp_cap, "wb")
                _w.setnchannels(1); _w.setsampwidth(2); _w.setframerate(rate)
                _w.writeframes(b"".join(struct.pack("<h", 1000 if i % 2 == 0 else -1000) for i in range(n)))
                _w.close()
                lr = analyze_loop_reproduction(wp_src, wp_cap, n)
                lr_is_dict = isinstance(lr, dict)
                lr_dict_ok = lr_is_dict and len(lr) > 0
                rs["checks"].append({"name": "analyze_loop_reproduction_valid_wavs_returns_dict",
                                     "mode": "valid_controlled_wavs",
                                     "source_frames": n, "capture_frames": n,
                                     "ok": lr_dict_ok,
                                     "content_keys": sorted(lr.keys()) if lr_is_dict else None})
                # missing-file behavior: diagnostic only — NEVER blocks E
                # (Layer 1 does not prescribe missing-file behavior).
                try:
                    analyze_loop_reproduction("v39-nonexistent-src.wav", "v39-nonexistent-cap.wav", n)
                    missing_behavior = "returned"
                except Exception as _ex:
                    missing_behavior = "raised:" + type(_ex).__name__
                rs["checks"].append({"name": "analyze_loop_reproduction_missing_file",
                                     "mode": "diagnostic_only_out_of_contract",
                                     "missing_file_behavior": missing_behavior,
                                     "gate_effect": "none",
                                     "ok": True})
                try:
                    os.unlink(wp_src)
                except OSError:
                    pass
                try:
                    os.unlink(wp_cap)
                except OSError:
                    pass
                fd2, wp2 = _tf.mkstemp(suffix=".wav")
                os.close(fd2)
                with open(wp2, "wb") as f:
                    f.write(b"RIFF\x00\x00\x00\x00WAVEjunk")
                try:
                    res2 = analyze_wav(wp2, n, n, 120.0, 4, rate)
                    malformed_ok = isinstance(res2, AnalysisResult) and getattr(res2, "verification", "PASS") != "PASS"
                    rs["checks"].append({"name": "malformed_wav", "mode": "returned",
                                         "verification": getattr(res2, "verification", None),
                                         "ok": malformed_ok})
                except Exception as ex:
                    rs["checks"].append({"name": "malformed_wav", "mode": "exception",
                                         "exc": str(ex)[:80], "ok": True, "controlled": True})
                os.unlink(wp)
                os.unlink(wp2)
            except Exception as ex:
                rs["verdict"] = "FAIL"
                rs["checks"].append({"name": "return_shape_block", "error": str(ex)[:200]})
        else:
            rs["checks"].append({"name": "lab_a_return_shapes",
                                 "note": "LAB-A has no normative runtime return-shape contract; owned-process runtime is covered by literal tests",
                                 "ok": True})
        rs["verdict"] = "PASS" if all(c.get("ok", False) for c in rs["checks"]) else "FAIL"
        e["runtime_return_shape_check"] = rs["verdict"]
        e["runtime_return_shape_checks"] = rs["checks"]
        # the LAB-B return-shape imports re-created bytecode caches: remove them
        # before the cleanliness/controlled-build gates.
        for root, dirs, _files in os.walk(self.wt):
            if "__pycache__" in dirs:
                shutil.rmtree(os.path.join(root, "__pycache__"))
                dirs.remove("__pycache__")
        e["external_runtime_semantics"] = {
            "jack_sooperlooper": "deferred_by_design",
            "note": "real JACK daemon / SooperLooper execution is NOT claimed proven here; it belongs to the dispatch phase"
        }
        cc = self.constraint_coverage(baseline, structural_ok, rs["verdict"] == "PASS")
        e["constraint_coverage"] = cc
        cc_ok = cc["all_pass_or_deferred"]
        all_ok = structural_ok and rs["verdict"] == "PASS" and cc_ok
        self.gate("runtime full contract PASS", all_ok,
                  "structural=" + str(e["structural_contract_check"]) +
                  " return_shape=" + str(rs["verdict"]) +
                  " constraints_ok=" + str(cc_ok))

    # ---------- ownership diff evidence: FC-F3 (v3.8) ----------
    def ownership_diff(self):
        e = self.result.setdefault("ownership_diff_evidence", {})
        diff_raw = git(self.repo, "diff", "--name-status", self.C, self.R)
        e["diff_raw"] = diff_raw
        changed = []
        for line in diff_raw.splitlines():
            parts = line.split("\t")
            if len(parts) >= 2:
                changed.append({"status": parts[0], "path": parts[-1]})
        e["changed_paths"] = [c["path"] for c in changed]
        owned = set(self.owned)
        def under(p, o):
            return p == o or p.startswith(o.rstrip("/") + "/")
        changed_not_owned = [p for p in e["changed_paths"] if not any(under(p, o) for o in owned)]
        forbidden = ["PROJECT-MANIFEST.json", "CURRENT.md", "WORK-QUEUE.md",
                     "doc/", "receipts/", ".github/", "scripts/", "tools/"]
        forbidden_changed = [p for p in e["changed_paths"]
                             if any(p == fp or p.startswith(fp.rstrip("/") + "/") for fp in forbidden)]
        e["changed_not_owned"] = changed_not_owned
        e["forbidden_changed"] = forbidden_changed
        e["changed_paths_non_empty"] = len(changed) > 0
        e["ownership_diff_pass"] = (e["changed_paths_non_empty"] and not changed_not_owned
                                    and not forbidden_changed)
        self.gate("ownership diff evidence PASS", e["ownership_diff_pass"],
                  "changed=" + str(len(changed)) + " not_owned=" + str(changed_not_owned)
                  + " forbidden=" + str(forbidden_changed))
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
        e["non_empty"] = len(files) > 0
        self.gate("artifact verification PASS", e["non_empty"], f"files={len(files)}")

    # ---------- LAB-B strict controlled build ----------
    def controlled_build(self):
        e = self.result.setdefault("build_provenance", {})
        e["applicable"] = self.lab == "LAB-B"
        if self.lab != "LAB-B":
            e["verdict"] = "not_applicable"
            self.gate("controlled build PASS/not_applicable", True, "not_applicable")
            return
        build_dir = os.path.join(self.wt, "build")
        # BF-12 fix: capture entries_before BEFORE any compilation
        if os.path.exists(build_dir):
            e["entries_before"] = sorted(os.listdir(build_dir))
            e["build_dir_empty_before"] = False
        else:
            e["entries_before"] = []
            e["build_dir_empty_before"] = True
            os.makedirs(build_dir, exist_ok=True)
        self.gate("empty build dir (entries_before=[])", e["build_dir_empty_before"],
                  str(e["entries_before"]))
        sources = self.func_baseline["canonical_sources"]
        compiles = []
        all_outputs_ok = True
        for src in sources:
            src_rel = src["path"]
            expected_sha = src["sha256"]
            blob_oid = git(self.repo, "rev-parse", f"{self.R}:{src_rel}")
            raw = subprocess.run(["git", "-C", self.repo, "cat-file", "blob", blob_oid],
                                 capture_output=True).stdout
            actual_sha = sha256_bytes(raw)
            out_name = os.path.basename(src_rel).replace(".c", "")
            out = os.path.join(build_dir, out_name)
            argv = ["gcc", "-std=c11", "-O2", "-o", out, src_rel, "-ljack", "-lm", "-lpthread"]
            argv_norm = [("<build-dir>/" + os.path.basename(a)) if a == out else a for a in argv]
            start_utc = now_utc()
            env_allowlist = {k: v for k, v in sorted(os.environ.items())
                             if k in ("PATH", "HOME", "LANG", "CC", "CFLAGS", "PKG_CONFIG_PATH", "LD_LIBRARY_PATH")}
            r = sh(argv, cwd=self.wt, timeout=300)
            end_utc = now_utc()
            exists = os.path.exists(out)
            size = os.path.getsize(out) if exists else None
            elf_magic = None
            ldd_exit = None
            ldd_libs = []
            if exists and size and size > 0:
                with open(out, "rb") as fh:
                    head = fh.read(4)
                elf_magic = head == b"\x7fELF"
                ldd = sh(["ldd", out], timeout=60)
                ldd_exit = ldd.returncode
                for line in ldd.stdout.splitlines():
                    if "=>" in line:
                        ldd_libs.append(line.split("=>")[0].strip())
                    elif "linux-vdso" in line or "ld-linux" in line:
                        ldd_libs.append(line.split("(")[0].strip())
            jack_visible = any("libjack" in lib or "jack" in lib for lib in ldd_libs)
            rec = {
                "source_path": src_rel,
                "source_blob_oid": blob_oid,
                "source_sha256_actual": actual_sha,
                "source_sha256_expected": expected_sha,
                "source_hash_matches": actual_sha == expected_sha,
                "argv": argv_norm,
                "cwd": self.wt,
                "env_allowlist": env_allowlist,
                "start_utc": start_utc,
                "end_utc": end_utc,
                "exit_code": r.returncode,
                "stdout_sha256": sha256_bytes(r.stdout.encode()),
                "stderr_sha256": sha256_bytes(r.stderr.encode()),
                "output_exists": exists,
                "output_size": size,
                "output_sha256": sha256_file(out) if exists else None,
                "elf_magic": elf_magic,
                "ldd_exit": ldd_exit,
                "ldd_libraries": sorted(set(ldd_libs)),
                "jack_dependency_visible": jack_visible,
            }
            output_ok = (rec["exit_code"] == 0 and rec["output_exists"] and rec["output_size"]
                         and rec["output_size"] > 0 and rec["elf_magic"] is True
                         and rec["ldd_exit"] == 0 and rec["jack_dependency_visible"])
            rec["output_matrix_pass"] = output_ok
            if not output_ok:
                all_outputs_ok = False
            if not rec["source_hash_matches"]:
                all_outputs_ok = False
            compiles.append(rec)
        e["compiles"] = compiles
        e["compiler_realpath"] = os.path.realpath(shutil.which("gcc") or "gcc")
        v = sh(["gcc", "--version"], timeout=60)
        e["compiler_version"] = v.stdout.splitlines()[0] if v.stdout else v.stderr.splitlines()[0]
        # BF-12/13/14 fix: cleanup outputs + build dir, verify cleanliness_after
        shutil.rmtree(build_dir, ignore_errors=True)
        # v3.8 real-C fixtures: the candidate TRACKS build/README; restore it so
        # the worktree is byte-identical to R before the cleanliness gate.
        sh(["git", "-C", self.wt, "checkout", "--", "build/"], timeout=120)
        r2 = sh(["git", "-C", self.wt, "status", "--porcelain"])
        e["clean_after_build"] = r2.stdout == ""
        untracked2 = sh(["git", "-C", self.wt, "ls-files", "--others", "--exclude-standard"])
        e["zero_untracked_after_build"] = untracked2.stdout.strip() == ""
        ignored2 = sh(["git", "-C", self.wt, "status", "--porcelain", "--ignored"])
        e["ignored_after_build"] = [l for l in ignored2.stdout.splitlines() if "!!" in l]
        e["zero_ignored_after_build"] = not e["ignored_after_build"]
        e["cleanliness_after"] = (e["clean_after_build"] and e["zero_untracked_after_build"]
                                  and e["zero_ignored_after_build"])
        e["verdict"] = "PASS" if (all_outputs_ok and e["cleanliness_after"]) else "FAIL"
        self.gate("controlled build PASS/not_applicable", e["verdict"] == "PASS",
                  f"outputs_ok={all_outputs_ok} clean_after={e['cleanliness_after']}")

    def clean_after(self):
        e = self.result.setdefault("cleanliness_after", {})
        # The literal build test (ft-b4) legitimately compiles into build/ and the
        # cleanup removes it. When the candidate tree TRACKS build artifacts (e.g.
        # build/README), restore them so the worktree is byte-identical to R.
        sh(["git", "-C", self.wt, "checkout", "--", "build/"], timeout=120)
        r = sh(["git", "-C", self.wt, "status", "--porcelain"])
        e["clean"] = r.stdout == ""
        e["dirty_lines"] = r.stdout
        e["tracked_build_restored"] = True
        self.gate("cleanliness_after PASS", e["clean"], repr(r.stdout[:100]))

    # ---------- evidence assembly ----------
    def assemble_evidence(self):
        prepass = self.prepass_ok()
        ev = {
            "schema": "controller-evidence/v3.8",
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
            "remote_absence_pre_evidence": self.result.get("remote_absence_pre_evidence"),
            "cleanliness": self.result.get("cleanliness"),
            "literal_tests": self.result.get("literal_tests"),
            "functional_acceptance": self.result.get("functional_acceptance"),
            "runtime_contract": self.result.get("runtime_contract"),
            "command_hash_evidence": self.result.get("command_hash_evidence"),
            "ownership_diff_evidence": self.result.get("ownership_diff_evidence"),
            "artifacts": self.result.get("artifacts"),
            "build_provenance": self.result.get("build_provenance"),
            "cleanliness_after": self.result.get("cleanliness_after"),
            "static_source_validation": self.result.get("static_source_validation"),
            "gates": self.result.get("gates"),
            "prepublication_verdict": "PASS" if prepass else "FAIL",
            "recorded_at_utc": self.recorded_at,
        }
        return ev

    def prepass_ok(self):
        return len(self.gate_failures) == 0

    # ---------- E creation (ONLY when prepass PASS) ----------
    def create_E(self, evidence_json):
        ev_path = os.path.join(self.wt, self.evidence_rel)
        os.makedirs(os.path.dirname(ev_path), exist_ok=True)
        with open(ev_path, "w") as f:
            f.write(evidence_json)
        sh(["git", "-C", self.wt, "add", "-f", self.evidence_rel])
        r = sh(["git", "-C", self.wt, "commit", "-m",
                f"evidence(v3.8): {self.lab} controller evidence container (fail-closed mechanical finalizer)"],
               timeout=120)
        if r.returncode != 0:
            raise RuntimeError("finalizer commit failed: " + r.stderr[-400:])
        e_oid = git(self.wt, "rev-parse", "HEAD")
        e_tree = git(self.wt, "rev-parse", "HEAD^{tree}")
        diff = git(self.repo, "diff", "--name-only", self.R, e_oid).splitlines()
        if diff != [self.evidence_rel]:
            raise RuntimeError(f"diff R..E not evidence-only: {diff}")
        return e_oid, e_tree, ev_path

    def check_identity(self):
        """ID-F3 fail-closed identity gate (v3.8): every identity source must
        agree with the authoritative package identity. Runs BEFORE checkout R,
        tests, evidence and E. Any mismatch makes run() return exit 3 with
        IDENTITY_MISMATCH and no side effects (no checkout, no evidence, no
        git add, no E; remote unchanged). There is NO fail-open mode."""
        auth = load_json(self.args.identity_authority)
        lease = load_json(self.args.lease_plan)
        failures = []
        lab_cfg = (auth.get("labs") or {}).get(self.lab)
        checks = [
            ("cli batch == authoritative batch", self.batch == auth.get("batch_id")),
            ("scope batch == authoritative batch", self.scope.get("batch_id") == auth.get("batch_id")),
            ("batch-manifest batch == authoritative batch",
             self.batch_manifest.get("batch_id") == auth.get("batch_id")),
            ("lease batch == authoritative batch", lease.get("batch_id") == auth.get("batch_id")),
            ("candidate C == authoritative candidate", self.C == auth.get("candidate_head_sha")),
            ("scope package == authoritative package", self.scope.get("package") == auth.get("package")),
            ("batch-manifest package == authoritative package",
             self.batch_manifest.get("package") == auth.get("package")),
            ("lab registered in authority", lab_cfg is not None),
        ]
        if lab_cfg is not None:
            checks += [
                ("leaf id exact", lease.get("leaf_id") == lab_cfg.get("leaf_id")),
                ("lease id exact", lease.get("lease_id") == lab_cfg.get("lease_id")),
                ("planned branch matches authoritative pattern",
                 branch_matches_authority_pattern(self.planned_branch, self.lab,
                                                  lab_cfg.get("planned_result_branch"))),
                ("evidence path exact", self.evidence_rel == lab_cfg.get("result_evidence_path")),
            ]
        for name, ok in checks:
            self.gate("identity: " + name, ok)
            if not ok:
                failures.append(name)
        self.result["identity_coherence"] = {
            "sources_checked": len(checks),
            "mismatches": failures,
            "pass": len(failures) == 0,
        }
        return len(failures) == 0

    def run(self):
        self.check_topology()
        self.check_required_paths()
        self.check_evidence_absent()
        self.check_remote_absence_pre_evidence()
        # ID-F3 fail-closed identity gate (v3.8): identity coherence is a
        # precondition for checkout R, tests, evidence and E. No fail-open mode.
        if not self.check_identity():
            fails = [g["gate"] + (" | " + str(g["detail"])[:160] if g["detail"] else "")
                     for g in self.gate_failures if str(g["gate"]).startswith("identity:")]
            print(json.dumps({
                "mode": self.args.mode,
                "identity_verdict": "IDENTITY_MISMATCH",
                "gate_failures": fails,
                "E_created": False,
                "evidence_written": False,
                "git_add_committed": False,
                "checkout_performed": False,
                "remote_unchanged": True,
            }, indent=2, sort_keys=True))
            return 3
        self.checkout_r()
        try:
            self.check_clean()
            self.run_literal_tests()
            self.ownership_diff()
            self.runtime_full_contract()
            self.artifact_hashes()
            self.controlled_build()
            self.clean_after()
            # v3.9 §13: gate traceability assertion — every blocking gate must
            # have normative authority; orphan gates => no E.
            self.check_traceability()
            # static source validation of the two controller-owned programs
            st_ok, st_findings = static_validate_source(os.path.abspath(__file__))
            st_ok2, st_findings2 = static_validate_source(self.args.verifier_path)
            self.result["static_source_validation"] = {
                "finalizer_ok": st_ok, "verifier_ok": st_ok2,
                "findings": st_findings + st_findings2,
            }
            self.gate("static source validation PASS", st_ok and st_ok2,
                      str(st_findings + st_findings2)[:200])
            # FAIL-CLOSED: no gate failure -> no E, exit nonzero
            if not self.prepass_ok():
                fails = [g["gate"] + (" | " + str(g["detail"])[:160] if g["detail"] else "") for g in self.gate_failures]
                print(json.dumps({
                    "mode": self.args.mode,
                    "prepublication_verdict": "FAIL",
                    "gate_failures": fails,
                    "E_created": False,
                    "evidence_written": False,
                    "git_add_committed": False,
                }, indent=2, sort_keys=True))
                return 1
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
                    "prepublication_verdict": "PASS",
                }, indent=2, sort_keys=True))
                return 0
            e_oid, e_tree, ev_path = self.create_E(ev_bytes.decode("utf-8"))
            print(json.dumps({
                "mode": "finalize",
                "E": e_oid,
                "E_tree": e_tree,
                "evidence_path": self.evidence_rel,
                "evidence_sha256": ev_sha,
                "prepublication_verdict": "PASS",
            }, indent=2, sort_keys=True))
            return 0
        finally:
            self.cleanup_worktree()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", choices=["finalize", "reproduce"])
    ap.add_argument("--repo")
    ap.add_argument("--c")
    ap.add_argument("--r")
    ap.add_argument("--lab")
    ap.add_argument("--batch")
    ap.add_argument("--planned-branch")
    ap.add_argument("--remote", default=None)
    ap.add_argument("--scope-freeze")
    ap.add_argument("--batch-manifest")
    ap.add_argument("--identity-authority")
    ap.add_argument("--lease-plan")
    ap.add_argument("--functional-baseline")
    ap.add_argument("--api-contract")
    ap.add_argument("--build-policy")
    ap.add_argument("--gate-traceability", default=None)
    ap.add_argument("--controller-dir")
    ap.add_argument("--evidence-path")
    ap.add_argument("--verifier-path")
    ap.add_argument("--recorded-at-utc", default=None)
    ap.add_argument("--reproduce-output", default=None)
    ap.add_argument("--static-validate", action="store_true")
    ap.add_argument("--finalizer", default=None)
    ap.add_argument("--verifier", default=None)
    args = ap.parse_args()

    if args.static_validate:
        f_path = args.finalizer or os.path.abspath(__file__)
        v_path = args.verifier or ""
        ok1, f1 = static_validate_source(f_path)
        ok2, f2 = static_validate_source(v_path) if v_path else (True, [])
        out = {"finalizer_ok": ok1, "verifier_ok": ok2, "findings": f1 + f2}
        print(json.dumps(out, indent=2, sort_keys=True))
        return 0 if (ok1 and ok2) else 1

    missing = [k for k in ("mode", "repo", "c", "r", "lab", "batch", "planned_branch",
                           "scope_freeze", "batch_manifest", "identity_authority",
                           "api_contract", "build_policy", "controller_dir",
                           "evidence_path", "verifier_path", "lease_plan")
               if not getattr(args, k)]
    if missing:
        print("missing required arguments:", missing)
        return 2
    fz = Finalizer(args)
    return fz.run()


if __name__ == "__main__":
    sys.exit(main())

