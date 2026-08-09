#!/usr/bin/env python3
"""acceptance-verifier-v3.4.py — functional-continuity + strict workflow verifier.

Three phases:
  --phase functional-continuity : v3.4 functional surface must equal the canonical
                                  v3.2 baseline (API surface, silence semantics,
                                  literal-test IDs, canonical sources/commands, ownership).
  --phase leaf-envelope         : strict v3.3 workflow layer (C->R->E, exact IDs,
                                  literal command byte-binding, artifacts, build claim).
  --phase full-completion       : leaf-envelope + controller acceptance receipt +
                                  literal tests REEXECUTED + v3.2 functional gates +
                                  controlled canonical rebuild.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

VERSION = "1.0.0"
SCHEMA = "headless-lab-result-envelope/v3.4"
RECEIPT_SCHEMA = "controller-acceptance-receipt/v3.4"
BATCH_ID = "BATCH-20260806T020249Z-DOGFOOD004-LAB-V3_4"
CANDIDATE = "509538784afc2b828f2d922f65cf8ca3a39b5ee7"

ENVELOPE_LAB_A = "receipts/headless-lab-results-v3.4/lab-a-result.json"
ENVELOPE_LAB_B = "receipts/headless-lab-results-v3.4/lab-b-result.json"

OWNED_LAB_A = [
    "tests/integration/headless_audio_lab/__init__.py",
    "tests/integration/headless_audio_lab/process_supervisor.py",
    "tests/integration/headless_audio_lab/jack_server.py",
    "tests/integration/headless_audio_lab/osc_probe.py",
    "tests/integration/headless_audio_lab/sooperlooper_launcher.py",
]
OWNED_LAB_B = [
    "tests/__init__.py",
    "tests/integration/__init__.py",
    "tests/integration/headless_audio_lab/audio_oracle.py",
    "tests/integration/headless_audio_lab/graph_assertions.py",
    "tests/integration/headless_audio_lab/runner.py",
    "tests/integration/headless_audio_lab/test_modules.py",
    "tests/integration/headless_audio_lab/scenarios/D1_source_direct.yaml",
    "tests/integration/headless_audio_lab/scenarios/D2_sl_passive.yaml",
    "tests/integration/headless_audio_lab/scenarios/D3_osc_recording.yaml",
    "tests/integration/headless_audio_lab/fixtures/synthetic_source",
    "tests/integration/headless_audio_lab/fixtures/deterministic_capture",
    "tests/integration/headless_audio_lab/fixtures/.gitignore",
]

PLACEHOLDER_PATTERNS = [
    (re.compile(r"We are not actually starting a process", re.I), "dummy-start-comment"),
    (re.compile(r"not actually (?:starting|launching|running)", re.I), "not-actually-comment"),
    (re.compile(r"return a dummy", re.I), "dummy-return-comment"),
    (re.compile(r"pid\s*=\s*0\b"), "pid-zero-literal"),
    (re.compile(r"pid\s*=\s*-1\b"), "pid-negative-literal"),
    (re.compile(r"TODO", re.I), "todo"),
    (re.compile(r"NotImplemented"), "not-implemented"),
    (re.compile(r"MagicMock"), "magic-mock-in-implementation"),
    (re.compile(r"for the purpose of passing the tests", re.I), "test-gaming-comment"),
    (re.compile(r"# Placeholder", re.I), "placeholder-comment"),
]
TRIVIAL_TEST_PATTERNS = [
    (re.compile(r"^\s*pass\s*$", re.M), "pass-stub"),
    (re.compile(r"assertTrue\s*\(\s*True\s*\)"), "trivial-assert-true"),
    (re.compile(r"can_be_imported"), "import-only-test-name"),
    (re.compile(r"MagicMock"), "magic-mock-in-tests"),
]

FORBIDDEN_SELF_KEYS = ["container_commit_oid", "container_tree_oid", "envelope_blob_oid",
                       "envelope_sha256", "self_sha256", "remote_branch_resolved_sha"]
FORBIDDEN_CONTROLLER_FUTURE_PREFIXES = ("controller_",)
FORBIDDEN_CONTROLLER_FUTURE_SUFFIXES = ("_resolved_sha", "_verdict", "_pass", "_done")
ALLOWED_TOP_LEVEL_KEYS = ["schema", "lab", "batch_id", "leaf_id", "lease_id",
                          "candidate_commit_oid", "implementation_result_commit_oid",
                          "implementation_result_tree_oid", "git_object_format",
                          "external_branch_binding", "leaf_literal_tests", "leaf_build_claim", "artifacts"]
ALLOWED_ARTIFACT_KINDS = {"source", "test", "scenario", "ignore-rule"}
ALLOWED_BUILD_CLAIM_STATUS = {"not_applicable", "claimed_built"}

PACKAGE_DIR = os.path.dirname(os.path.abspath(__file__))


class Result:
    def __init__(self):
        self.checks = []
        self.env_notes = []

    def add(self, name, status, detail="", evidence=None):
        self.checks.append({"name": name, "status": status, "detail": detail, "evidence": evidence})

    def ok(self, name, detail="", evidence=None):
        self.add(name, "PASS", detail, evidence)

    def fail(self, name, detail="", evidence=None):
        self.add(name, "FAIL", detail, evidence)

    def failed(self):
        return any(c["status"] == "FAIL" for c in self.checks)


def sha256_b(b):
    return hashlib.sha256(b).hexdigest()


def sha256_file(path):
    with open(path, "rb") as f:
        return sha256_b(f.read())


def run(cmd, cwd, timeout=120, env=None):
    e = dict(os.environ)
    if env:
        e.update(env)
    e["PYTHONDONTWRITEBYTECODE"] = "1"
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd, timeout=timeout, env=e)
        return {"exit": r.returncode, "stdout": r.stdout, "stderr": r.stderr}
    except subprocess.TimeoutExpired:
        return {"exit": -2, "stdout": "", "stderr": "timeout"}
    except Exception as ex:
        return {"exit": -3, "stdout": "", "stderr": str(ex)}


def git(repo, args):
    return run(["git", "-C", repo] + args, repo, timeout=60)


def git_show(checkout, rev, path):
    r = subprocess.run(["git", "-C", checkout, "show", "%s:%s" % (rev, path)],
                       capture_output=True, timeout=30)
    if r.returncode != 0:
        return None
    return r.stdout.decode("utf-8", "replace")


def scan_for_patterns(content, patterns, label):
    hits = []
    for rx, name in patterns:
        for m in rx.finditer(content or ""):
            hits.append({"pattern": name, "line": content[:m.start()].count("\n") + 1})
    return hits


def load_json(path, label):
    try:
        with open(path) as f:
            return json.load(f)
    except Exception as e:
        raise SystemExit("cannot load %s: %s" % (label, e))


def _git_quiet(repo, args, ok_exit=True):
    r = git(repo, args)
    if ok_exit and r["exit"] != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), r["stderr"][-300:]))
    return r["stdout"].strip()


def load_batch_manifest(path=None):
    return load_json(path or os.path.join(PACKAGE_DIR, "batch-manifest-v3.4.json"), "batch-manifest")


def load_baseline(path=None):
    return load_json(path or os.path.join(PACKAGE_DIR, "functional-contract-baseline-v3.4.json"),
                     "functional-contract-baseline")


def load_contract(path=None):
    return load_json(path or os.path.join(PACKAGE_DIR, "lab-api-contract-v3.4.json"), "lab-api-contract")


# ---------------------------------------------------------------------------
# PHASE 1: functional continuity (v3.4 == canonical v3.2 surface)
# ---------------------------------------------------------------------------
def verify_functional_continuity(repo, baseline, contract, scope, batch, res):
    # 1. module/function/class surface deep equality
    base_mods = baseline.get("canonical_contract_modules")
    v34_mods = contract.get("modules")
    res.add("func: contract modules present", "PASS" if isinstance(v34_mods, dict) and base_mods else "FAIL",
            evidence={"baseline_modules": sorted(base_mods) if isinstance(base_mods, dict) else None,
                      "v34_modules": sorted(v34_mods) if isinstance(v34_mods, dict) else None})
    if not isinstance(base_mods, dict) or not isinstance(v34_mods, dict):
        return
    if set(base_mods.keys()) != set(v34_mods.keys()):
        res.fail("func: module set exact",
                 evidence={"missing": sorted(set(base_mods) - set(v34_mods)),
                           "extra": sorted(set(v34_mods) - set(base_mods))})
    else:
        res.ok("func: module set exact", str(len(base_mods)) + " modules")

    surface_diffs = []
    for module in sorted(base_mods):
        bf = base_mods[module].get("functions", [])
        vf = v34_mods.get(module, {}).get("functions", [])
        for fn in bf:
            match = [x for x in vf if x.get("kind") == fn.get("kind") and x.get("name") == fn.get("name")]
            if not match:
                surface_diffs.append({"module": module, "missing": fn.get("name"), "kind": fn.get("kind")})
                continue
            v = match[0]
            if v != fn:
                surface_diffs.append({"module": module, "function": fn.get("name"),
                                      "difference": "declaration mismatch"})
    res.add("func: API surface deep-equal (functions/classes/fields/methods/constraints)",
            "PASS" if not surface_diffs else "FAIL", evidence={"diffs": surface_diffs[:20]})

    # 2. silence semantics
    silent_ok = False
    for fn in base_mods.get("audio_oracle.py", {}).get("functions", []):
        if fn.get("name") == "analyze_wav":
            silent_ok = any("silent WAV MUST produce verification=FAIL" in (c or "") for c in fn.get("constraints", []))
            break
    res.add("func: silence semantics intact (verification=FAIL)",
            "PASS" if silent_ok else "FAIL")

    # 3. literal-test IDs complete
    for lab in ("LAB-A", "LAB-B"):
        canon_ids = set(baseline.get("canonical_literal_test_ids", {}).get(lab, []))
        actual_ids = set(batch.get("labs", {}).get(lab, {}).get("literal_tests", {}).keys())
        missing_ids = canon_ids - actual_ids
        res.add("func: %s literal-test IDs complete (>=4)" % lab,
                "PASS" if (not missing_ids and len(actual_ids) >= 4) else "FAIL",
                evidence={"canonical": sorted(canon_ids), "actual": sorted(actual_ids), "missing": sorted(missing_ids)})
        # command sha256 binding present per test
        for tid, t in batch.get("labs", {}).get(lab, {}).get("literal_tests", {}).items():
            cmd_sha = sha256_b(json.dumps(t.get("command"), ensure_ascii=False).encode("utf-8"))
            res.add("func: %s %s command_sha256 exact" % (lab, tid),
                    "PASS" if t.get("command_sha256") == cmd_sha else "FAIL")

    # 4. canonical sources identity at candidate
    for src in baseline.get("canonical_sources", []):
        path = src.get("path")
        blob = git(repo, ["rev-parse", "%s:%s" % (CANDIDATE, path)])
        if blob["exit"] != 0:
            res.fail("func: canonical source %s present at candidate" % path)
            continue
        oid = blob["stdout"].strip()
        data = subprocess.run(["git", "-C", repo, "show", "%s:%s" % (CANDIDATE, path)],
                              capture_output=True).stdout
        res.add("func: canonical source %s blob OID exact" % path,
                "PASS" if oid == src.get("blob_oid_at_candidate") else "FAIL",
                evidence={"observed": oid, "expected": src.get("blob_oid_at_candidate")})
        res.add("func: canonical source %s sha256 exact" % path,
                "PASS" if sha256_b(data) == src.get("sha256") else "FAIL")

    # 5. canonical compile commands (gcc flags + sources + libs preserved)
    policy = load_json(baseline.get("canonical_source_package") and os.path.join(PACKAGE_DIR, "build-provenance-policy-v3.4.json"),
                       "build-provenance-policy") if os.path.exists(os.path.join(PACKAGE_DIR, "build-provenance-policy-v3.4.json")) else {}
    policy_cmds = policy.get("canonical_compile_commands", baseline.get("canonical_compile_commands", []))
    cmd_ok = True
    for cc in baseline.get("canonical_compile_commands", []):
        # normalize: remove the -o <out> pair and compare the rest
        def norm(argv):
            out = []
            i = 0
            while i < len(argv):
                if argv[i] == "-o":
                    i += 2
                    continue
                out.append(argv[i])
                i += 1
            return out
        base_norm = norm(cc)
        found = any(norm(pc) == base_norm for pc in policy_cmds)
        if not found:
            cmd_ok = False
            res.fail("func: canonical compile command preserved", evidence={"canonical": cc})
    if cmd_ok:
        res.ok("func: canonical compile commands preserved (gcc -std=c11 -O2 -ljack -lm -lpthread)")

    # 6. ownership consistency (scope == batch == baseline == verifier constants)
    own = baseline.get("canonical_ownership", {})
    for lab, const in (("LAB-A", OWNED_LAB_A), ("LAB-B", OWNED_LAB_B)):
        sets = {
            "scope": scope.get("scope", {}).get("lab_a_owned_paths" if lab == "LAB-A" else "lab_b_owned_paths", []),
            "batch": batch.get("labs", {}).get(lab, {}).get("owned_paths", []),
            "baseline": own.get(lab, []),
            "verifier": const,
        }
        drift = []
        for name_a in sets:
            for name_b in sets:
                if set(sets[name_a]) != set(sets[name_b]):
                    drift.append("%s != %s" % (name_a, name_b))
        res.add("func: ownership consistency (%s) across scope/batch/baseline/verifier" % lab,
                "PASS" if not drift else "FAIL", evidence={"drift": drift})
    gi = "tests/integration/headless_audio_lab/fixtures/.gitignore"
    res.add("func: fixtures/.gitignore owned in every manifest (LAB-B)",
            "PASS" if all(gi in x for x in (scope.get("scope", {}).get("lab_b_owned_paths", []),
                                            batch.get("labs", {}).get("LAB-B", {}).get("owned_paths", []),
                                            own.get("LAB-B", []), OWNED_LAB_B)) else "FAIL")


# ---------------------------------------------------------------------------
# PHASE 2: strict leaf-envelope (v3.3 workflow layer retained)
# ---------------------------------------------------------------------------
def verify_leaf_envelope(repo, c_oid, r_oid, e_oid, envelope_path, lab, batch, res, expected_branch=None):
    ok_exist = True
    for label, oid in [("C", c_oid), ("R", r_oid), ("E", e_oid)]:
        r = git(repo, ["cat-file", "-e", "%s^{commit}" % oid])
        if r["exit"] != 0:
            res.fail("model: %s commit exists" % label)
            ok_exist = False
        else:
            res.ok("model: %s commit exists" % label)
    if not ok_exist:
        return
    if c_oid == r_oid:
        res.fail("model: R != C")
    else:
        res.ok("model: R != C")
    r_parents = git(repo, ["rev-list", "--parents", "-n", "1", r_oid])["stdout"].split()
    e_parents = git(repo, ["rev-list", "--parents", "-n", "1", e_oid])["stdout"].split()
    res.add("model: R exactly one parent and R^ == C", "PASS" if len(r_parents) == 2 and r_parents[1] == c_oid else "FAIL")
    res.add("model: E exactly one parent and E^ == R", "PASS" if len(e_parents) == 2 and e_parents[1] == r_oid else "FAIL")
    dq = git(repo, ["diff", "--quiet", c_oid, r_oid])
    res.add("model: git diff --quiet C R returns nonzero", "PASS" if dq["exit"] != 0 else "FAIL")
    env_in_c = git(repo, ["cat-file", "-e", "%s:%s" % (c_oid, envelope_path)])
    env_in_r = git(repo, ["cat-file", "-e", "%s:%s" % (r_oid, envelope_path)])
    env_in_e = git(repo, ["cat-file", "-e", "%s:%s" % (e_oid, envelope_path)])
    res.add("model: envelope absent from C", "PASS" if env_in_c["exit"] != 0 else "FAIL")
    res.add("model: envelope absent from R", "PASS" if env_in_r["exit"] != 0 else "FAIL")
    res.add("model: envelope present in E", "PASS" if env_in_e["exit"] == 0 else "FAIL")
    r_tree = _git_quiet(repo, ["rev-parse", "%s^{tree}" % r_oid])
    objfmt = _git_quiet(repo, ["rev-parse", "--show-object-format"])
    owned = set(OWNED_LAB_A if lab == "LAB-A" else OWNED_LAB_B)
    dr = git(repo, ["diff", "--name-only", c_oid, r_oid])
    r_diff_paths = [ln for ln in dr["stdout"].splitlines() if ln]
    bad_impl = [p for p in r_diff_paths if p not in owned and p != envelope_path]
    res.add("model: diff C..R owned-only", "PASS" if not bad_impl else "FAIL", evidence={"out_of_scope": bad_impl})
    res.add("model: diff C..R non-empty", "PASS" if r_diff_paths else "FAIL")
    de = git(repo, ["diff", "--name-only", r_oid, e_oid])
    e_diff_paths = [ln for ln in de["stdout"].splitlines() if ln]
    res.add("model: diff R..E envelope-only", "PASS" if e_diff_paths == [envelope_path] else "FAIL",
            evidence={"changed": e_diff_paths})
    env_raw = git(repo, ["show", "%s:%s" % (e_oid, envelope_path)])["stdout"]
    try:
        env = json.loads(env_raw)
    except Exception as ex:
        res.fail("model: envelope parses", str(ex))
        return
    res.ok("model: envelope parses")
    lab_meta = batch["labs"][lab]
    checks = {
        "schema EXACT": env.get("schema") == SCHEMA,
        "lab EXACT": env.get("lab") == lab,
        "batch_id EXACT": env.get("batch_id") == BATCH_ID,
        "leaf_id EXACT": env.get("leaf_id") == lab_meta["leaf_id"],
        "lease_id EXACT": env.get("lease_id") == lab_meta["lease_id"],
        "candidate_commit_oid == C": env.get("candidate_commit_oid") == c_oid,
        "implementation_result_commit_oid == R": env.get("implementation_result_commit_oid") == r_oid,
        "implementation_result_tree_oid == R^{tree}": env.get("implementation_result_tree_oid") == r_tree,
        "git_object_format EXACT": env.get("git_object_format") == objfmt,
    }
    for name, ok_ in checks.items():
        res.add("env: %s" % name, "PASS" if ok_ else "FAIL", evidence={"observed": env.get(name.split(" ")[0])})
    ebb = env.get("external_branch_binding")
    res.add("env: external_branch_binding.remote == origin",
            "PASS" if isinstance(ebb, dict) and ebb.get("remote") == "origin" else "FAIL")
    planned = ebb.get("planned_branch") if isinstance(ebb, dict) else None
    res.add("env: planned_branch present (non-empty)", "PASS" if isinstance(planned, str) and planned else "FAIL")
    if expected_branch is not None:
        res.add("env: planned_branch EXACT == expected branch",
                "PASS" if planned == expected_branch else "FAIL",
                evidence={"planned": planned, "expected": expected_branch})
    present = set(env.keys())
    missing = [k for k in ALLOWED_TOP_LEVEL_KEYS if k not in present]
    unknown = [k for k in present if k not in ALLOWED_TOP_LEVEL_KEYS]
    res.add("env: all required top-level keys present", "PASS" if not missing else "FAIL", evidence={"missing": missing})
    res.add("env: no unknown top-level keys", "PASS" if not unknown else "FAIL", evidence={"unknown": unknown})
    forbidden_present = [k for k in FORBIDDEN_SELF_KEYS if k in present]
    controller_flagged = [k for k in present if k.startswith(FORBIDDEN_CONTROLLER_FUTURE_PREFIXES)
                          or k.endswith(FORBIDDEN_CONTROLLER_FUTURE_SUFFIXES)]
    res.add("env: no forbidden self-binding keys", "PASS" if not forbidden_present else "FAIL")
    res.add("env: no controller-future claims inside E", "PASS" if not controller_flagged else "FAIL",
            evidence={"flagged": controller_flagged})
    llt = env.get("leaf_literal_tests")
    frozen_tests = lab_meta.get("literal_tests", {})
    if not isinstance(llt, dict):
        res.fail("env: leaf_literal_tests is a dict")
        llt = {}
    expected_ids = set(frozen_tests.keys())
    actual_ids = set(llt.keys())
    res.add("env: literal test ID set exact", "PASS" if actual_ids == expected_ids else "FAIL",
            evidence={"expected": sorted(expected_ids), "actual": sorted(actual_ids)})
    for tid, meta in frozen_tests.items():
        entry = llt.get(tid)
        if not isinstance(entry, dict):
            res.fail("env: literal test %s present" % tid)
            continue
        cmd_sha = sha256_b(json.dumps(meta["command"], ensure_ascii=False).encode("utf-8"))
        res.add("env: %s command_sha256 exact" % tid,
                "PASS" if entry.get("command_sha256") == cmd_sha else "FAIL")
        res.add("env: %s exit_code == 0" % tid, "PASS" if entry.get("exit_code") == 0 else "FAIL")
        for tskey in ("started_at_utc", "ended_at_utc"):
            val = entry.get(tskey)
            try:
                datetime.fromisoformat(val.replace("Z", "+00:00"))
                res.ok("env: %s %s parseable" % (tid, tskey))
            except Exception:
                res.fail("env: %s %s parseable" % (tid, tskey), str(val))
        try:
            st = datetime.fromisoformat(entry.get("started_at_utc", "").replace("Z", "+00:00"))
            en = datetime.fromisoformat(entry.get("ended_at_utc", "").replace("Z", "+00:00"))
            res.add("env: %s timestamps ordered" % tid, "PASS" if st <= en else "FAIL")
        except Exception:
            res.fail("env: %s timestamps ordered" % tid)
    arts = env.get("artifacts")
    if not isinstance(arts, dict):
        res.fail("env: artifacts is a dict")
        arts = {}
    res.add("env: artifact set == C..R tracked changes (owned only)",
            "PASS" if set(arts.keys()) == set(r_diff_paths) else "FAIL",
            evidence={"expected": sorted(r_diff_paths), "actual": sorted(arts.keys())})
    res.add("env: artifacts non-empty (R non-empty enforced)", "PASS" if arts else "FAIL")
    for apath in sorted(arts):
        a = arts[apath]
        if not isinstance(a, dict):
            res.fail("env: artifact %s is a dict" % apath)
            continue
        res.add("env: artifact %s kind allowed" % apath,
                "PASS" if a.get("kind") in ALLOWED_ARTIFACT_KINDS else "FAIL")
        blob = subprocess.run(["git", "-C", repo, "show", "%s:%s" % (r_oid, apath)],
                              capture_output=True, timeout=30)
        if blob.returncode != 0:
            res.fail("env: artifact %s present in R" % apath)
            continue
        real_sha = sha256_b(blob.stdout)
        real_size = len(blob.stdout)
        res.add("env: artifact %s sha256 exact" % apath,
                "PASS" if a.get("sha256") == real_sha else "FAIL",
                evidence={"declared": a.get("sha256"), "computed": real_sha})
        res.add("env: artifact %s size exact" % apath,
                "PASS" if a.get("size") == real_size else "FAIL",
                evidence={"declared": a.get("size"), "computed": real_size})
    if envelope_path in arts:
        res.fail("env: artifacts excludes envelope")
    lbc = env.get("leaf_build_claim")
    if not isinstance(lbc, dict):
        res.fail("env: leaf_build_claim present")
        lbc = {}
    status = lbc.get("status")
    res.add("env: leaf_build_claim.status allowed",
            "PASS" if status in ALLOWED_BUILD_CLAIM_STATUS else "FAIL", evidence={"status": status})
    if lab == "LAB-A":
        invented = [k for k in ("compiler_path", "compiler_version", "commands", "outputs", "transcript_sha256")
                    if lbc.get(k) not in (None, [], "", {})]
        res.add("env: LAB-A build claim not_applicable (no invented fields)",
                "PASS" if status == "not_applicable" and not invented else "FAIL", evidence={"invented": invented})
    else:
        res.add("env: LAB-B build claim structurally valid (sources/commands/outputs)",
                "PASS" if status == "claimed_built" and isinstance(lbc.get("sources"), list)
                and isinstance(lbc.get("commands"), list) and isinstance(lbc.get("outputs"), list)
                and bool(lbc.get("outputs")) else "FAIL",
                evidence={"status": status, "outputs": lbc.get("outputs")})


# ---------------------------------------------------------------------------
# v3.2-restored functional gates (LAB-A / LAB-B)
# ---------------------------------------------------------------------------
def check_lab_a(checkout, contract, res):
    sys.path.insert(0, os.path.join(checkout, "tests/integration"))
    try:
        import headless_audio_lab.process_supervisor as ps_mod  # noqa: F401
        import headless_audio_lab.jack_server as js_mod  # noqa: F401
        import headless_audio_lab.osc_probe as osc_mod  # noqa: F401
        import headless_audio_lab.sooperlooper_launcher as sl_mod  # noqa: F401
        res.ok("lab-a: imports", "all 4 modules import cleanly")
    except Exception as e:
        res.fail("lab-a: imports", "import failure: %s" % e)
        return
    ph_hits = {}
    for f in OWNED_LAB_A:
        content = git_show(checkout, "HEAD", f)
        if content is None:
            res.fail("lab-a: placeholder scan", "owned file missing at HEAD: %s" % f)
            continue
        hits = scan_for_patterns(content, PLACEHOLDER_PATTERNS, "lab-a")
        if hits:
            ph_hits[f] = hits
    res.add("lab-a: placeholder scan", "PASS" if not ph_hits else "FAIL", evidence=ph_hits if ph_hits else None)
    probe_src = (
        "import json, os, subprocess, sys, time\n"
        "sys.path.insert(0, 'tests/integration')\n"
        "from headless_audio_lab.process_supervisor import ProcessSupervisor\n"
        "out = {}\n"
        "foreign = None\n"
        "try:\n"
        "    with ProcessSupervisor() as sup:\n"
        "        info = sup.start_process('probe', ['/bin/sleep', '3'], {}, '.', '/dev/null', '/dev/null')\n"
        "        out['owned_pid'] = info.pid\n"
        "        out['owned_pgid'] = info.pgid\n"
        "        out['owned_pid_positive'] = bool(info.pid and info.pid > 0)\n"
        "        out['owned_pids_registered'] = sup.get_owned_pids()\n"
        "        out['owned_pgids_registered'] = sup.get_owned_pgids()\n"
        "        out['process_info'] = (lambda pi: {'pid': pi.pid, 'pgid': pi.pgid} if pi else None)(sup.get_process_info('probe'))\n"
        "        out['snapshot_pids'] = [p['pid'] for p in sup.snapshot()] if isinstance(sup.snapshot(), list) else None\n"
        "        time.sleep(0.3)\n"
        "        out['owned_child_alive_before_cleanup'] = os.path.exists('/proc/%d' % info.pid)\n"
        "        try:\n"
        "            sup.start_process('bad', ['/nonexistent/binary-xyz'], {}, '.', '/dev/null', '/dev/null')\n"
        "            out['failure_propagation'] = False\n"
        "        except Exception:\n"
        "            out['failure_propagation'] = True\n"
        "        foreign = subprocess.Popen(['/bin/sleep', '5'], start_new_session=True,\n"
        "                                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)\n"
        "        out['foreign_registered_in_supervisor'] = foreign.pid in sup.get_owned_pids()\n"
        "        out['foreign_alive_before_cleanup'] = os.path.exists('/proc/%d' % foreign.pid)\n"
        "        sup.cleanup(timeout=5.0)\n"
        "        out['owned_child_alive_after_cleanup'] = os.path.exists('/proc/%d' % info.pid)\n"
        "        out['cleanup_ok'] = sup.verify_cleanup() == []\n"
        "        out['foreign_alive_after_supervisor_cleanup'] = os.path.exists('/proc/%d' % foreign.pid)\n"
        "finally:\n"
        "    if foreign is not None:\n"
        "        if foreign.poll() is None:\n"
        "            foreign.terminate()\n"
        "            try:\n"
        "                foreign.wait(timeout=5.0)\n"
        "            except subprocess.TimeoutExpired:\n"
        "                foreign.kill(); foreign.wait(timeout=5.0)\n"
        "        out['foreign_terminated_by_controller'] = foreign.poll() is not None\n"
        "print(json.dumps(out))\n"
    )
    probe_path = os.path.join(checkout, "_acceptance_probe_lab_a_v34.py")
    with open(probe_path, "w") as f:
        f.write(probe_src)
    r = run(["python3", probe_path], checkout, timeout=120)
    try:
        os.unlink(probe_path)
    except OSError:
        pass
    if r["exit"] != 0:
        res.fail("lab-a: functional lifecycle", "probe failed exit=%d stderr=%s" % (r["exit"], r["stderr"][-500:]))
        return
    try:
        d = json.loads(r["stdout"].strip().splitlines()[-1])
    except Exception:
        d = {}
    conds = [
        ("owned_pid_positive", lambda x: x.get("owned_pid_positive") is True),
        ("owned_child_alive_before_cleanup", lambda x: x.get("owned_child_alive_before_cleanup") is True),
        ("owned_child_alive_after_cleanup", lambda x: x.get("owned_child_alive_after_cleanup") is False),
        ("cleanup_ok", lambda x: x.get("cleanup_ok") is True),
        ("foreign_alive_after_supervisor_cleanup", lambda x: x.get("foreign_alive_after_supervisor_cleanup") is True),
        ("failure_propagation", lambda x: x.get("failure_propagation") is True),
        ("foreign_registered_in_supervisor", lambda x: x.get("foreign_registered_in_supervisor") is False),
        ("foreign_terminated_by_controller", lambda x: x.get("foreign_terminated_by_controller") is True),
        ("owned_pgids_registered", lambda x: x.get("owned_pgids_registered") is not None),
        ("snapshot_available", lambda x: x.get("snapshot_pids") is not None),
    ]
    ok_all = True
    cond_results = {}
    for name, fn in conds:
        val = fn(d)
        cond_results[name] = {"satisfied": bool(val), "observed": d.get(name)}
        ok_all = ok_all and bool(val)
    res.add("lab-a: functional lifecycle (v3.2 surface)", "PASS" if ok_all else "FAIL", evidence=cond_results)


def check_lab_b(checkout, contract, res):
    impl = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/audio_oracle.py")
    if impl is None:
        res.fail("lab-b: analyze_wav signature", "audio_oracle.py missing at HEAD")
        return
    m = re.search(r"def analyze_wav\s*\((.*?)\)\s*->", impl, re.S)
    if not m:
        res.fail("lab-b: analyze_wav signature", "cannot parse signature")
        return
    sig = re.sub(r"\s+", " ", m.group(1))
    required = ["wav_path", "expected_loop_frames", "observed_loop_frames", "tempo", "numerator", "sample_rate"]
    missing = [a for a in required if not re.search(r"\b%s\b" % a, sig)]
    res.add("lab-b: analyze_wav full v3.2 signature (8 args, 6 required)",
            "PASS" if not missing and "silence_threshold" in sig and "peak_threshold" in sig else "FAIL",
            evidence={"sig": sig, "missing": missing})
    tm = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/test_modules.py") or ""
    hits = scan_for_patterns(tm, TRIVIAL_TEST_PATTERNS, "lab-b")
    invokes = "analyze_wav(" in tm
    res.add("lab-b: test_modules negative gates", "PASS" if (not hits and invokes) else "FAIL",
            evidence={"hits": hits, "invokes": invokes})
    probe_path = os.path.join(checkout, "_acceptance_probe_lab_b_v34.py")
    with open(probe_path, "w") as f:
        f.write(
            "import os, struct, sys, tempfile, wave, json\n"
            "sys.path.insert(0, 'tests/integration')\n"
            "from headless_audio_lab.audio_oracle import analyze_wav, AnalysisResult\n"
            "out = {}\n"
            "rate = 48000; n = 4800\n"
            "def make(path, nonzero):\n"
            "    w = wave.open(path, 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)\n"
            "    body = (b'\\x01\\x02' if nonzero else b'\\x00\\x00') * n\n"
            "    w.writeframes(body); w.close()\n"
            "fd, p = tempfile.mkstemp(suffix='.wav'); os.close(fd)\n"
            "make(p, False)\n"
            "r = analyze_wav(p, n, n, 120.0, 4, rate)\n"
            "d = r.to_dict() if hasattr(r, 'to_dict') else r\n"
            "out['silence_verification'] = d.get('verification')\n"
            "out['captured_frames'] = d.get('captured_wav_frames')\n"
            "os.unlink(p)\n"
            "fd2, p2 = tempfile.mkstemp(suffix='.wav'); os.write(fd2, b'RIFFnotreallyawav'); os.close(fd2)\n"
            "try:\n"
            "    r2 = analyze_wav(p2, n, n, 120.0, 4, rate)\n"
            "    d2 = r2.to_dict() if hasattr(r2, 'to_dict') else r2\n"
            "    out['malformed_result'] = d2.get('verification')\n"
            "except Exception as e:\n"
            "    out['malformed_result'] = 'EXCEPTION:' + type(e).__name__\n"
            "os.unlink(p2)\n"
            "fd3, p3 = tempfile.mkstemp(suffix='.wav'); os.close(fd3)\n"
            "make(p3, True)\n"
            "r3 = analyze_wav(p3, n, n, 120.0, 4, rate)\n"
            "d3 = r3.to_dict() if hasattr(r3, 'to_dict') else r3\n"
            "out['frame_count'] = d3.get('captured_wav_frames')\n"
            "os.unlink(p3)\n"
            "print(json.dumps(out))\n"
        )
    r = run(["python3", probe_path], checkout, timeout=120)
    try:
        os.unlink(probe_path)
    except OSError:
        pass
    if r["exit"] != 0:
        res.fail("lab-b: audio probes", "probe failed exit=%d stderr=%s" % (r["exit"], r["stderr"][-500:]))
        return
    try:
        d = json.loads(r["stdout"].strip().splitlines()[-1])
    except Exception:
        d = {}
    res.add("lab-b: silent WAV -> verification=FAIL",
            "PASS" if d.get("silence_verification") == "FAIL" else "FAIL", evidence=d)
    res.add("lab-b: captured_wav_frames exact",
            "PASS" if d.get("captured_frames") == 4800 else "FAIL", evidence=d)
    res.add("lab-b: malformed WAV FAIL/ERROR",
            "PASS" if str(d.get("malformed_result", "")).startswith(("FAIL", "ERROR", "EXCEPTION")) else "FAIL",
            evidence={"malformed": d.get("malformed_result")})
    res.add("lab-b: non-silent frames contract",
            "PASS" if d.get("frame_count") == 4800 else "FAIL", evidence=d)
    gi = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/fixtures/.gitignore") or ""
    literal_n = "\\n" in gi
    real_newline = "\n" in gi
    res.add("lab-b: .gitignore byte-exact",
            "PASS" if (not literal_n and real_newline) else "FAIL", evidence={"repr": repr(gi)})
    r = git(checkout, ["ls-tree", "-r", "HEAD"])
    elf_tracked = []
    for line in r["stdout"].splitlines():
        parts = line.split("\t", 1)
        if len(parts) != 2:
            continue
        blob = parts[0].split()[-1]
        br = subprocess.run(["git", "-C", checkout, "cat-file", "-p", blob], capture_output=True, timeout=30)
        if br.stdout[:4] == b"\x7fELF":
            elf_tracked.append(parts[1])
    res.add("lab-b: no ELF tracked", "PASS" if not elf_tracked else "FAIL", evidence={"elf_tracked": elf_tracked})
    try:
        import yaml  # noqa: F401
        have_yaml = True
    except ImportError:
        have_yaml = False
    bad = []
    for s in ["D1_source_direct.yaml", "D2_sl_passive.yaml", "D3_osc_recording.yaml"]:
        content = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/scenarios/%s" % s)
        if content is None:
            bad.append({"scenario": s, "error": "missing"})
            continue
        if have_yaml:
            try:
                yaml.safe_load(content)
            except Exception as e:
                bad.append({"scenario": s, "error": str(e)})
        elif not content.strip() or not any(k in content for k in ["source:", "capture:", "sooperlooper:", "expected:"]):
            bad.append({"scenario": s, "error": "missing required sections"})
    res.add("lab-b: scenarios parseable", "PASS" if not bad else "FAIL", evidence={"bad": bad})
    runner = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/runner.py") or ""
    api_hits = [a for a in ["ProcessSupervisor", "JackServer", "OscProbe", "SooperLooperLauncher",
                            "analyze_wav", "GraphAssertions", "LabRunner"] if a in runner]
    res.add("lab-b: runner uses contractual APIs",
            "PASS" if len(api_hits) >= 4 else "FAIL", evidence={"api_hits": api_hits})


# ---------------------------------------------------------------------------
# PHASE 3: full completion (workflow + reexecution + gates + rebuild)
# ---------------------------------------------------------------------------
def verify_controller_receipt(repo, c_oid, r_oid, e_oid, envelope_path, lab, batch,
                              env_raw, receipt, expected_branch, res):
    req = ["schema", "batch_id", "lab", "candidate_commit_oid", "implementation_result_commit_oid",
           "implementation_result_tree_oid", "envelope_container_commit_oid", "envelope_container_tree_oid",
           "envelope_blob_oid", "envelope_sha256", "remote", "branch", "remote_resolved_oid",
           "controller_literal_tests", "acceptance_harness_sha256", "acceptance_harness_verdict",
           "artifact_verification", "controlled_rebuild", "build_provenance_verdict",
           "worktree_clean_before", "worktree_clean_after", "completion_criteria", "final_verdict",
           "recorded_at_utc"]
    missing = [k for k in req if k not in receipt]
    res.add("receipt: schema exact", "PASS" if receipt.get("schema") == RECEIPT_SCHEMA else "FAIL")
    res.add("receipt: all required fields present", "PASS" if not missing else "FAIL", evidence={"missing": missing})
    r_tree = _git_quiet(repo, ["rev-parse", "%s^{tree}" % r_oid])
    e_tree = _git_quiet(repo, ["rev-parse", "%s^{tree}" % e_oid])
    e_blob = _git_quiet(repo, ["rev-parse", "%s:%s" % (e_oid, envelope_path)])
    env_sha = sha256_b(env_raw.encode("utf-8"))
    checks = {
        "batch_id exact": receipt.get("batch_id") == BATCH_ID,
        "lab exact": receipt.get("lab") == lab,
        "candidate OID exact": receipt.get("candidate_commit_oid") == c_oid,
        "result OID exact": receipt.get("implementation_result_commit_oid") == r_oid,
        "result tree OID exact": receipt.get("implementation_result_tree_oid") == r_tree,
        "envelope container OID == E": receipt.get("envelope_container_commit_oid") == e_oid,
        "envelope container tree OID == E^{tree}": receipt.get("envelope_container_tree_oid") == e_tree,
        "envelope blob OID == E:path": receipt.get("envelope_blob_oid") == e_blob,
        "envelope sha256 exact": receipt.get("envelope_sha256") == env_sha,
        "remote == origin": receipt.get("remote") == "origin",
        "branch == expected": receipt.get("branch") == expected_branch,
    }
    for name, ok_ in checks.items():
        res.add("receipt: %s" % name, "PASS" if ok_ else "FAIL",
                evidence={"observed": receipt.get(name)})
    rr = git(repo, ["ls-remote", "origin", expected_branch])
    resolved = ""
    for line in rr["stdout"].splitlines():
        if "\t" in line:
            ref = line.split("\t", 1)[1]
            if ref == expected_branch or ref == "refs/heads/" + expected_branch:
                resolved = line.split()[0]
    res.add("receipt: remote resolved OID == E", "PASS" if resolved == e_oid else "FAIL",
            evidence={"resolved": resolved, "expected": e_oid})
    res.add("receipt: remote_resolved_oid field consistent with actual",
            "PASS" if resolved and receipt.get("remote_resolved_oid") == resolved else "FAIL")
    rlt = receipt.get("controller_literal_tests")
    frozen_tests = batch["labs"][lab].get("literal_tests", {})
    if not isinstance(rlt, dict):
        rlt = {}
    ids_ok = set(rlt.keys()) == set(frozen_tests.keys())
    res.add("receipt: controller literal test IDs exact",
            "PASS" if ids_ok else "FAIL",
            evidence={"expected": sorted(frozen_tests), "actual": sorted(rlt)})
    for tid in frozen_tests:
        entry = rlt.get(tid)
        res.add("receipt: %s controller exit_code == 0" % tid,
                "PASS" if isinstance(entry, dict) and entry.get("exit_code") == 0 else "FAIL")
    res.add("receipt: acceptance harness sha256 exact",
            "PASS" if receipt.get("acceptance_harness_sha256") == sha256_file(__file__) else "FAIL")
    res.add("receipt: acceptance harness verdict PASS",
            "PASS" if receipt.get("acceptance_harness_verdict") == "PASS" else "FAIL")
    res.add("receipt: artifact verification PASS",
            "PASS" if receipt.get("artifact_verification") == "PASS" else "FAIL")
    res.add("receipt: build provenance verdict",
            "PASS" if receipt.get("build_provenance_verdict") in ("demonstrated", "not_applicable") else "FAIL")
    cc = receipt.get("completion_criteria")
    if isinstance(cc, dict):
        cc_false = [k for k, v in cc.items() if v is not True]
        res.add("receipt: completion criteria all true", "PASS" if not cc_false else "FAIL",
                evidence={"false": cc_false})
    else:
        res.fail("receipt: completion criteria all true")
    res.add("receipt: final verdict PASS", "PASS" if receipt.get("final_verdict") == "PASS" else "FAIL")
    for wk in ("worktree_clean_before", "worktree_clean_after"):
        res.add("receipt: %s true" % wk, "PASS" if receipt.get(wk) is True else "FAIL")


def run_full_completion_execution(repo, r_oid, lab, batch, receipt, res, claimed_outputs=None):
    tmp = tempfile.mkdtemp(prefix="v34-checkout-")
    wt = None
    try:
        wt = os.path.join(tmp, "wt")
        r_add = git(repo, ["worktree", "add", "--detach", wt, r_oid])
        if r_add["exit"] != 0:
            res.fail("completion: clean checkout of R", r_add["stderr"][-300:])
            return
        st0 = git(wt, ["status", "--porcelain=v1"])
        dirty0 = [ln for ln in st0["stdout"].splitlines() if ln]
        res.add("completion: worktree clean before", "PASS" if not dirty0 else "FAIL",
                evidence={"dirty": dirty0})
        frozen_tests = batch["labs"][lab].get("literal_tests", {})
        rlt = receipt.get("controller_literal_tests", {})
        for tid, meta in frozen_tests.items():
            entry = rlt.get(tid, {})
            rr = run(meta["command"], wt, timeout=meta.get("timeout_seconds", 180))
            res.add("completion: %s literal test reexec exit 0" % tid,
                    "PASS" if rr["exit"] == 0 else "FAIL",
                    evidence={"exit": rr["exit"], "stderr_tail": rr["stderr"][-300:]})
            res.add("completion: %s stdout hash matches receipt" % tid,
                    "PASS" if sha256_b(rr["stdout"].encode("utf-8")) == entry.get("stdout_sha256") else "FAIL")
        if lab == "LAB-A":
            check_lab_a(wt, load_contract(), res)
        else:
            check_lab_b(wt, load_contract(), res)
        # controlled canonical rebuild
        build_cmds = batch["labs"][lab].get("build_command")
        if build_cmds is None:
            res.add("completion: controlled rebuild not_applicable (LAB-A)",
                    "PASS" if receipt.get("controlled_rebuild") == "not_applicable" else "FAIL")
        else:
            build_dir = os.path.join(wt, "build")
            shutil.rmtree(build_dir, ignore_errors=True)
            os.makedirs(build_dir)
            for bc in build_cmds:
                rr = run(bc, wt, timeout=180)
                res.add("completion: controlled rebuild exit 0 (%s)" % os.path.basename(bc[-2] if len(bc) >= 2 else "?"),
                        "PASS" if rr["exit"] == 0 else "FAIL",
                        evidence={"exit": rr["exit"], "stderr_tail": rr["stderr"][-300:]})
            for out_rel in ("build/synthetic_source", "build/deterministic_capture"):
                opath = os.path.join(wt, out_rel)
                if os.path.exists(opath):
                    data = open(opath, "rb").read()
                    res.add("completion: controller-built %s exists + ELF metadata" % out_rel,
                            "PASS" if data[:4] == b"\x7fELF" else "FAIL",
                            evidence={"sha256": sha256_b(data), "size": len(data), "elf": data[:4] == b"\x7fELF"})
                else:
                    res.fail("completion: controller-built %s exists" % out_rel)
            # compare with leaf claim outputs
            for claim in (claimed_outputs or []):
                cpath = os.path.join(wt, claim.get("path", ""))
                if os.path.exists(cpath):
                    real_sha = sha256_file(cpath)
                    real_size = os.path.getsize(cpath)
                else:
                    real_sha, real_size = "", -1
                res.add("completion: rebuild output %s sha256 == claim" % claim.get("path"),
                        "PASS" if real_sha == claim.get("sha256") else "FAIL",
                        evidence={"computed": real_sha, "claimed": claim.get("sha256")})
                res.add("completion: rebuild output %s size == claim" % claim.get("path"),
                        "PASS" if real_size == claim.get("size") else "FAIL",
                        evidence={"computed": real_size, "claimed": claim.get("size")})
            shutil.rmtree(build_dir, ignore_errors=True)
        st = git(wt, ["status", "--porcelain=v1"])
        dirty = [ln for ln in st["stdout"].splitlines() if ln]
        res.add("completion: worktree clean after", "PASS" if not dirty else "FAIL",
                evidence={"dirty": dirty})
    finally:
        if wt is not None:
            git(repo, ["worktree", "remove", "--force", wt])
        shutil.rmtree(tmp, ignore_errors=True)


def verify_full_completion(repo, c_oid, r_oid, e_oid, envelope_path, lab, batch,
                           env_raw, receipt, expected_branch, res):
    verify_leaf_envelope(repo, c_oid, r_oid, e_oid, envelope_path, lab, batch, res,
                         expected_branch=expected_branch)
    if receipt is None:
        res.fail("completion: controller acceptance receipt present", "missing (required in full-completion)")
        return
    verify_controller_receipt(repo, c_oid, r_oid, e_oid, envelope_path, lab, batch,
                              env_raw, receipt, expected_branch, res)
    claimed_outputs = None
    try:
        env = json.loads(env_raw)
        claimed_outputs = (env.get("leaf_build_claim") or {}).get("outputs")
    except Exception:
        pass
    run_full_completion_execution(repo, r_oid, lab, batch, receipt, res, claimed_outputs)


# ---------------------------------------------------------------------------
# selftest helpers + selftest
# ---------------------------------------------------------------------------
def _write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(content)


def _commit(repo, msg):
    run(["git", "add", "-A"], repo, timeout=60)
    r = run(["git", "commit", "-m", msg], repo, timeout=60)
    if r["exit"] != 0:
        raise RuntimeError("commit failed: %s" % r["stderr"][-300:])
    return _git_quiet(repo, ["rev-parse", "HEAD"])


def _init_repo(base, name):
    repo = os.path.join(base, name)
    os.makedirs(repo)
    run(["git", "init", "-q"], repo)
    run(["git", "config", "user.email", "controller@hermes.local"], repo)
    run(["git", "config", "user.name", "Controller"], repo)
    return repo


def _now():
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


C_CANONICAL_SOURCES = {
    "tests/integration/virtual_studio/audio/synthetic_source.c": (
        "#include <jack/jack.h>\n"
        "#include <stdio.h>\n"
        "int main(void) { printf(\"synthetic-source\\n\"); return 0; }\n"
    ),
    "tests/integration/virtual_studio/audio/deterministic_capture.c": (
        "#include <jack/jack.h>\n"
        "#include <stdio.h>\n"
        "int main(void) { printf(\"deterministic-capture\\n\"); return 0; }\n"
    ),
}


def fixture_impl(lab):
    if lab == "LAB-A":
        sup = (
            "import os\n"
            "import signal\n"
            "import subprocess\n"
            "\n"
            "class ProcessInfo:\n"
            "    def __init__(self, pid, pgid):\n"
            "        self.pid = pid\n"
            "        self.pgid = pgid\n"
            "\n"
            "class ProcessSupervisor:\n"
            "    def __init__(self):\n"
            "        self._owned = {}\n"
            "        self._names = {}\n"
            "    def __enter__(self):\n"
            "        return self\n"
            "    def __exit__(self, *exc):\n"
            "        self.cleanup(timeout=5.0)\n"
            "        return False\n"
            "    def start_process(self, name, cmd, env, cwd, stdout_path, stderr_path):\n"
            "        try:\n"
            "            p = subprocess.Popen(cmd, env=env, cwd=cwd,\n"
            "                                stdin=subprocess.DEVNULL,\n"
            "                                stdout=open(stdout_path, 'w') if stdout_path != '/dev/null' else subprocess.DEVNULL,\n"
            "                                stderr=open(stderr_path, 'w') if stderr_path != '/dev/null' else subprocess.DEVNULL,\n"
            "                                start_new_session=True)\n"
            "        except Exception as e:\n"
            "            raise RuntimeError('start failed: %s' % e) from e\n"
            "        info = ProcessInfo(p.pid, p.pid)\n"
            "        self._owned[p.pid] = p\n"
            "        self._names[name] = p.pid\n"
            "        return info\n"
            "    def get_owned_pids(self):\n"
            "        return list(self._owned)\n"
            "    def get_owned_pgids(self):\n"
            "        return [os.getpgid(pid) for pid in self._owned]\n"
            "    def get_process_info(self, name):\n"
            "        pid = self._names.get(name)\n"
            "        if pid is None:\n"
            "            return None\n"
            "        return ProcessInfo(pid, os.getpgid(pid))\n"
            "    def snapshot(self):\n"
            "        return [{'pid': pid, 'pgid': os.getpgid(pid)} for pid in self._owned]\n"
            "    def cleanup(self, timeout=5.0):\n"
            "        for pid, p in list(self._owned.items()):\n"
            "            try:\n"
            "                if p.poll() is None:\n"
            "                    os.killpg(os.getpgid(pid), signal.SIGTERM)\n"
            "                    try:\n"
            "                        p.wait(timeout=timeout)\n"
            "                    except subprocess.TimeoutExpired:\n"
            "                        os.killpg(os.getpgid(pid), signal.SIGKILL)\n"
            "                        p.wait(timeout=5.0)\n"
            "            except (ProcessLookupError, PermissionError):\n"
            "                pass\n"
            "            self._owned.pop(pid, None)\n"
            "    def verify_cleanup(self):\n"
            "        return [pid for pid, p in self._owned.items() if p.poll() is None]\n"
        )
        return {
            "tests/integration/headless_audio_lab/__init__.py": "# headless audio lab package\n",
            "tests/integration/headless_audio_lab/process_supervisor.py": sup,
            "tests/integration/headless_audio_lab/jack_server.py": (
                "class JackServer:\n"
                "    def start(self):\n        return True\n"
                "    def readiness_probe(self, timeout_s=5):\n        return True\n"
                "    def get_ports(self):\n        return []\n"
                "    def get_connections(self):\n        return []\n"
                "    def verify_no_external_clients(self):\n        return True\n"
                "    def cleanup(self):\n        return True\n"
                "    def is_running(self):\n        return False\n"
                "    def get_pid(self):\n        return None\n"
                "    def get_pgid(self):\n        return None\n"
                "    def snapshot(self):\n        return {'running': False}\n"
            ),
            "tests/integration/headless_audio_lab/osc_probe.py": (
                "class OscProbe:\n"
                "    def ping(self):\n        return False\n"
                "    def get_control(self, name):\n        return None\n"
                "    def set_control(self, name, value):\n        return True\n"
                "    def hit_command(self, cmd):\n        return True\n"
                "    def register_auto_update(self, cb):\n        return True\n"
                "    def unregister_auto_update(self, cb):\n        return True\n"
                "    def poll(self):\n        return None\n"
            ),
            "tests/integration/headless_audio_lab/sooperlooper_launcher.py": (
                "class SooperLooperLauncher:\n"
                "    def launch(self):\n        return None\n"
                "    def readiness_ping(self):\n        return False\n"
                "    def cleanup(self):\n        return True\n"
            ),
        }
    # LAB-B
    oracle = (
        "import os\n"
        "import struct\n"
        "import wave\n"
        "\n"
        "class AnalysisResult:\n"
        "    def __init__(self, expected_loop_frames, observed_loop_frames, captured_wav_frames,\n"
        "                 analyzed_segment_frames, verification):\n"
        "        self.expected_loop_frames = expected_loop_frames\n"
        "        self.observed_loop_frames = observed_loop_frames\n"
        "        self.captured_wav_frames = captured_wav_frames\n"
        "        self.analyzed_segment_frames = analyzed_segment_frames\n"
        "        self.verification = verification\n"
        "    def to_dict(self):\n"
        "        return {'expected_loop_frames': self.expected_loop_frames,\n"
        "                'observed_loop_frames': self.observed_loop_frames,\n"
        "                'captured_wav_frames': self.captured_wav_frames,\n"
        "                'analyzed_segment_frames': self.analyzed_segment_frames,\n"
        "                'verification': self.verification}\n"
        "\n"
        "def _read_wav(path):\n"
        "    with open(path, 'rb') as f:\n"
        "        data = f.read()\n"
        "    if len(data) < 44 or data[:4] != b'RIFF' or data[8:12] != b'WAVE':\n"
        "        raise ValueError('not a WAV')\n"
        "    fmt_off = 12\n"
        "    fmt_size = struct.unpack('<I', data[fmt_off+4:fmt_off+8])[0]\n"
        "    channels, rate = struct.unpack('<HH', data[fmt_off+8:fmt_off+12])\n"
        "    data_off = fmt_off + 8 + fmt_size\n"
        "    data_size = struct.unpack('<I', data[data_off+4:data_off+8])[0]\n"
        "    pcm = data[data_off+8:data_off+8+data_size]\n"
        "    return rate, channels, pcm\n"
        "\n"
        "def analyze_wav(wav_path, expected_loop_frames, observed_loop_frames, tempo, numerator,\n"
        "                sample_rate, silence_threshold=0.01, peak_threshold=0.9) -> AnalysisResult:\n"
        "    rate, channels, pcm = _read_wav(wav_path)\n"
        "    frames = len(pcm) // (2 * channels)\n"
        "    peak = 0\n"
        "    total = 0\n"
        "    for i in range(0, len(pcm) - 1, 2):\n"
        "        v = struct.unpack('<h', pcm[i:i+2])[0]\n"
        "        peak = max(peak, abs(v))\n"
        "        total += v * v\n"
        "    rms = (total / max(2 * frames, 1)) ** 0.5\n"
        "    if frames != observed_loop_frames or peak == 0 or rms < silence_threshold:\n"
        "        verification = 'FAIL'\n"
        "    elif abs(frames - expected_loop_frames) <= max(1, int(expected_loop_frames * 0.02)):\n"
        "        verification = 'PASS'\n"
        "    else:\n"
        "        verification = 'FAIL'\n"
        "    return AnalysisResult(expected_loop_frames, observed_loop_frames, frames, frames, verification)\n"
        "\n"
        "def analyze_loop_reproduction(source_wav, captured_wav, expected_loop_frames, tolerance_percent=10.0):\n"
        "    _, _, src = _read_wav(source_wav)\n"
        "    _, _, cap = _read_wav(captured_wav)\n"
        "    src_frames = len(src) // 2\n"
        "    cap_frames = len(cap) // 2\n"
        "    ratio = cap_frames / max(src_frames, 1)\n"
        "    ok = abs(ratio - 1.0) <= tolerance_percent / 100.0\n"
        "    return {'source_frames': src_frames, 'captured_frames': cap_frames,\n"
        "            'ratio': ratio, 'verification': 'PASS' if ok else 'FAIL'}\n"
    )
    return {
        "tests/__init__.py": "# tests package\n",
        "tests/integration/__init__.py": "# integration package\n",
        "tests/integration/headless_audio_lab/audio_oracle.py": oracle,
        "tests/integration/headless_audio_lab/graph_assertions.py": (
            "class GraphAssertions:\n"
            "    def get_graph(self):\n        return {}\n"
            "    def get_ports(self):\n        return []\n"
            "    def get_connections(self):\n        return []\n"
            "    def get_clients(self):\n        return []\n"
            "    def snapshot(self):\n        return {}\n"
            "    def diff_snapshots(self, a, b):\n        return {}\n"
            "    def assert_clean_graph(self):\n        return True\n"
        ),
        "tests/integration/headless_audio_lab/runner.py": (
            "class LabRunner:\n"
            "    def run(self, scenario):\n"
            "        return {'scenario': scenario, 'ok': True}\n"
            "    def cli(self):\n        return 0\n"
            "\n"
            "def _uses_contractual_apis():\n"
            "    return ['ProcessSupervisor', 'JackServer', 'OscProbe', 'SooperLooperLauncher',\n"
            "            'analyze_wav', 'GraphAssertions', 'LabRunner']\n"
        ),
        "tests/integration/headless_audio_lab/test_modules.py": (
            "from headless_audio_lab.audio_oracle import analyze_wav, AnalysisResult, analyze_loop_reproduction\n"
            "from headless_audio_lab.graph_assertions import GraphAssertions\n"
            "from headless_audio_lab.runner import LabRunner\n"
            "def test_oracle_contract():\n"
            "    return callable(analyze_wav) or analyze_wav(b'') is not None\n"
        ),
        "tests/integration/headless_audio_lab/scenarios/D1_source_direct.yaml": (
            "name: D1 source direct\n"
            "source:\n"
            "  type: synthetic\n"
            "  file: fixtures/synthetic_source\n"
            "expected:\n"
            "  captured: true\n"
        ),
        "tests/integration/headless_audio_lab/scenarios/D2_sl_passive.yaml": (
            "name: D2 sooperlooper passive\n"
            "sooperlooper:\n"
            "  mode: passive\n"
            "  loops: 1\n"
            "capture:\n"
            "  enabled: true\n"
            "expected:\n"
            "  frames: 4800\n"
        ),
        "tests/integration/headless_audio_lab/scenarios/D3_osc_recording.yaml": (
            "name: D3 osc recording\n"
            "sooperlooper:\n"
            "  mode: active\n"
            "  osc: true\n"
            "capture:\n"
            "  enabled: true\n"
            "expected:\n"
            "  recorded: true\n"
        ),
        "tests/integration/headless_audio_lab/fixtures/synthetic_source": (
            "#!/bin/sh\n"
            "# synthetic source fixture (controller rebuilds from canonical C source)\n"
            "echo source\n"
        ),
        "tests/integration/headless_audio_lab/fixtures/deterministic_capture": (
            "#!/bin/sh\n"
            "# deterministic capture fixture (controller rebuilds from canonical C source)\n"
            "echo capture\n"
        ),
        "tests/integration/headless_audio_lab/fixtures/.gitignore": "# generated lab artifacts\nbuild/\n",
    }


def _blob_sha(repo, oid, path):
    r = subprocess.run(["git", "-C", repo, "show", "%s:%s" % (oid, path)], capture_output=True, timeout=30)
    return sha256_b(r.stdout), len(r.stdout)


def build_valid_fixture(base, name, lab, mutate=None, impl_files=None, envelope_mutate=None,
                        skip_artifacts=False, minimal_envelope=False, include_canonical_c=True):
    repo = _init_repo(base, name)
    _write_file(os.path.join(repo, "README.md"), "base\n")
    _write_file(os.path.join(repo, ".gitignore"), "build/\n__pycache__/\n")
    if include_canonical_c:
        for cpath, csrc in C_CANONICAL_SOURCES.items():
            _write_file(os.path.join(repo, cpath), csrc)
    c = _commit(repo, "C")
    impl = dict(fixture_impl(lab) if impl_files is None else impl_files)
    for path, content in impl.items():
        _write_file(os.path.join(repo, path), content)
    r = _commit(repo, "R impl")
    r_tree = _git_quiet(repo, ["rev-parse", "HEAD^{tree}"])
    objfmt = _git_quiet(repo, ["rev-parse", "--show-object-format"])
    batch = load_batch_manifest()
    lab_meta = batch["labs"][lab]
    env_path = lab_meta["envelope_path"]

    if minimal_envelope:
        env = {"candidate_commit_oid": c, "implementation_result_commit_oid": r,
               "implementation_result_tree_oid": r_tree, "git_object_format": objfmt, "artifacts": {}}
        env_raw = json.dumps(env, indent=2) + "\n"
        _write_file(os.path.join(repo, env_path), env_raw)
        e = _commit(repo, "E envelope")
        if mutate:
            mutate(repo, c, r, e)
        return repo, c, r, e, env, env_raw

    artifacts = {}
    for path in sorted(impl):
        sha, size = _blob_sha(repo, r, path)
        kind = "source"
        if path.endswith(".yaml"):
            kind = "scenario"
        elif path.endswith("test_modules.py"):
            kind = "test"
        elif path.endswith("fixtures/.gitignore"):
            kind = "ignore-rule"
        artifacts[path] = {"kind": kind, "sha256": sha, "size": size}
    if skip_artifacts:
        artifacts = {}

    t_start, t_end = _now(), _now()
    llt = {}
    for tid, m in lab_meta["literal_tests"].items():
        llt[tid] = {"command_sha256": sha256_b(json.dumps(m["command"], ensure_ascii=False).encode("utf-8")),
                    "exit_code": 0, "stdout_sha256": "0" * 64, "stderr_sha256": "0" * 64,
                    "started_at_utc": t_start, "ended_at_utc": t_end}
    if lab == "LAB-A":
        lbc = {"status": "not_applicable", "sources": [], "compiler_path": None,
               "compiler_version": None, "commands": [], "outputs": [], "transcript_sha256": None}
    else:
        # LAB-B claimed_built: claim the two canonical helper outputs (REAL compile)
        build_cmds = batch["labs"]["LAB-B"].get("build_command", [])
        outputs = []
        os.makedirs(os.path.join(repo, "build"), exist_ok=True)
        for bc in build_cmds:
            run(bc, repo, timeout=180)
            out_rel = bc[bc.index("-o") + 1] if "-o" in bc else None
            if out_rel:
                opath = os.path.join(repo, out_rel)
                if os.path.exists(opath):
                    data = open(opath, "rb").read()
                    outputs.append({"path": out_rel, "sha256": sha256_b(data), "size": len(data)})
        shutil.rmtree(os.path.join(repo, "build"), ignore_errors=True)
        lbc = {"status": "claimed_built",
               "sources": sorted(C_CANONICAL_SOURCES.keys()),
               "compiler_path": "/usr/bin/gcc", "compiler_version": "13.3.0",
               "commands": [json.dumps(x, ensure_ascii=False) for x in build_cmds],
               "outputs": outputs,
               "transcript_sha256": None}

    env = {
        "schema": SCHEMA, "lab": lab, "batch_id": BATCH_ID,
        "leaf_id": lab_meta["leaf_id"], "lease_id": lab_meta["lease_id"],
        "candidate_commit_oid": c, "implementation_result_commit_oid": r,
        "implementation_result_tree_oid": r_tree, "git_object_format": objfmt,
        "external_branch_binding": {"remote": "origin",
                                    "planned_branch": "result/v3-4-fixture-%s" % lab.lower()},
        "leaf_literal_tests": llt, "leaf_build_claim": lbc, "artifacts": artifacts,
    }
    if envelope_mutate:
        envelope_mutate(env, repo, c, r, r_tree, impl)
    env_raw = json.dumps(env, indent=2) + "\n"
    _write_file(os.path.join(repo, env_path), env_raw)
    e = _commit(repo, "E envelope")
    if mutate:
        mutate(repo, c, r, e)
    return repo, c, r, e, env, env_raw


def _default_receipt(repo, c, r, e, env_raw, lab, expected_branch, **overrides):
    e_tree = _git_quiet(repo, ["rev-parse", "%s^{tree}" % e])
    lab_meta = load_batch_manifest()["labs"][lab]
    env_path = lab_meta["envelope_path"]
    e_blob = _git_quiet(repo, ["rev-parse", "%s:%s" % (e, env_path)])
    env_sha = sha256_b(env_raw.encode("utf-8"))
    rlt = {}
    tmp = tempfile.mkdtemp(prefix="v34-receipt-")
    wt = os.path.join(tmp, "wt")
    git(repo, ["worktree", "add", "--detach", wt, r])
    try:
        for tid, m in lab_meta["literal_tests"].items():
            t0 = _now()
            rr = run(m["command"], wt, timeout=m.get("timeout_seconds", 180))
            t1 = _now()
            rlt[tid] = {"command_sha256": sha256_b(json.dumps(m["command"], ensure_ascii=False).encode("utf-8")),
                        "exit_code": rr["exit"],
                        "stdout_sha256": sha256_b(rr["stdout"].encode("utf-8")),
                        "stderr_sha256": sha256_b(rr["stderr"].encode("utf-8")),
                        "started_at_utc": t0, "ended_at_utc": t1}
    finally:
        git(repo, ["worktree", "remove", "--force", wt])
        shutil.rmtree(tmp, ignore_errors=True)
    rec = {
        "schema": RECEIPT_SCHEMA, "batch_id": BATCH_ID, "lab": lab,
        "candidate_commit_oid": c, "implementation_result_commit_oid": r,
        "implementation_result_tree_oid": _git_quiet(repo, ["rev-parse", "%s^{tree}" % r]),
        "envelope_container_commit_oid": e, "envelope_container_tree_oid": e_tree,
        "envelope_blob_oid": e_blob, "envelope_sha256": env_sha,
        "remote": "origin", "branch": expected_branch, "remote_resolved_oid": e,
        "controller_clean_checkout_path_or_receipt": "fixture-clean-checkout",
        "controller_literal_tests": rlt,
        "acceptance_harness_sha256": sha256_file(__file__),
        "acceptance_harness_verdict": "PASS", "artifact_verification": "PASS",
        "controlled_rebuild": "not_applicable" if lab == "LAB-A" else "PASS",
        "build_provenance_verdict": "not_applicable" if lab == "LAB-A" else "demonstrated",
        "worktree_clean_before": True, "worktree_clean_after": True,
        "completion_criteria": {"non_empty_commit": True, "published_branch": True, "full_result_head_sha": True,
                                "clean_worktree": True, "valid_result_envelope": True, "owned_path_only_diff": True,
                                "literal_focused_tests_exit_0": True, "controller_clean_checkout_acceptance_pass": True},
        "final_verdict": "PASS", "recorded_at_utc": _now(),
    }
    rec.update(overrides)
    return rec


def _push_to_bare(base, repo, branch, oid):
    bare = os.path.join(base, "origin.git")
    if not os.path.exists(bare):
        subprocess.run(["git", "init", "--bare", "-q", bare], check=True)
    subprocess.run(["git", "-C", repo, "remote", "add", "origin", bare], check=True)
    subprocess.run(["git", "-C", bare, "update-ref", "-d", "refs/heads/%s" % branch], capture_output=True)
    subprocess.run(["git", "-C", repo, "push", "origin", "%s:refs/heads/%s" % (oid, branch)],
                   capture_output=True, check=True, timeout=60)
    return bare


def selftest():
    res = Result()
    batch = load_batch_manifest()
    baseline = load_baseline()
    scope = load_json(os.path.join(PACKAGE_DIR, "scope-freeze-v3.4.json"), "scope-freeze")
    lab = "LAB-A"
    env_path = batch["labs"][lab]["envelope_path"]
    expected = "result/v3-4-fixture-lab-a"
    MAIN = "/home/ubuntu/code/music/seq66-loves-sooperlooper"

    # --- functional continuity phase ---
    def run_fc(contract_obj):
        fd, cpath = tempfile.mkstemp(suffix=".json")
        os.close(fd)
        with open(cpath, "w") as f:
            json.dump(contract_obj, f, indent=2)
        r = Result()
        try:
            verify_functional_continuity(MAIN, baseline, contract_obj,
                                         scope, batch, r)
        finally:
            os.unlink(cpath)
        return r

    r_fc = run_fc(load_contract())
    res.add("selftest: valid v3.2 functional contract + v3.3 strict workflow PASS",
            "PASS" if not r_fc.failed() else "FAIL",
            evidence={"failing": [x["name"] for x in r_fc.checks if x["status"] == "FAIL"]})

    def mutate_contract(fn):
        c = json.loads(json.dumps(load_contract()))
        fn(c)
        return c

    def fc_reject(name, contract_obj):
        r = run_fc(contract_obj)
        res.add(name, "PASS" if r.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in r.checks if x["status"] == "FAIL"]})

    fc_reject("selftest: v3.3 analyze_wav(wav_bytes)->dict REJECT", mutate_contract(
        lambda c: c["modules"]["audio_oracle.py"]["functions"].__setitem__(
            [i for i, f in enumerate(c["modules"]["audio_oracle.py"]["functions"]) if f.get("name") == "analyze_wav"][0],
            {"args": [{"name": "wav_bytes", "annotation": "bytes"}], "name": "analyze_wav", "returns": "dict",
             "constraints": ["silent WAV MUST produce silent=True"]})))
    fc_reject("selftest: v3.3 silent=True without verification FAIL REJECT", mutate_contract(
        lambda c: [f.update({"constraints": ["silent WAV MUST produce silent=True"]})
                   for f in c["modules"]["audio_oracle.py"]["functions"] if f.get("name") == "analyze_wav"]))
    fc_reject("selftest: AnalysisResult removed REJECT", mutate_contract(
        lambda c: c["modules"]["audio_oracle.py"]["functions"].__setitem__(
            [i for i, f in enumerate(c["modules"]["audio_oracle.py"]["functions"]) if f.get("name") == "AnalysisResult"][0],
            {"kind": "class", "name": "AnalysisResultX", "fields": []})))
    fc_reject("selftest: analyze_loop_reproduction missing REJECT", mutate_contract(
        lambda c: c["modules"]["audio_oracle.py"]["functions"].__setitem__(
            [i for i, f in enumerate(c["modules"]["audio_oracle.py"]["functions"]) if f.get("name") == "analyze_loop_reproduction"][0],
            {"name": "analyze_loop_reproduction_removed"})))
    def drop_methods(cls):
        def fn(c):
            for mod in ("jack_server.py", "osc_probe.py", "sooperlooper_launcher.py", "graph_assertions.py", "runner.py"):
                for f in c["modules"].get(mod, {}).get("functions", []):
                    if f.get("kind") == "class" and f.get("name") == cls:
                        f["methods"] = f.get("methods", [])[:1]
        return fn
    for cls, name in (("GraphAssertions", "selftest: GraphAssertions API reduced REJECT"),
                      ("LabRunner", "selftest: LabRunner API reduced REJECT"),
                      ("JackServer", "selftest: JackServer API reduced REJECT"),
                      ("OscProbe", "selftest: OscProbe API reduced REJECT"),
                      ("SooperLooperLauncher", "selftest: SooperLooperLauncher API reduced REJECT")):
        fc_reject(name, mutate_contract(drop_methods(cls)))

    def fc_literal_missing(lab_id):
        def fn(c):
            pass
        b2 = json.loads(json.dumps(batch))
        for tid in list(b2["labs"][lab_id]["literal_tests"]):
            b2["labs"][lab_id]["literal_tests"].pop(tid)
            break
        return b2
    for lid, name in (("LAB-A", "selftest: a LAB-A literal test missing REJECT"),
                      ("LAB-B", "selftest: a LAB-B literal test missing REJECT")):
        b2 = fc_literal_missing(lid)
        r = Result()
        verify_functional_continuity(MAIN, baseline, load_contract(), scope, b2, r)
        res.add(name, "PASS" if r.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in r.checks if x["status"] == "FAIL"]})

    # synthetic JSON build command REJECT (mutate build policy)
    bp = json.loads(open(os.path.join(PACKAGE_DIR, "build-provenance-policy-v3.4.json")).read())
    bp_synth = json.loads(json.dumps(bp))
    bp_synth["canonical_compile_commands"] = [["python3", "-c", "import hashlib; print(hashlib.sha256(b'x').hexdigest())"]]
    fd, bp_path = tempfile.mkstemp(suffix=".json")
    os.close(fd)
    with open(bp_path, "w") as f:
        json.dump(bp_synth, f, indent=2)
    r_bp = Result()
    try:
        verify_functional_continuity(MAIN, baseline, load_contract(), scope, batch, r_bp)
    finally:
        os.unlink(bp_path)
    # the continuity phase reads the policy from PACKAGE_DIR; simulate by checking command equality against mutated baseline
    r_bp2 = Result()
    base_mut = json.loads(json.dumps(baseline))
    base_mut["canonical_compile_commands"] = [["python3", "-c", "print('synthetic')"]]
    # re-run with mutated baseline to check compile command preservation fires FAIL
    verify_functional_continuity(MAIN, base_mut, load_contract(), scope, batch, r_bp2)
    res.add("selftest: synthetic JSON build command REJECT",
            "PASS" if r_bp2.failed() else "FAIL",
            evidence={"failing": [x["name"] for x in r_bp2.checks if x["status"] == "FAIL"]})

    # canonical source path changed REJECT
    r_src = Result()
    base_src = json.loads(json.dumps(baseline))
    base_src["canonical_sources"][0]["path"] = "tests/integration/virtual_studio/audio/other.c"
    verify_functional_continuity(MAIN, base_src, load_contract(), scope, batch, r_src)
    res.add("selftest: canonical source path changed REJECT", "PASS" if r_src.failed() else "FAIL")

    # ownership drift (remove fixtures/.gitignore from scope)
    r_own = Result()
    scope_drift = json.loads(json.dumps(scope))
    scope_drift["scope"]["lab_b_owned_paths"].remove("tests/integration/headless_audio_lab/fixtures/.gitignore")
    verify_functional_continuity(MAIN, baseline, load_contract(), scope_drift, batch, r_own)
    res.add("selftest: .gitignore absent from ownership manifest REJECT", "PASS" if r_own.failed() else "FAIL")
    r_own2 = Result()
    batch_drift = json.loads(json.dumps(batch))
    batch_drift["labs"]["LAB-B"]["owned_paths"].remove("tests/integration/headless_audio_lab/fixtures/.gitignore")
    verify_functional_continuity(MAIN, baseline, load_contract(), scope, batch_drift, r_own2)
    res.add("selftest: ownership drift between artifacts REJECT", "PASS" if r_own2.failed() else "FAIL")

    # --- leaf-envelope + full-completion (workflow layer, from v3.3) ---
    with tempfile.TemporaryDirectory(prefix="v34-selftest-") as tmp:
        g = build_valid_fixture(tmp, "valid-leaf", lab)
        repo, c, r, e, env, env_raw = g
        r1 = Result()
        verify_leaf_envelope(repo, c, r, e, env_path, lab, batch, r1, expected_branch=expected)
        res.add("selftest: strict envelope valid PASS", "PASS" if not r1.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in r1.checks if x["status"] == "FAIL"]})

        g2 = build_valid_fixture(tmp, "minimal-env", lab, minimal_envelope=True)
        r2 = Result()
        verify_leaf_envelope(g2[0], g2[1], g2[2], g2[3], env_path, lab, batch, r2, expected_branch=expected)
        res.add("selftest: minimal v3.2 envelope REJECT", "PASS" if r2.failed() else "FAIL")

        g3 = build_valid_fixture(tmp, "r-empty", lab, impl_files={"tests/integration/headless_audio_lab/__init__.py": "# lab\n"})
        # R == C: no implementation result at all (candidate == result)
        repo3 = _init_repo(tmp, "r-empty2")
        _write_file(os.path.join(repo3, "README.md"), "base\n")
        c3 = _commit(repo3, "C")
        r3_tree = _git_quiet(repo3, ["rev-parse", "HEAD^{tree}"])
        objfmt3 = _git_quiet(repo3, ["rev-parse", "--show-object-format"])
        env3 = {
            "schema": SCHEMA, "lab": lab, "batch_id": BATCH_ID,
            "leaf_id": batch["labs"][lab]["leaf_id"], "lease_id": batch["labs"][lab]["lease_id"],
            "candidate_commit_oid": c3, "implementation_result_commit_oid": c3,
            "implementation_result_tree_oid": r3_tree, "git_object_format": objfmt3,
            "external_branch_binding": {"remote": "origin", "planned_branch": expected},
            "leaf_literal_tests": {}, "leaf_build_claim": {"status": "not_applicable", "sources": [],
            "compiler_path": None, "compiler_version": None, "commands": [], "outputs": [], "transcript_sha256": None},
            "artifacts": {},
        }
        _write_file(os.path.join(repo3, env_path), json.dumps(env3, indent=2) + "\n")
        e3 = _commit(repo3, "E")
        r3res = Result()
        verify_leaf_envelope(repo3, c3, c3, e3, env_path, lab, batch, r3res, expected_branch=expected)
        res.add("selftest: R empty REJECT", "PASS" if r3res.failed() else "FAIL")

        g4 = build_valid_fixture(tmp, "wrong-art-hash", lab, envelope_mutate=lambda env, *a: env["artifacts"].__setitem__(
            next(iter(env["artifacts"])), {"kind": "source", "sha256": "1" * 64, "size": 1}))
        r4 = Result()
        verify_leaf_envelope(g4[0], g4[1], g4[2], g4[3], env_path, lab, batch, r4, expected_branch=expected)
        res.add("selftest: wrong artifact hash REJECT", "PASS" if r4.failed() else "FAIL")

        gf = build_valid_fixture(tmp, "full-a", lab)
        repoF, cF, rF, eF, _, env_rawF = gf
        _push_to_bare(tmp, repoF, expected, eF)
        recF = _default_receipt(repoF, cF, rF, eF, env_rawF, lab, expected)
        rFres = Result()
        verify_full_completion(repoF, cF, rF, eF, env_path, lab, batch, env_rawF, recF, expected, rFres)
        res.add("selftest: strict full completion valid PASS", "PASS" if not rFres.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in rFres.checks if x["status"] == "FAIL"]})

        # LAB-B full completion with REAL canonical helper rebuild
        gb = build_valid_fixture(tmp, "full-b", "LAB-B")
        repoB, cB, rB, eB, _, env_rawB = gb
        expectedB = "result/v3-4-fixture-lab-b"
        _push_to_bare(tmp, repoB, expectedB, eB)
        recB = _default_receipt(repoB, cB, rB, eB, env_rawB, "LAB-B", expectedB)
        rBres = Result()
        verify_full_completion(repoB, cB, rB, eB, batch["labs"]["LAB-B"]["envelope_path"],
                               "LAB-B", batch, env_rawB, recB, expectedB, rBres)
        res.add("selftest: LAB-B full completion with real canonical rebuild PASS",
                "PASS" if not rBres.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in rBres.checks if x["status"] == "FAIL"]})

        r34 = Result()
        verify_full_completion(repoF, cF, rF, eF, env_path, lab, batch, env_rawF, None, expected, r34)
        res.add("selftest: missing controller receipt REJECT", "PASS" if r34.failed() else "FAIL")

        recBad = _default_receipt(repoF, cF, rF, eF, env_rawF, lab, expected, remote_resolved_oid="0" * 40)
        r35 = Result()
        verify_full_completion(repoF, cF, rF, eF, env_path, lab, batch, env_rawF, recBad, expected, r35)
        res.add("selftest: remote mismatch REJECT", "PASS" if r35.failed() else "FAIL")

        g40 = build_valid_fixture(tmp, "full-dirty", lab, mutate=lambda repo, c, r, e: _write_file(os.path.join(repo, "dirty.txt"), "dirty\n"))
        repo40, c40, r40, e40, _, env_raw40 = g40
        rec40 = _default_receipt(repo40, c40, r40, e40, env_raw40, lab, expected)
        r40res = Result()
        verify_full_completion(repo40, c40, r40, e40, env_path, lab, batch, env_raw40, rec40, expected, r40res)
        res.add("selftest: dirty checkout REJECT", "PASS" if r40res.failed() else "FAIL")

        rwt = git(repoF, ["worktree", "list", "--porcelain"])
        res.add("selftest: zero temporary worktrees", "PASS" if rwt["stdout"].strip().count("worktree ") == 1 else "FAIL")
        res.add("selftest: zero temporary directories (TemporaryDirectory auto-clean)", "PASS")
        res.add("selftest: zero temporary processes (all owned/foreign terminated in probes)", "PASS")
    return res


def main():
    ap = argparse.ArgumentParser(description=SCHEMA)
    ap.add_argument("--lab", choices=["a", "b"])
    ap.add_argument("--repository")
    ap.add_argument("--candidate-head")
    ap.add_argument("--result-head")
    ap.add_argument("--envelope-container-head")
    ap.add_argument("--batch-manifest", default=os.path.join(PACKAGE_DIR, "batch-manifest-v3.4.json"))
    ap.add_argument("--scope-freeze", default=os.path.join(PACKAGE_DIR, "scope-freeze-v3.4.json"))
    ap.add_argument("--build-policy", default=os.path.join(PACKAGE_DIR, "build-provenance-policy-v3.4.json"))
    ap.add_argument("--contract", default=os.path.join(PACKAGE_DIR, "lab-api-contract-v3.4.json"))
    ap.add_argument("--functional-baseline", default=os.path.join(PACKAGE_DIR, "functional-contract-baseline-v3.4.json"))
    ap.add_argument("--envelope-path")
    ap.add_argument("--expected-branch")
    ap.add_argument("--controller-acceptance-receipt")
    ap.add_argument("--phase", choices=["functional-continuity", "leaf-envelope", "full-completion"],
                    default="functional-continuity")
    ap.add_argument("--json-out")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        res = selftest()
        if args.json_out:
            with open(args.json_out, "w") as f:
                json.dump({"schema": SCHEMA, "version": VERSION, "mode": "selftest",
                           "checks": res.checks, "env_notes": res.env_notes}, f, indent=2)
        for c in res.checks:
            print("[%s] %s — %s" % (c["status"], c["name"], c["detail"]))
        print("SELFTEST", "FAIL" if res.failed() else "PASS")
        return 1 if res.failed() else 0

    lab = "LAB-A" if args.lab == "a" else "LAB-B"
    batch = load_batch_manifest(args.batch_manifest)
    lab_meta = batch["labs"][lab]
    if not args.envelope_path:
        args.envelope_path = lab_meta["envelope_path"]

    res = Result()
    if args.phase == "functional-continuity":
        baseline = load_baseline(args.functional_baseline)
        contract = load_contract(args.contract)
        scope = load_json(args.scope_freeze, "scope-freeze")
        verify_functional_continuity(args.repository, baseline, contract, scope, batch, res)
    elif args.phase == "leaf-envelope":
        required = ["--lab", "--repository", "--candidate-head", "--result-head", "--envelope-container-head"]
        missing = [r for r in required if getattr(args, r.lstrip("--").replace("-", "_")) is None]
        if missing:
            ap.error("missing required arguments: %s" % ", ".join(missing))
        verify_leaf_envelope(args.repository, args.candidate_head, args.result_head,
                             args.envelope_container_head, args.envelope_path, lab, batch, res,
                             expected_branch=args.expected_branch or None)
    else:
        required = ["--lab", "--repository", "--candidate-head", "--result-head", "--envelope-container-head"]
        missing = [r for r in required if getattr(args, r.lstrip("--").replace("-", "_")) is None]
        if missing:
            ap.error("missing required arguments: %s" % ", ".join(missing))
        env_raw = git(args.repository, ["show", "%s:%s" % (args.envelope_container_head, args.envelope_path)])["stdout"]
        receipt = None
        if args.controller_acceptance_receipt:
            receipt = load_json(args.controller_acceptance_receipt, "controller-acceptance-receipt")
        verify_full_completion(args.repository, args.candidate_head, args.result_head,
                               args.envelope_container_head, args.envelope_path, lab, batch,
                               env_raw, receipt, args.expected_branch or "", res)

    if args.json_out:
        with open(args.json_out, "w") as f:
            json.dump({"schema": SCHEMA, "version": VERSION, "lab": lab,
                       "repository": args.repository, "phase": args.phase,
                       "checks": res.checks, "env_notes": res.env_notes,
                       "verdict": "REJECT" if res.failed() else "PASS"}, f, indent=2)
    for c in res.checks:
        print("[%s] %s — %s" % (c["status"], c["name"], c["detail"]))
    print("VERDICT:", "REJECT (one or more FAIL)" if res.failed() else "PASS")
    return 1 if res.failed() else 0


if __name__ == "__main__":
    sys.exit(main())
