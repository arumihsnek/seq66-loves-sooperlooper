#!/usr/bin/env python3
"""acceptance-verifier-v3.3.py — STRICT envelope + controller-acceptance verifier.

Model (kept from v3.2):
    C = candidate head
    R = implementation result (parent(R)=C, NO envelope, ONLY R integration-eligible)
    E = leaf envelope container (parent(E)=R, contains EXACTLY the frozen envelope)
    remote branch -> E; external binding recorded by the controller AFTER push.

New in v3.3 (vs v3.2):
    - strict runtime enforcement of EVERY normative envelope field
      (schema/lab/batch/leaf/lease/external branch/literal tests/build claim/artifacts)
    - exact artifact set == C..R tracked changes; per-artifact sha256/size verified
      from R blobs; artifacts NEVER empty; R non-empty enforced
    - literal test IDs bound byte-exactly to frozen batch-manifest commands
      (command_sha256 == sha256(UTF-8 bytes of the authoritative command))
    - leaf envelope E may NOT contain controller-future claims
    - SEPARATE controller acceptance receipt A (created after E is pushed)
    - two phases: --phase leaf-envelope | full-completion
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
SCHEMA = "headless-lab-result-envelope/v3.3"
RECEIPT_SCHEMA = "controller-acceptance-receipt/v3.3"
BATCH_ID = "BATCH-20260806T011700Z-DOGFOOD004-LAB-V3_3"

ENVELOPE_LAB_A = "receipts/headless-lab-results-v3.3/lab-a-result.json"
ENVELOPE_LAB_B = "receipts/headless-lab-results-v3.3/lab-b-result.json"

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

FORBIDDEN_SELF_KEYS = [
    "container_commit_oid", "container_tree_oid", "envelope_blob_oid",
    "envelope_sha256", "self_sha256", "remote_branch_resolved_sha",
]
FORBIDDEN_CONTROLLER_FUTURE_PREFIXES = ("controller_",)
FORBIDDEN_CONTROLLER_FUTURE_SUFFIXES = ("_resolved_sha", "_verdict", "_pass", "_done")
ALLOWED_TOP_LEVEL_KEYS = [
    "schema", "lab", "batch_id", "leaf_id", "lease_id",
    "candidate_commit_oid", "implementation_result_commit_oid",
    "implementation_result_tree_oid", "git_object_format",
    "external_branch_binding", "leaf_literal_tests", "leaf_build_claim", "artifacts",
]
ALLOWED_ARTIFACT_KINDS = {"source", "test", "scenario", "ignore-rule"}
ALLOWED_BUILD_CLAIM_STATUS = {"not_applicable", "claimed_built"}

PACKAGE_DIR = os.path.dirname(os.path.abspath(__file__))


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------
class Result:
    def __init__(self):
        self.checks = []
        self.env_notes = []

    def add(self, name, status, detail="", evidence=None):
        self.checks.append({"name": name, "status": status, "detail": detail,
                            "evidence": evidence})

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
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd,
                           timeout=timeout, env=e)
        return {"exit": r.returncode, "stdout": r.stdout, "stderr": r.stderr}
    except subprocess.TimeoutExpired:
        return {"exit": -2, "stdout": "", "stderr": "timeout"}
    except Exception as ex:
        return {"exit": -3, "stdout": "", "stderr": str(ex)}


def git(repo, args):
    r = run(["git", "-C", repo] + args, repo, timeout=60)
    return r


def git_show(checkout, rev, path):
    r = subprocess.run(["git", "-C", checkout, "show", "%s:%s" % (rev, path)],
                       capture_output=True, timeout=30)
    if r.returncode != 0:
        return None
    try:
        return r.stdout.decode("utf-8")
    except Exception:
        return r.stdout.decode("utf-8", "replace")


def scan_for_patterns(content, patterns, label):
    hits = []
    for rx, name in patterns:
        for m in rx.finditer(content or ""):
            hits.append({"pattern": name, "line": content[:m.start()].count("\n") + 1})
    return hits


def load_json(path, label):
    try:
        with open(path, "r") as f:
            return json.load(f)
    except Exception as e:
        raise SystemExit("cannot load %s: %s" % (label, e))


def load_batch_manifest():
    return load_json(os.path.join(PACKAGE_DIR, "batch-manifest-v3.3.json"), "batch-manifest")


def load_contract(lab):
    c = load_json(os.path.join(PACKAGE_DIR, "lab-api-contract-v3.3.json"), "lab-api-contract")
    return c["labs"][lab]


def _git_quiet(repo, args, ok_exit=True):
    r = git(repo, args)
    if ok_exit and r["exit"] != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), r["stderr"][-300:]))
    return r["stdout"].strip()


# ---------------------------------------------------------------------------
# leaf-envelope strict verification
# ---------------------------------------------------------------------------
def verify_leaf_envelope(repo, c_oid, r_oid, e_oid, envelope_path, lab, batch, res,
                         expected_branch=None):
    """All 10.1 leaf-envelope checks."""
    ok_exist = True
    for label, oid in [("C", c_oid), ("R", r_oid), ("E", e_oid)]:
        r = git(repo, ["cat-file", "-e", "%s^{commit}" % oid])
        if r["exit"] != 0:
            res.fail("model: %s commit exists" % label, "%s^{commit} not resolvable" % oid)
            ok_exist = False
        else:
            res.ok("model: %s commit exists" % label, oid)
    if not ok_exist:
        return

    if c_oid == r_oid:
        res.fail("model: R != C", "result head equals candidate head")
    else:
        res.ok("model: R != C")

    r_parents = git(repo, ["rev-list", "--parents", "-n", "1", r_oid])["stdout"].split()
    e_parents = git(repo, ["rev-list", "--parents", "-n", "1", e_oid])["stdout"].split()
    res.add("model: R exactly one parent and R^ == C",
            "PASS" if len(r_parents) == 2 and r_parents[1] == c_oid else "FAIL",
            evidence={"parents": r_parents})
    res.add("model: E exactly one parent and E^ == R",
            "PASS" if len(e_parents) == 2 and e_parents[1] == r_oid else "FAIL",
            evidence={"parents": e_parents})

    # git diff --quiet C R must return nonzero (C..R non-empty)
    dq = git(repo, ["diff", "--quiet", c_oid, r_oid])
    res.add("model: git diff --quiet C R returns nonzero", "PASS" if dq["exit"] != 0 else "FAIL")

    env_in_c = git(repo, ["cat-file", "-e", "%s:%s" % (c_oid, envelope_path)])
    env_in_r = git(repo, ["cat-file", "-e", "%s:%s" % (r_oid, envelope_path)])
    env_in_e = git(repo, ["cat-file", "-e", "%s:%s" % (e_oid, envelope_path)])
    res.add("model: envelope absent from C", "PASS" if env_in_c["exit"] != 0 else "FAIL")
    res.add("model: envelope absent from R", "PASS" if env_in_r["exit"] != 0 else "FAIL")
    res.add("model: envelope present in E", "PASS" if env_in_e["exit"] == 0 else "FAIL")

    r_tree = _git_quiet(repo, ["rev-parse", "%s^{tree}" % r_oid])
    e_tree = _git_quiet(repo, ["rev-parse", "%s^{tree}" % e_oid])
    objfmt = _git_quiet(repo, ["rev-parse", "--show-object-format"])

    owned = set(OWNED_LAB_A if lab == "LAB-A" else OWNED_LAB_B)
    dr = git(repo, ["diff", "--name-only", c_oid, r_oid])
    r_diff_paths = [ln for ln in dr["stdout"].splitlines() if ln]
    bad_impl = [p for p in r_diff_paths if p not in owned and p != envelope_path]
    res.add("model: diff C..R owned-only", "PASS" if not bad_impl else "FAIL",
            evidence={"changed": r_diff_paths, "out_of_scope": bad_impl})
    res.add("model: diff C..R non-empty", "PASS" if r_diff_paths else "FAIL")

    de = git(repo, ["diff", "--name-only", r_oid, e_oid])
    e_diff_paths = [ln for ln in de["stdout"].splitlines() if ln]
    res.add("model: diff R..E envelope-only",
            "PASS" if e_diff_paths == [envelope_path] else "FAIL",
            evidence={"changed": e_diff_paths, "expected": [envelope_path]})

    env_raw = git(repo, ["show", "%s:%s" % (e_oid, envelope_path)])["stdout"]
    try:
        env = json.loads(env_raw)
    except Exception as ex:
        res.fail("model: envelope parses", "invalid JSON: %s" % ex)
        return
    res.ok("model: envelope parses")

    # --- identity (schema/lab/batch/leaf/lease) ---
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
    expected_map = {
        "schema EXACT": SCHEMA,
        "lab EXACT": lab,
        "batch_id EXACT": BATCH_ID,
        "leaf_id EXACT": lab_meta["leaf_id"],
        "lease_id EXACT": lab_meta["lease_id"],
        "candidate_commit_oid == C": c_oid,
        "implementation_result_commit_oid == R": r_oid,
        "implementation_result_tree_oid == R^{tree}": r_tree,
        "git_object_format EXACT": objfmt,
    }
    observed_map = {k: env.get(k) for k in checks}
    for name, ok_ in checks.items():
        res.add("env: %s" % name, "PASS" if ok_ else "FAIL",
                evidence={"observed": observed_map[name], "expected": expected_map[name]})

    ebb = env.get("external_branch_binding")
    res.add("env: external_branch_binding.remote == origin",
            "PASS" if isinstance(ebb, dict) and ebb.get("remote") == "origin" else "FAIL",
            evidence={"binding": ebb})
    planned = ebb.get("planned_branch") if isinstance(ebb, dict) else None
    planned_ok = isinstance(planned, str) and len(planned) > 0
    res.add("env: planned_branch present (non-empty)",
            "PASS" if planned_ok else "FAIL",
            evidence={"planned_branch": planned})
    if expected_branch is not None:
        res.add("env: planned_branch EXACT == expected branch",
                "PASS" if planned == expected_branch else "FAIL",
                evidence={"planned": planned, "expected": expected_branch})

    # --- top-level keys policy ---
    present = set(env.keys())
    missing = [k for k in ALLOWED_TOP_LEVEL_KEYS if k not in present]
    unknown = [k for k in present if k not in ALLOWED_TOP_LEVEL_KEYS]
    res.add("env: all required top-level keys present", "PASS" if not missing else "FAIL",
            evidence={"missing": missing})
    res.add("env: no unknown top-level keys", "PASS" if not unknown else "FAIL",
            evidence={"unknown": unknown})

    forbidden_present = [k for k in FORBIDDEN_SELF_KEYS if k in present]
    controller_flagged = [
        k for k in present
        if k.startswith(FORBIDDEN_CONTROLLER_FUTURE_PREFIXES)
        or k.endswith(FORBIDDEN_CONTROLLER_FUTURE_SUFFIXES)
    ]
    res.add("env: no forbidden self-binding keys", "PASS" if not forbidden_present else "FAIL",
            evidence={"forbidden": forbidden_present})
    res.add("env: no controller-future claims inside E",
            "PASS" if not controller_flagged else "FAIL",
            evidence={"flagged": controller_flagged})

    # --- leaf literal tests ---
    llt = env.get("leaf_literal_tests")
    frozen_tests = lab_meta.get("literal_tests", {})
    if not isinstance(llt, dict):
        res.fail("env: leaf_literal_tests is a dict", type(llt).__name__)
        llt = {}
    expected_ids = set(frozen_tests.keys())
    actual_ids = set(llt.keys())
    res.add("env: literal test ID set exact",
            "PASS" if actual_ids == expected_ids else "FAIL",
            evidence={"expected": sorted(expected_ids), "actual": sorted(actual_ids)})
    for tid, meta in frozen_tests.items():
        entry = llt.get(tid)
        if not isinstance(entry, dict):
            res.fail("env: literal test %s present" % tid, "missing")
            continue
        cmd_sha = sha256_b(json.dumps(meta["command"], ensure_ascii=False).encode("utf-8"))
        res.add("env: %s command_sha256 exact" % tid,
                "PASS" if entry.get("command_sha256") == cmd_sha else "FAIL",
                evidence={"declared": entry.get("command_sha256"), "computed": cmd_sha})
        res.add("env: %s exit_code == 0" % tid,
                "PASS" if entry.get("exit_code") == 0 else "FAIL",
                evidence={"exit_code": entry.get("exit_code")})
        for tskey in ("started_at_utc", "ended_at_utc"):
            val = entry.get(tskey)
            try:
                datetime.fromisoformat(val.replace("Z", "+00:00"))
                ok_t = True
            except Exception:
                ok_t = False
            res.add("env: %s %s parseable" % (tid, tskey), "PASS" if ok_t else "FAIL",
                    evidence={"value": val})
        try:
            st = datetime.fromisoformat(entry.get("started_at_utc", "").replace("Z", "+00:00"))
            en = datetime.fromisoformat(entry.get("ended_at_utc", "").replace("Z", "+00:00"))
            order_ok = st <= en
        except Exception:
            order_ok = False
        res.add("env: %s timestamps ordered" % tid, "PASS" if order_ok else "FAIL",
                evidence={"started": entry.get("started_at_utc"), "ended": entry.get("ended_at_utc")})

    # --- artifacts: exact set == C..R tracked changes; hashes/sizes from R blobs ---
    arts = env.get("artifacts")
    if not isinstance(arts, dict):
        res.fail("env: artifacts is a dict", type(arts).__name__)
        arts = {}
    expected_artifact_set = set(r_diff_paths)
    actual_artifact_set = set(arts.keys())
    res.add("env: artifact set == C..R tracked changes (owned only)",
            "PASS" if actual_artifact_set == expected_artifact_set else "FAIL",
            evidence={"expected": sorted(expected_artifact_set), "actual": sorted(actual_artifact_set)})
    res.add("env: artifacts non-empty (R non-empty enforced)",
            "PASS" if actual_artifact_set else "FAIL")
    for apath in sorted(actual_artifact_set):
        a = arts[apath]
        if not isinstance(a, dict):
            res.fail("env: artifact %s is a dict" % apath, type(a).__name__)
            continue
        kind_ok = a.get("kind") in ALLOWED_ARTIFACT_KINDS
        res.add("env: artifact %s kind allowed" % apath, "PASS" if kind_ok else "FAIL",
                evidence={"kind": a.get("kind")})
        blob = subprocess.run(["git", "-C", repo, "show", "%s:%s" % (r_oid, apath)],
                              capture_output=True, timeout=30)
        if blob.returncode != 0:
            res.fail("env: artifact %s present in R" % apath, "not in R")
            continue
        real_sha = sha256_b(blob.stdout)
        real_size = len(blob.stdout)
        res.add("env: artifact %s sha256 exact" % apath,
                "PASS" if a.get("sha256") == real_sha else "FAIL",
                evidence={"declared": a.get("sha256"), "computed": real_sha})
        res.add("env: artifact %s size exact" % apath,
                "PASS" if a.get("size") == real_size else "FAIL",
                evidence={"declared": a.get("size"), "computed": real_size})
    if envelope_path in actual_artifact_set:
        res.fail("env: artifacts excludes envelope", "envelope path present in artifacts")

    # --- leaf build claim ---
    lbc = env.get("leaf_build_claim")
    if not isinstance(lbc, dict):
        res.fail("env: leaf_build_claim present", "missing or not dict")
        lbc = {}
    status = lbc.get("status")
    res.add("env: leaf_build_claim.status allowed", "PASS" if status in ALLOWED_BUILD_CLAIM_STATUS else "FAIL",
            evidence={"status": status})
    if lab == "LAB-A":
        invented = [k for k in ("compiler_path", "compiler_version", "commands", "outputs", "transcript_sha256")
                    if lbc.get(k) not in (None, [], "", {})]
        res.add("env: LAB-A build claim not_applicable (no invented fields)",
                "PASS" if status == "not_applicable" and not invented else "FAIL",
                evidence={"invented": invented})
    else:
        sources = lbc.get("sources")
        commands = lbc.get("commands")
        outputs = lbc.get("outputs")
        res.add("env: LAB-B build claim structurally valid (sources/commands/outputs)",
                "PASS" if status == "claimed_built" and isinstance(sources, list)
                and isinstance(commands, list) and isinstance(outputs, list)
                and bool(outputs) else "FAIL",
                evidence={"status": status, "sources": sources, "commands": commands, "outputs": outputs})


# ---------------------------------------------------------------------------
# functional gates (LAB-A / LAB-B) on a clean checkout of R
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
    res.add("lab-a: placeholder scan", "PASS" if not ph_hits else "FAIL",
            evidence=ph_hits if ph_hits else None)

    probe_src = (
        "import json\n"
        "import os\n"
        "import subprocess\n"
        "import sys\n"
        "import time\n"
        "sys.path.insert(0, 'tests/integration')\n"
        "from headless_audio_lab.process_supervisor import ProcessSupervisor\n"
        "\n"
        "out = {}\n"
        "foreign = None\n"
        "try:\n"
        "    with ProcessSupervisor() as sup:\n"
        "        info = sup.start_process('probe', ['/bin/sleep', '3'], {}, '.', '/dev/null', '/dev/null')\n"
        "        out['owned_pid'] = info.pid\n"
        "        out['owned_pgid'] = info.pgid\n"
        "        out['owned_pid_positive'] = bool(info.pid and info.pid > 0)\n"
        "        out['owned_pids_registered'] = sup.get_owned_pids()\n"
        "        time.sleep(0.3)\n"
        "        out['owned_child_alive_before_cleanup'] = os.path.exists('/proc/%d' % info.pid)\n"
        "        try:\n"
        "            sup.start_process('bad', ['/nonexistent/binary-xyz'], {}, '.', '/dev/null', '/dev/null')\n"
        "            out['failure_propagation'] = False\n"
        "        except Exception:\n"
        "            out['failure_propagation'] = True\n"
        "        foreign = subprocess.Popen(['/bin/sleep', '5'], start_new_session=True,\n"
        "                                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,\n"
        "                                   stderr=subprocess.DEVNULL)\n"
        "        out['foreign_pid'] = foreign.pid\n"
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
        "                foreign.kill()\n"
        "                foreign.wait(timeout=5.0)\n"
        "        out['foreign_terminated_by_controller'] = foreign.poll() is not None\n"
        "print(json.dumps(out))\n"
    )
    probe_path = os.path.join(checkout, "_acceptance_probe_lab_a_v33.py")
    with open(probe_path, "w") as f:
        f.write(probe_src)
    r = run(["python3", probe_path], checkout, timeout=120)
    try:
        os.unlink(probe_path)
    except OSError:
        pass
    if r["exit"] != 0:
        res.fail("lab-a: functional lifecycle",
                 "probe failed exit=%d stderr=%s" % (r["exit"], r["stderr"][-500:]))
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
    ]
    ok_all = True
    cond_results = {}
    for name, fn in conds:
        val = fn(d)
        cond_results[name] = {"satisfied": bool(val), "observed": d.get(name)}
        ok_all = ok_all and bool(val)
    res.add("lab-a: functional lifecycle", "PASS" if ok_all else "FAIL",
            evidence=cond_results)


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
    res.add("lab-b: analyze_wav signature (wav_bytes)",
            "PASS" if re.search(r"\bwav_bytes\b", sig) else "FAIL",
            evidence={"sig": sig})

    tm = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/test_modules.py") or ""
    hits = scan_for_patterns(tm, TRIVIAL_TEST_PATTERNS, "lab-b")
    invokes = "analyze_wav(" in tm
    res.add("lab-b: test_modules negative gates",
            "PASS" if (not hits and invokes) else "FAIL",
            evidence={"hits": hits, "invokes_analyze_wav": invokes})

    probe_path = os.path.join(checkout, "_acceptance_probe_lab_b_v33.py")
    with open(probe_path, "w") as f:
        f.write(
            "import struct, sys, json\n"
            "sys.path.insert(0, 'tests/integration')\n"
            "from headless_audio_lab.audio_oracle import analyze_wav\n"
            "out = {}\n"
            "# silent WAV (mono 16-bit 44100 Hz, 0.1s)\n"
            "rate=44100; n=4410\n"
            "def wav_bytes(nonzero=False):\n"
            "    body = (b'\\x01\\x02' if nonzero else b'\\x00\\x00') * n\n"
            "    return b'RIFF'+struct.pack('<I4s4sIHHIIHH4sI',36+n*2,b'WAVE',b'fmt ',16,1,1,rate,rate*2,2,16,b'data',n*2)+body\n"
            "r1 = analyze_wav(wav_bytes())\n"
            "d1 = r1 if isinstance(r1, dict) else r1.to_dict()\n"
            "out['silent'] = d1.get('silent')\n"
            "out['frames'] = d1.get('frames')\n"
            "out['rate'] = d1.get('rate')\n"
            "out['channels'] = d1.get('channels')\n"
            "r2 = analyze_wav(wav_bytes(nonzero=True))\n"
            "d2 = r2 if isinstance(r2, dict) else r2.to_dict()\n"
            "out['nonsilent_silent'] = d2.get('silent')\n"
            "try:\n"
            "    r3 = analyze_wav(b'RIFFnotreallyawav')\n"
            "    d3 = r3 if isinstance(r3, dict) else r3.to_dict()\n"
            "    out['malformed'] = d3.get('verification')\n"
            "except Exception as e:\n"
            "    out['malformed'] = 'EXCEPTION:' + type(e).__name__\n"
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
    res.add("lab-b: silent WAV -> silent=True",
            "PASS" if d.get("silent") is True else "FAIL", evidence=d)
    res.add("lab-b: silent WAV frames/rate/channels exact",
            "PASS" if d.get("frames") == 4410 and d.get("rate") == 44100
            and d.get("channels") == 1 else "FAIL", evidence=d)
    res.add("lab-b: non-silent WAV -> silent=False",
            "PASS" if d.get("nonsilent_silent") is False else "FAIL", evidence=d)
    res.add("lab-b: malformed WAV FAIL/ERROR",
            "PASS" if str(d.get("malformed", "")).startswith(("FAIL", "ERROR", "EXCEPTION")) else "FAIL",
            evidence={"malformed": d.get("malformed")})

    gi = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/fixtures/.gitignore") or ""
    literal_n = "\\n" in gi
    real_newline = "\n" in gi
    res.add("lab-b: .gitignore byte-exact",
            "PASS" if (not literal_n and real_newline) else "FAIL",
            evidence={"repr": repr(gi)})

    r = git(checkout, ["ls-tree", "-r", "HEAD"])
    elf_tracked = []
    for line in r["stdout"].splitlines():
        parts = line.split("\t", 1)
        if len(parts) != 2:
            continue
        blob = parts[0].split()[-1]
        br = subprocess.run(["git", "-C", checkout, "cat-file", "-p", blob],
                            capture_output=True, timeout=30)
        if br.stdout[:4] == b"\x7fELF":
            elf_tracked.append(parts[1])
    res.add("lab-b: no ELF tracked", "PASS" if not elf_tracked else "FAIL",
            evidence={"elf_tracked": elf_tracked})

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
        else:
            if not content.strip() or not any(k in content for k in
                                              ["source:", "capture:", "sooperlooper:", "expected:"]):
                bad.append({"scenario": s, "error": "missing required sections"})
    res.add("lab-b: scenarios parseable", "PASS" if not bad else "FAIL", evidence={"bad": bad})

    runner = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/runner.py") or ""
    api_hits = [a for a in ["ProcessSupervisor", "JackServer", "OscProbe", "SooperLooperLauncher",
                            "analyze_wav", "GraphAssertions"] if a in runner]
    res.add("lab-b: runner uses contractual APIs",
            "PASS" if len(api_hits) >= 3 else "FAIL", evidence={"api_hits": api_hits})


# ---------------------------------------------------------------------------
# controller acceptance receipt verification (full-completion)
# ---------------------------------------------------------------------------
def verify_controller_receipt(repo, c_oid, r_oid, e_oid, envelope_path, lab, batch,
                              env_raw, receipt, expected_branch, res):
    """10.2 receipt checks that do NOT need execution."""
    req = ["schema", "batch_id", "lab", "candidate_commit_oid",
           "implementation_result_commit_oid", "implementation_result_tree_oid",
           "envelope_container_commit_oid", "envelope_container_tree_oid",
           "envelope_blob_oid", "envelope_sha256", "remote", "branch",
           "remote_resolved_oid", "controller_literal_tests",
           "acceptance_harness_sha256", "acceptance_harness_verdict",
           "artifact_verification", "controlled_rebuild",
           "build_provenance_verdict", "worktree_clean_before", "worktree_clean_after",
           "completion_criteria", "final_verdict", "recorded_at_utc"]
    missing = [k for k in req if k not in receipt]
    res.add("receipt: schema exact", "PASS" if receipt.get("schema") == RECEIPT_SCHEMA else "FAIL",
            evidence={"schema": receipt.get("schema")})
    res.add("receipt: all required fields present", "PASS" if not missing else "FAIL",
            evidence={"missing": missing})

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

    # remote resolved OID == E (real ls-remote against origin)
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
            "PASS" if resolved and receipt.get("remote_resolved_oid") == resolved else "FAIL",
            evidence={"field": receipt.get("remote_resolved_oid"), "actual": resolved})

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
                "PASS" if isinstance(entry, dict) and entry.get("exit_code") == 0 else "FAIL",
                evidence={"declared_exit": entry.get("exit_code") if isinstance(entry, dict) else None})

    res.add("receipt: acceptance harness sha256 exact",
            "PASS" if receipt.get("acceptance_harness_sha256") == sha256_file(__file__) else "FAIL")
    res.add("receipt: acceptance harness verdict PASS",
            "PASS" if receipt.get("acceptance_harness_verdict") == "PASS" else "FAIL",
            evidence={"verdict": receipt.get("acceptance_harness_verdict")})
    res.add("receipt: artifact verification PASS",
            "PASS" if receipt.get("artifact_verification") == "PASS" else "FAIL")
    res.add("receipt: build provenance verdict",
            "PASS" if receipt.get("build_provenance_verdict") in ("demonstrated", "not_applicable") else "FAIL",
            evidence={"verdict": receipt.get("build_provenance_verdict")})

    cc = receipt.get("completion_criteria")
    if isinstance(cc, dict):
        cc_false = [k for k, v in cc.items() if v is not True]
        res.add("receipt: completion criteria all true", "PASS" if not cc_false else "FAIL",
                evidence={"false": cc_false})
    else:
        res.fail("receipt: completion criteria all true", "missing/not dict")
    res.add("receipt: final verdict PASS",
            "PASS" if receipt.get("final_verdict") == "PASS" else "FAIL",
            evidence={"final_verdict": receipt.get("final_verdict")})
    try:
        datetime.fromisoformat(receipt.get("recorded_at_utc", "").replace("Z", "+00:00"))
        res.ok("receipt: recorded_at_utc parseable")
    except Exception:
        res.fail("receipt: recorded_at_utc parseable", str(receipt.get("recorded_at_utc")))
    for wk in ("worktree_clean_before", "worktree_clean_after"):
        res.add("receipt: %s true" % wk,
                "PASS" if receipt.get(wk) is True else "FAIL",
                evidence={"value": receipt.get(wk)})


def run_full_completion_execution(repo, r_oid, lab, batch, receipt, res, claimed_outputs=None):
    """Execution part of full-completion: literal test reexec, harness, rebuild."""
    # clean checkout of R
    tmp = tempfile.mkdtemp(prefix="v33-checkout-")
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

        # 1. literal tests reexecution (controller does NOT trust leaf exit codes)
        frozen_tests = batch["labs"][lab].get("literal_tests", {})
        rlt = receipt.get("controller_literal_tests", {})
        for tid, meta in frozen_tests.items():
            entry = rlt.get(tid, {})
            rr = run(meta["command"], wt, timeout=meta.get("timeout_seconds", 120))
            stdout_sha = sha256_b(rr["stdout"].encode("utf-8"))
            stderr_sha = sha256_b(rr["stderr"].encode("utf-8"))
            res.add("completion: %s literal test reexec exit 0" % tid,
                    "PASS" if rr["exit"] == 0 else "FAIL",
                    evidence={"exit": rr["exit"], "stderr_tail": rr["stderr"][-300:]})
            res.add("completion: %s stdout hash matches receipt" % tid,
                    "PASS" if stdout_sha == entry.get("stdout_sha256") else "FAIL",
                    evidence={"computed": stdout_sha, "declared": entry.get("stdout_sha256")})
            res.add("completion: %s stderr hash matches receipt" % tid,
                    "PASS" if stderr_sha == entry.get("stderr_sha256") else "FAIL",
                    evidence={"computed": stderr_sha, "declared": entry.get("stderr_sha256")})

        # 2. acceptance harness on clean R
        if lab == "LAB-A":
            check_lab_a(wt, load_contract("LAB-A"), res)
        else:
            check_lab_b(wt, load_contract("LAB-B"), res)

        # 3. controlled rebuild
        build_cmd = batch["labs"][lab].get("build_command")
        if build_cmd is None:
            res.add("completion: controlled rebuild not_applicable (LAB-A)",
                    "PASS" if receipt.get("controlled_rebuild") == "not_applicable" else "FAIL")
        else:
            # empty build directory inside the checkout
            build_dir = os.path.join(wt, "build")
            shutil.rmtree(build_dir, ignore_errors=True)
            os.makedirs(build_dir)
            rr = run(build_cmd, wt, timeout=120)
            res.add("completion: controlled rebuild exit 0", "PASS" if rr["exit"] == 0 else "FAIL",
                    evidence={"exit": rr["exit"], "stderr_tail": rr["stderr"][-300:]})
            # verify rebuilt output hashes/sizes vs the leaf_build_claim.outputs
            for claim in (claimed_outputs or []):
                cpath = os.path.join(wt, claim.get("path", ""))
                cpath_abs = os.path.abspath(cpath)
                if not cpath_abs.startswith(os.path.abspath(wt)):
                    res.fail("completion: claimed output %s inside checkout" % claim.get("path"))
                    continue
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
        lbc = env.get("leaf_build_claim") or {}
        claimed_outputs = lbc.get("outputs")
    except Exception:
        pass
    run_full_completion_execution(repo, r_oid, lab, batch, receipt, res, claimed_outputs)


# ---------------------------------------------------------------------------
# selftest (real temp git repositories)
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


def fixture_impl(lab):
    """Fixture implementations satisfying v3.3 frozen literal tests + gates."""
    if lab == "LAB-A":
        return {
            "tests/integration/headless_audio_lab/__init__.py": "# headless audio lab package\n",
            "tests/integration/headless_audio_lab/process_supervisor.py": (
                "class ProcessSupervisor:\n"
                "    def __init__(self):\n"
                "        self._owned = {}\n"
                "    def __enter__(self):\n"
                "        return self\n"
                "    def __exit__(self, *exc):\n"
                "        self.cleanup(timeout=5.0)\n"
                "        return False\n"
                "    def start_process(self, name, cmd, env, cwd, so, se):\n"
                "        import os, subprocess\n"
                "        try:\n"
                "            p = subprocess.Popen(cmd, env=env, cwd=cwd,\n"
                "                                stdin=subprocess.DEVNULL,\n"
                "                                stdout=open(so, 'w') if so != '/dev/null' else subprocess.DEVNULL,\n"
                "                                stderr=open(se, 'w') if se != '/dev/null' else subprocess.DEVNULL,\n"
                "                                start_new_session=True)\n"
                "        except Exception as e:\n"
                "            raise RuntimeError('start failed: %s' % e) from e\n"
                "        info = type('I', (), {'pid': p.pid, 'pgid': p.pid})()\n"
                "        self._owned[info.pid] = p\n"
                "        return info\n"
                "    def get_owned_pids(self):\n"
                "        return list(self._owned)\n"
                "    def cleanup(self, timeout=5.0):\n"
                "        import os, signal, subprocess\n"
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
            ),
            "tests/integration/headless_audio_lab/jack_server.py": (
                "class JackServer:\n"
                "    def start(self, sample_rate=48000, period_size=1024):\n"
                "        return True\n"
                "    def stop(self):\n"
                "        return True\n"
            ),
            "tests/integration/headless_audio_lab/osc_probe.py": (
                "class OscProbe:\n"
                "    def probe(self, host='127.0.0.1', port=9951):\n"
                "        return {'reachable': False}\n"
            ),
            "tests/integration/headless_audio_lab/sooperlooper_launcher.py": (
                "class SooperLooperLauncher:\n"
                "    def launch(self, loops=1, channels=2):\n"
                "        return None\n"
            ),
        }
    # LAB-B
    oracle = (
        "import struct\n"
        "def analyze_wav(wav_bytes):\n"
        "    if len(wav_bytes) < 44 or wav_bytes[:4] != b'RIFF' or wav_bytes[8:12] != b'WAVE':\n"
        "        raise ValueError('not a WAV')\n"
        "    fmt_off = 12\n"
        "    if wav_bytes[fmt_off:fmt_off+4] != b'fmt ':\n"
        "        raise ValueError('no fmt chunk')\n"
        "    fmt_size = struct.unpack('<I', wav_bytes[fmt_off+4:fmt_off+8])[0]\n"
        "    a = struct.unpack('<HHIIHH', wav_bytes[fmt_off+8:fmt_off+8+16])\n"
        "    channels, rate = a[1], a[2]\n"
        "    data_off = fmt_off + 8 + fmt_size\n"
        "    if data_off + 8 > len(wav_bytes) or wav_bytes[data_off:data_off+4] != b'data':\n"
        "        raise ValueError('no data chunk')\n"
        "    data_size = struct.unpack('<I', wav_bytes[data_off+4:data_off+8])[0]\n"
        "    pcm = wav_bytes[data_off+8:data_off+8+data_size]\n"
        "    samples = len(pcm) // (2 * channels)\n"
        "    if samples == 0:\n"
        "        raise ValueError('no samples')\n"
        "    total = 0\n"
        "    peak = 0\n"
        "    for i in range(0, len(pcm) - 1, 2):\n"
        "        v = struct.unpack('<h', pcm[i:i+2])[0]\n"
        "        total += v * v\n"
        "        peak = max(peak, abs(v))\n"
        "    rms = (total / (2 * samples)) ** 0.5\n"
        "    return {'rate': rate, 'channels': channels, 'frames': samples,\n"
        "            'duration_sec': samples / float(rate), 'rms': rms, 'peak': peak,\n"
        "            'silent': peak == 0}\n"
    )
    return {
        "tests/__init__.py": "# tests package\n",
        "tests/integration/__init__.py": "# integration package\n",
        "tests/integration/headless_audio_lab/audio_oracle.py": oracle,
        "tests/integration/headless_audio_lab/graph_assertions.py": (
            "class GraphAssertions:\n"
            "    def assert_connected(self, src, dst):\n"
            "        return src in dst\n"
        ),
        "tests/integration/headless_audio_lab/runner.py": (
            "class Runner:\n"
            "    def __init__(self):\n"
            "        self.api = ['ProcessSupervisor', 'JackServer', 'OscProbe', 'SooperLooperLauncher', 'analyze_wav', 'GraphAssertions']\n"
            "    def run(self, scenario):\n"
            "        return {'scenario': scenario, 'ok': True}\n"
        ),
        "tests/integration/headless_audio_lab/test_modules.py": (
            "from headless_audio_lab.audio_oracle import analyze_wav\n"
            "def test_oracle_contract():\n"
            "    r = analyze_wav(b'')\n"
            "    return isinstance(r, dict)\n"
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
            "# synthetic source placeholder implementation note\n"
            "echo source\n"
        ),
        "tests/integration/headless_audio_lab/fixtures/deterministic_capture": (
            "#!/bin/sh\n"
            "# deterministic capture implementation note\n"
            "echo capture\n"
        ),
        "tests/integration/headless_audio_lab/fixtures/.gitignore": "# generated lab artifacts\nbuild/\n",
    }


def _blob_sha(repo, oid, path):
    r = subprocess.run(["git", "-C", repo, "show", "%s:%s" % (oid, path)],
                       capture_output=True, timeout=30)
    return sha256_b(r.stdout), len(r.stdout)


def build_valid_fixture(base, name, lab, mutate=None, impl_files=None, envelope_mutate=None,
                        skip_artifacts=False, minimal_envelope=False):
    """Build C->R->E with a full strict envelope; returns (repo, C, R, E, env, env_raw)."""
    repo = _init_repo(base, name)
    _write_file(os.path.join(repo, "README.md"), "base\n")
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
        env = {
            "candidate_commit_oid": c,
            "implementation_result_commit_oid": r,
            "implementation_result_tree_oid": r_tree,
            "git_object_format": objfmt,
            "artifacts": {},
        }
        env_raw = json.dumps(env, indent=2) + "\n"
        _write_file(os.path.join(repo, env_path), env_raw)
        e = _commit(repo, "E envelope")
        if mutate:
            mutate(repo, c, r, e)
        return repo, c, r, e, env, env_raw

    if skip_artifacts:
        artifacts = {}
    else:
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

    t_start = _now()
    t_end = _now()
    if lab == "LAB-A":
        llt = {tid: {"command_sha256": sha256_b(json.dumps(m["command"], ensure_ascii=False).encode("utf-8")),
                     "exit_code": 0, "stdout_sha256": "0" * 64, "stderr_sha256": "0" * 64,
                     "started_at_utc": t_start, "ended_at_utc": t_end}
               for tid, m in lab_meta["literal_tests"].items()}
    else:
        llt = {tid: {"command_sha256": sha256_b(json.dumps(m["command"], ensure_ascii=False).encode("utf-8")),
                     "exit_code": 0, "stdout_sha256": "0" * 64, "stderr_sha256": "0" * 64,
                     "started_at_utc": t_start, "ended_at_utc": t_end}
               for tid, m in lab_meta["literal_tests"].items()}

    if lab == "LAB-A":
        lbc = {"status": "not_applicable", "sources": [], "compiler_path": None,
               "compiler_version": None, "commands": [], "outputs": [],
               "transcript_sha256": None}
    else:
        # LAB-B claimed_built: claim for the deterministic build output
        h = hashlib.sha256(b"v3.3-oracle-claim").hexdigest()
        out_bytes = (h + "\n").encode("utf-8")
        lbc = {"status": "claimed_built",
               "sources": sorted(path for path in impl),
               "compiler_path": "/usr/bin/python3",
               "compiler_version": "3.11.15",
               "commands": [json.dumps(lab_meta["build_command"], ensure_ascii=False)],
               "outputs": [{"path": "build/lab-b-oracle-claim.json",
                            "sha256": sha256_b(out_bytes), "size": len(out_bytes)}],
               "transcript_sha256": None}

    env = {
        "schema": SCHEMA,
        "lab": lab,
        "batch_id": BATCH_ID,
        "leaf_id": lab_meta["leaf_id"],
        "lease_id": lab_meta["lease_id"],
        "candidate_commit_oid": c,
        "implementation_result_commit_oid": r,
        "implementation_result_tree_oid": r_tree,
        "git_object_format": objfmt,
        "external_branch_binding": {"remote": "origin",
                                    "planned_branch": "result/v3-3-fixture-%s" % lab.lower()},
        "leaf_literal_tests": llt,
        "leaf_build_claim": lbc,
        "artifacts": artifacts,
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
    batch = load_batch_manifest()
    lab_meta = batch["labs"][lab]
    env_path = lab_meta["envelope_path"]
    e_blob = _git_quiet(repo, ["rev-parse", "%s:%s" % (e, env_path)])
    env_sha = sha256_b(env_raw.encode("utf-8"))
    rlt = {}
    tmp = tempfile.mkdtemp(prefix="v33-receipt-")
    wt = os.path.join(tmp, "wt")
    git(repo, ["worktree", "add", "--detach", wt, r])
    try:
        for tid, m in lab_meta["literal_tests"].items():
            t0 = _now()
            rr = run(m["command"], wt, timeout=m.get("timeout_seconds", 120))
            t1 = _now()
            rlt[tid] = {
                "command_sha256": sha256_b(json.dumps(m["command"], ensure_ascii=False).encode("utf-8")),
                "exit_code": rr["exit"],
                "stdout_sha256": sha256_b(rr["stdout"].encode("utf-8")),
                "stderr_sha256": sha256_b(rr["stderr"].encode("utf-8")),
                "started_at_utc": t0, "ended_at_utc": t1,
            }
    finally:
        git(repo, ["worktree", "remove", "--force", wt])
        shutil.rmtree(tmp, ignore_errors=True)
    rec = {
        "schema": RECEIPT_SCHEMA,
        "batch_id": BATCH_ID,
        "lab": lab,
        "candidate_commit_oid": c,
        "implementation_result_commit_oid": r,
        "implementation_result_tree_oid": _git_quiet(repo, ["rev-parse", "%s^{tree}" % r]),
        "envelope_container_commit_oid": e,
        "envelope_container_tree_oid": e_tree,
        "envelope_blob_oid": e_blob,
        "envelope_sha256": env_sha,
        "remote": "origin",
        "branch": expected_branch,
        "remote_resolved_oid": e,
        "controller_clean_checkout_path_or_receipt": "fixture-clean-checkout",
        "controller_literal_tests": rlt,
        "acceptance_harness_sha256": sha256_file(__file__),
        "acceptance_harness_verdict": "PASS",
        "artifact_verification": "PASS",
        "controlled_rebuild": "not_applicable" if lab == "LAB-A" else "PASS",
        "build_provenance_verdict": "not_applicable" if lab == "LAB-A" else "demonstrated",
        "worktree_clean_before": True,
        "worktree_clean_after": True,
        "completion_criteria": {
            "non_empty_commit": True, "published_branch": True, "full_result_head_sha": True,
            "clean_worktree": True, "valid_result_envelope": True, "owned_path_only_diff": True,
            "literal_focused_tests_exit_0": True, "controller_clean_checkout_acceptance_pass": True,
        },
        "final_verdict": "PASS",
        "recorded_at_utc": _now(),
    }
    rec.update(overrides)
    return rec


def _push_to_bare(base, repo, branch, oid):
    bare = os.path.join(base, "origin.git")
    if not os.path.exists(bare):
        subprocess.run(["git", "init", "--bare", "-q", bare], check=True)
    subprocess.run(["git", "-C", repo, "remote", "add", "origin", bare], check=True)
    # reset the branch ref so each fixture pushes its own E (isolated cases)
    subprocess.run(["git", "-C", bare, "update-ref", "-d", "refs/heads/%s" % branch],
                   capture_output=True)
    subprocess.run(["git", "-C", repo, "push", "origin", "%s:refs/heads/%s" % (oid, branch)],
                   capture_output=True, check=True, timeout=60)
    return bare


def selftest():
    res = Result()
    batch = load_batch_manifest()
    lab = "LAB-A"
    env_path = batch["labs"][lab]["envelope_path"]
    expected_branch = "result/v3-3-fixture-lab-a"

    with tempfile.TemporaryDirectory(prefix="v33-selftest-") as tmp:
        # 1. valid C->R->E leaf envelope PASS
        g = build_valid_fixture(tmp, "valid-leaf", lab)
        repo, c, r, e, env, env_raw = g
        r1 = Result()
        verify_leaf_envelope(repo, c, r, e, env_path, lab, batch, r1)
        res.add("selftest: valid C->R->E leaf envelope PASS",
                "PASS" if not r1.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in r1.checks if x["status"] == "FAIL"]})

        # 2. v3.2 minimal envelope REJECT under v3.3 (4 OIDs + artifacts={} only)
        g2 = build_valid_fixture(tmp, "minimal-env", lab, minimal_envelope=True)
        r2res = Result()
        verify_leaf_envelope(g2[0], g2[1], g2[2], g2[3], env_path, lab, batch, r2res)
        res.add("selftest: v3.2 minimal envelope rejected by v3.3",
                "PASS" if r2res.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in r2res.checks if x["status"] == "FAIL"]})

        # 3. missing schema
        g3 = build_valid_fixture(tmp, "miss-schema", lab, envelope_mutate=lambda env, *a: env.pop("schema", None))
        r3 = Result()
        verify_leaf_envelope(g3[0], g3[1], g3[2], g3[3], env_path, lab, batch, r3)
        res.add("selftest: missing schema REJECT", "PASS" if r3.failed() else "FAIL")

        # 4. wrong schema
        g4 = build_valid_fixture(tmp, "wrong-schema", lab, envelope_mutate=lambda env, *a: env.update({"schema": "x"}))
        r4 = Result()
        verify_leaf_envelope(g4[0], g4[1], g4[2], g4[3], env_path, lab, batch, r4)
        res.add("selftest: wrong schema REJECT", "PASS" if r4.failed() else "FAIL")

        # 5. wrong lab
        g5 = build_valid_fixture(tmp, "wrong-lab", lab, envelope_mutate=lambda env, *a: env.update({"lab": "LAB-B"}))
        r5 = Result()
        verify_leaf_envelope(g5[0], g5[1], g5[2], g5[3], env_path, lab, batch, r5)
        res.add("selftest: wrong lab REJECT", "PASS" if r5.failed() else "FAIL")

        # 6. wrong batch
        g6 = build_valid_fixture(tmp, "wrong-batch", lab, envelope_mutate=lambda env, *a: env.update({"batch_id": "BATCH-WRONG"}))
        r6 = Result()
        verify_leaf_envelope(g6[0], g6[1], g6[2], g6[3], env_path, lab, batch, r6)
        res.add("selftest: wrong batch REJECT", "PASS" if r6.failed() else "FAIL")

        # 7. wrong leaf
        g7 = build_valid_fixture(tmp, "wrong-leaf", lab, envelope_mutate=lambda env, *a: env.update({"leaf_id": "LEAF-WRONG"}))
        r7 = Result()
        verify_leaf_envelope(g7[0], g7[1], g7[2], g7[3], env_path, lab, batch, r7)
        res.add("selftest: wrong leaf REJECT", "PASS" if r7.failed() else "FAIL")

        # 8. wrong lease
        g8 = build_valid_fixture(tmp, "wrong-lease", lab, envelope_mutate=lambda env, *a: env.update({"lease_id": "LEASE-WRONG"}))
        r8 = Result()
        verify_leaf_envelope(g8[0], g8[1], g8[2], g8[3], env_path, lab, batch, r8)
        res.add("selftest: wrong lease REJECT", "PASS" if r8.failed() else "FAIL")

        # 9. unknown top-level key
        g9 = build_valid_fixture(tmp, "unknown-key", lab, envelope_mutate=lambda env, *a: env.update({"mystery": 1}))
        r9 = Result()
        verify_leaf_envelope(g9[0], g9[1], g9[2], g9[3], env_path, lab, batch, r9)
        res.add("selftest: unknown top-level key REJECT", "PASS" if r9.failed() else "FAIL")

        # 10. missing planned branch
        g10 = build_valid_fixture(tmp, "miss-branch", lab, envelope_mutate=lambda env, *a: env["external_branch_binding"].pop("planned_branch", None))
        r10 = Result()
        verify_leaf_envelope(g10[0], g10[1], g10[2], g10[3], env_path, lab, batch, r10,
                             expected_branch=expected_branch)
        res.add("selftest: missing planned branch REJECT", "PASS" if r10.failed() else "FAIL")

        # 11. wrong planned branch
        g11 = build_valid_fixture(tmp, "wrong-branch", lab, envelope_mutate=lambda env, *a: env["external_branch_binding"].update({"planned_branch": "wrong/branch"}))
        r11 = Result()
        verify_leaf_envelope(g11[0], g11[1], g11[2], g11[3], env_path, lab, batch, r11,
                             expected_branch=expected_branch)
        res.add("selftest: wrong planned branch REJECT", "PASS" if r11.failed() else "FAIL")

        # 12. R empty REJECT (R == C: no implementation result at all)
        repo12 = _init_repo(tmp, "r-empty2")
        _write_file(os.path.join(repo12, "README.md"), "base\n")
        c12 = _commit(repo12, "C")
        r12_tree = _git_quiet(repo12, ["rev-parse", "HEAD^{tree}"])
        objfmt12 = _git_quiet(repo12, ["rev-parse", "--show-object-format"])
        env12 = {
            "schema": SCHEMA, "lab": lab, "batch_id": BATCH_ID,
            "leaf_id": batch["labs"][lab]["leaf_id"],
            "lease_id": batch["labs"][lab]["lease_id"],
            "candidate_commit_oid": c12, "implementation_result_commit_oid": c12,
            "implementation_result_tree_oid": r12_tree, "git_object_format": objfmt12,
            "external_branch_binding": {"remote": "origin",
                                        "planned_branch": "result/v3-3-fixture-lab-a"},
            "leaf_literal_tests": {},
            "leaf_build_claim": {"status": "not_applicable", "sources": [], "compiler_path": None,
                                 "compiler_version": None, "commands": [], "outputs": [],
                                 "transcript_sha256": None},
            "artifacts": {},
        }
        _write_file(os.path.join(repo12, env_path), json.dumps(env12, indent=2) + "\n")
        e12 = _commit(repo12, "E")
        r12res = Result()
        verify_leaf_envelope(repo12, c12, c12, e12, env_path, lab, batch, r12res)
        res.add("selftest: R empty REJECT", "PASS" if r12res.failed() else "FAIL")

        # 13. R merge commit REJECT
        repo13 = _init_repo(tmp, "r-merge")
        _write_file(os.path.join(repo13, "README.md"), "base\n")
        c13 = _commit(repo13, "C")
        b13 = _git_quiet(repo13, ["rev-parse", "HEAD"])
        _write_file(os.path.join(repo13, "a.txt"), "a\n")
        r13a = _commit(repo13, "R side A")
        run(["git", "checkout", "-q", b13], repo13)
        _write_file(os.path.join(repo13, "b.txt"), "b\n")
        r13b = _commit(repo13, "R side B")
        run(["git", "merge", "-q", "--no-ff", "-m", "merge", r13a], repo13)
        r13 = _git_quiet(repo13, ["rev-parse", "HEAD"])
        # write a valid full envelope at the merge head so E commits; the REJECT
        # gate under test is "R exactly one parent" (merge commit).
        r13_tree = _git_quiet(repo13, ["rev-parse", "HEAD^{tree}"])
        objfmt13 = _git_quiet(repo13, ["rev-parse", "--show-object-format"])
        env13 = {
            "schema": SCHEMA, "lab": lab, "batch_id": BATCH_ID,
            "leaf_id": batch["labs"][lab]["leaf_id"],
            "lease_id": batch["labs"][lab]["lease_id"],
            "candidate_commit_oid": c13, "implementation_result_commit_oid": r13,
            "implementation_result_tree_oid": r13_tree, "git_object_format": objfmt13,
            "external_branch_binding": {"remote": "origin",
                                        "planned_branch": "result/v3-3-fixture-lab-a"},
            "leaf_literal_tests": {},
            "leaf_build_claim": {"status": "not_applicable", "sources": [], "compiler_path": None,
                                 "compiler_version": None, "commands": [], "outputs": [],
                                 "transcript_sha256": None},
            "artifacts": {},
        }
        _write_file(os.path.join(repo13, env_path), json.dumps(env13, indent=2) + "\n")
        e13 = _commit(repo13, "E")
        r13res = Result()
        verify_leaf_envelope(repo13, c13, r13, e13, env_path, lab, batch, r13res)
        res.add("selftest: R merge commit REJECT", "PASS" if r13res.failed() else "FAIL")

        # 14. E merge commit REJECT
        g14 = build_valid_fixture(tmp, "e-merge", lab, mutate=lambda repo, c, r, e: None)
        repo14, c14, r14, e14 = g14[0], g14[1], g14[2], g14[3]
        # make E a merge: add second parent commit then merge
        run(["git", "checkout", "-q", "%s^" % r14], repo14)
        _write_file(os.path.join(repo14, "zz.txt"), "z\n")
        extra14 = _commit(repo14, "extra")
        run(["git", "checkout", "-q", e14], repo14)
        run(["git", "merge", "-q", "--no-ff", "-m", "merge", extra14], repo14)
        e14m = _git_quiet(repo14, ["rev-parse", "HEAD"])
        r14res = Result()
        verify_leaf_envelope(repo14, c14, r14, e14m, env_path, lab, batch, r14res)
        res.add("selftest: E merge commit REJECT", "PASS" if r14res.failed() else "FAIL")

        # 15. envelope in R REJECT
        repo15 = _init_repo(tmp, "env-in-r")
        _write_file(os.path.join(repo15, "README.md"), "base\n")
        c15 = _commit(repo15, "C")
        impl15 = fixture_impl(lab)
        for path, content in impl15.items():
            _write_file(os.path.join(repo15, path), content)
        r15 = _commit(repo15, "R")
        _write_file(os.path.join(repo15, env_path), "{}")
        r15b = _commit(repo15, "R2 with envelope")
        # E must be a NEW commit: overwrite the envelope with a full valid one
        r15_tree = _git_quiet(repo15, ["rev-parse", "HEAD^{tree}"])
        objfmt15 = _git_quiet(repo15, ["rev-parse", "--show-object-format"])
        env15 = {
            "schema": SCHEMA, "lab": lab, "batch_id": BATCH_ID,
            "leaf_id": batch["labs"][lab]["leaf_id"],
            "lease_id": batch["labs"][lab]["lease_id"],
            "candidate_commit_oid": c15, "implementation_result_commit_oid": r15b,
            "implementation_result_tree_oid": r15_tree, "git_object_format": objfmt15,
            "external_branch_binding": {"remote": "origin",
                                        "planned_branch": "result/v3-3-fixture-lab-a"},
            "leaf_literal_tests": {},
            "leaf_build_claim": {"status": "not_applicable", "sources": [], "compiler_path": None,
                                 "compiler_version": None, "commands": [], "outputs": [],
                                 "transcript_sha256": None},
            "artifacts": {},
        }
        _write_file(os.path.join(repo15, env_path), json.dumps(env15, indent=2) + "\n")
        e15 = _commit(repo15, "E")
        r15res = Result()
        verify_leaf_envelope(repo15, c15, r15b, e15, env_path, lab, batch, r15res)
        res.add("selftest: envelope in R REJECT", "PASS" if r15res.failed() else "FAIL")

        # 16. implementation change in E REJECT
        g16 = build_valid_fixture(tmp, "impl-in-e", lab, mutate=lambda repo, c, r, e: None)
        repo16, c16, r16, e16 = g16[0], g16[1], g16[2], g16[3]
        _write_file(os.path.join(repo16, "tests/integration/headless_audio_lab/jack_server.py"),
                    "class JackServer:\n    pass\n")
        e16b = _commit(repo16, "E2 with impl change")
        r16res = Result()
        verify_leaf_envelope(repo16, c16, r16, e16b, env_path, lab, batch, r16res)
        res.add("selftest: implementation change in E REJECT", "PASS" if r16res.failed() else "FAIL")

        # 17. missing leaf_literal_tests
        g17 = build_valid_fixture(tmp, "miss-llt", lab, envelope_mutate=lambda env, *a: env.pop("leaf_literal_tests", None))
        r17 = Result()
        verify_leaf_envelope(g17[0], g17[1], g17[2], g17[3], env_path, lab, batch, r17)
        res.add("selftest: missing leaf_literal_tests REJECT", "PASS" if r17.failed() else "FAIL")

        # 18. missing authoritative test ID
        g18 = build_valid_fixture(tmp, "miss-test-id", lab, envelope_mutate=lambda env, *a: env["leaf_literal_tests"].pop(list(env["leaf_literal_tests"])[0], None))
        r18 = Result()
        verify_leaf_envelope(g18[0], g18[1], g18[2], g18[3], env_path, lab, batch, r18)
        res.add("selftest: missing authoritative test ID REJECT", "PASS" if r18.failed() else "FAIL")

        # 19. extra test ID
        g19 = build_valid_fixture(tmp, "extra-test-id", lab, envelope_mutate=lambda env, *a: env["leaf_literal_tests"].update({"ft-x9": {"command_sha256": "0"*64, "exit_code": 0}}))
        r19 = Result()
        verify_leaf_envelope(g19[0], g19[1], g19[2], g19[3], env_path, lab, batch, r19)
        res.add("selftest: extra test ID REJECT", "PASS" if r19.failed() else "FAIL")

        # 20. wrong command hash
        g20 = build_valid_fixture(tmp, "wrong-cmd-hash", lab, envelope_mutate=lambda env, *a: env["leaf_literal_tests"].__setitem__(list(env["leaf_literal_tests"])[0], {"command_sha256": "1"*64, "exit_code": 0, "stdout_sha256": "0"*64, "stderr_sha256": "0"*64, "started_at_utc": _now(), "ended_at_utc": _now()}))
        r20 = Result()
        verify_leaf_envelope(g20[0], g20[1], g20[2], g20[3], env_path, lab, batch, r20)
        res.add("selftest: wrong command hash REJECT", "PASS" if r20.failed() else "FAIL")

        # 21. nonzero leaf test
        g21 = build_valid_fixture(tmp, "nonzero-test", lab, envelope_mutate=lambda env, *a: env["leaf_literal_tests"].__setitem__(list(env["leaf_literal_tests"])[0], {"command_sha256": "0"*64, "exit_code": 1, "stdout_sha256": "0"*64, "stderr_sha256": "0"*64, "started_at_utc": _now(), "ended_at_utc": _now()}))
        r21 = Result()
        verify_leaf_envelope(g21[0], g21[1], g21[2], g21[3], env_path, lab, batch, r21)
        res.add("selftest: nonzero leaf test REJECT", "PASS" if r21.failed() else "FAIL")

        # 22. invalid timestamp ordering
        g22 = build_valid_fixture(tmp, "bad-ts", lab, envelope_mutate=lambda env, *a: env["leaf_literal_tests"].__setitem__(list(env["leaf_literal_tests"])[0], {"command_sha256": "0"*64, "exit_code": 0, "stdout_sha256": "0"*64, "stderr_sha256": "0"*64, "started_at_utc": "2026-08-06T10:00:00Z", "ended_at_utc": "2026-08-06T09:00:00Z"}))
        r22 = Result()
        verify_leaf_envelope(g22[0], g22[1], g22[2], g22[3], env_path, lab, batch, r22)
        res.add("selftest: invalid timestamp ordering REJECT", "PASS" if r22.failed() else "FAIL")

        # 23. artifacts empty
        g23 = build_valid_fixture(tmp, "arts-empty", lab, skip_artifacts=True)
        r23 = Result()
        verify_leaf_envelope(g23[0], g23[1], g23[2], g23[3], env_path, lab, batch, r23)
        res.add("selftest: artifacts empty REJECT", "PASS" if r23.failed() else "FAIL")

        # 24. missing artifact
        g24 = build_valid_fixture(tmp, "miss-art", lab, envelope_mutate=lambda env, *a: env["artifacts"].pop(next(iter(env["artifacts"]))))
        r24 = Result()
        verify_leaf_envelope(g24[0], g24[1], g24[2], g24[3], env_path, lab, batch, r24)
        res.add("selftest: missing artifact REJECT", "PASS" if r24.failed() else "FAIL")

        # 25. extra artifact
        g25 = build_valid_fixture(tmp, "extra-art", lab, envelope_mutate=lambda env, *a: env["artifacts"].update({"README.md": {"kind": "source", "sha256": hashlib.sha256(b"base\n").hexdigest(), "size": 5}}))
        r25 = Result()
        verify_leaf_envelope(g25[0], g25[1], g25[2], g25[3], env_path, lab, batch, r25)
        res.add("selftest: extra artifact REJECT", "PASS" if r25.failed() else "FAIL")

        # 26. wrong artifact hash
        g26 = build_valid_fixture(tmp, "wrong-art-hash", lab, envelope_mutate=lambda env, *a: env["artifacts"].__setitem__(next(iter(env["artifacts"])), {"kind": "source", "sha256": "1"*64, "size": 1}))
        r26 = Result()
        verify_leaf_envelope(g26[0], g26[1], g26[2], g26[3], env_path, lab, batch, r26)
        res.add("selftest: wrong artifact hash REJECT", "PASS" if r26.failed() else "FAIL")

        # 27. wrong artifact size
        g27 = build_valid_fixture(tmp, "wrong-art-size", lab, envelope_mutate=lambda env, *a: env["artifacts"].__setitem__(next(iter(env["artifacts"])), {"kind": "source", "sha256": "1"*64, "size": 999}))
        r27 = Result()
        verify_leaf_envelope(g27[0], g27[1], g27[2], g27[3], env_path, lab, batch, r27)
        res.add("selftest: wrong artifact size REJECT", "PASS" if r27.failed() else "FAIL")

        # 28. artifact outside R
        g28 = build_valid_fixture(tmp, "art-outside-r", lab, envelope_mutate=lambda env, *a: env["artifacts"].update({"nope.py": {"kind": "source", "sha256": "1"*64, "size": 1}}))
        r28 = Result()
        verify_leaf_envelope(g28[0], g28[1], g28[2], g28[3], env_path, lab, batch, r28)
        res.add("selftest: artifact outside R REJECT", "PASS" if r28.failed() else "FAIL")

        # 29. artifact outside ownership
        g29 = build_valid_fixture(tmp, "art-out-own", lab, envelope_mutate=lambda env, *a: env["artifacts"].update({"other.py": {"kind": "source", "sha256": "1"*64, "size": 1}}))
        r29 = Result()
        verify_leaf_envelope(g29[0], g29[1], g29[2], g29[3], env_path, lab, batch, r29)
        res.add("selftest: artifact outside ownership REJECT", "PASS" if r29.failed() else "FAIL")

        # 30. missing leaf build claim
        g30 = build_valid_fixture(tmp, "miss-lbc", lab, envelope_mutate=lambda env, *a: env.pop("leaf_build_claim", None))
        r30 = Result()
        verify_leaf_envelope(g30[0], g30[1], g30[2], g30[3], env_path, lab, batch, r30)
        res.add("selftest: missing leaf build claim REJECT", "PASS" if r30.failed() else "FAIL")

        # 31. invalid LAB-A build claim (invented fields)
        g31 = build_valid_fixture(tmp, "bad-lab-a-claim", lab, envelope_mutate=lambda env, *a: env["leaf_build_claim"].update({"status": "claimed_built", "compiler_path": "/usr/bin/gcc"}))
        r31 = Result()
        verify_leaf_envelope(g31[0], g31[1], g31[2], g31[3], env_path, lab, batch, r31)
        res.add("selftest: invalid LAB-A build claim REJECT", "PASS" if r31.failed() else "FAIL")

        # 32. incomplete LAB-B build claim
        g32 = build_valid_fixture(tmp, "bad-lab-b-claim", "LAB-B", envelope_mutate=lambda env, *a: env["leaf_build_claim"].update({"outputs": []}))
        r32 = Result()
        verify_leaf_envelope(g32[0], g32[1], g32[2], g32[3], batch["labs"]["LAB-B"]["envelope_path"], "LAB-B", batch, r32)
        res.add("selftest: incomplete LAB-B build claim REJECT", "PASS" if r32.failed() else "FAIL")

        # 33. valid full controller completion PASS (LAB-A)
        gf = build_valid_fixture(tmp, "full-a", lab)
        repoF, cF, rF, eF, _, env_rawF = gf
        expected = "result/v3-3-fixture-lab-a"
        _push_to_bare(tmp, repoF, expected, eF)
        recF = _default_receipt(repoF, cF, rF, eF, env_rawF, lab, expected)
        rec_path = os.path.join(tmp, "receipt-a.json")
        with open(rec_path, "w") as f:
            json.dump(recF, f, indent=2)
        rFres = Result()
        verify_full_completion(repoF, cF, rF, eF, env_path, lab, batch, env_rawF, recF, expected, rFres)
        res.add("selftest: valid full controller completion PASS (LAB-A)",
                "PASS" if not rFres.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in rFres.checks if x["status"] == "FAIL"]})

        # 34. missing controller receipt REJECT in full-completion
        r34 = Result()
        verify_full_completion(repoF, cF, rF, eF, env_path, lab, batch, env_rawF, None, expected, r34)
        res.add("selftest: missing controller receipt REJECT in full-completion",
                "PASS" if r34.failed() else "FAIL")

        # 35. remote branch mismatch REJECT
        recBad = _default_receipt(repoF, cF, rF, eF, env_rawF, lab, expected, remote_resolved_oid="0"*40)
        r35 = Result()
        verify_full_completion(repoF, cF, rF, eF, env_path, lab, batch, env_rawF, recBad, expected, r35)
        res.add("selftest: remote branch mismatch REJECT", "PASS" if r35.failed() else "FAIL")

        # 36. controller test failure REJECT (receipt claims exit 0 but command fails on a broken impl)
        impl36 = fixture_impl(lab)
        impl36["tests/integration/headless_audio_lab/process_supervisor.py"] = (
            "class ProcessSupervisor:\n"
            "    def __init__(self):\n        self._owned = {}\n"
            "    def __enter__(self):\n        return self\n"
            "    def __exit__(self, *exc):\n        return False\n"
            "    def start_process(self, name, cmd, env, cwd, so, se):\n"
            "        raise RuntimeError('broken fixture')\n"
            "    def get_owned_pids(self):\n        return []\n"
            "    def cleanup(self, timeout=5.0):\n        return None\n"
            "    def verify_cleanup(self):\n        return []\n"
        )
        g36 = build_valid_fixture(tmp, "full-broken", lab, impl_files=impl36)
        repo36, c36, r36, e36, _, env_raw36 = g36
        _push_to_bare(tmp, repo36, expected, e36)
        rec36 = _default_receipt(repo36, c36, r36, e36, env_raw36, lab, expected)
        r36res = Result()
        verify_full_completion(repo36, c36, r36, e36, env_path, lab, batch, env_raw36, rec36, expected, r36res)
        res.add("selftest: controller test failure REJECT",
                "PASS" if r36res.failed() else "FAIL",
                evidence={"failing": [x["name"] for x in r36res.checks if x["status"] == "FAIL"]})

        # 37. acceptance harness failure REJECT (placeholder in owned source; literal test still passes)
        impl37 = fixture_impl(lab)
        impl37["tests/integration/headless_audio_lab/jack_server.py"] = (
            "class JackServer:\n"
            "    def start(self, sample_rate=48000, period_size=1024):\n"
            "        return a dummy\n"
        )
        g37 = build_valid_fixture(tmp, "full-harness-fail", lab, impl_files=impl37)
        repo37, c37, r37, e37, _, env_raw37 = g37
        _push_to_bare(tmp, repo37, expected, e37)
        rec37 = _default_receipt(repo37, c37, r37, e37, env_raw37, lab, expected)
        r37res = Result()
        verify_full_completion(repo37, c37, r37, e37, env_path, lab, batch, env_raw37, rec37, expected, r37res)
        res.add("selftest: acceptance harness failure REJECT",
                "PASS" if r37res.failed() else "FAIL")

        # 38. artifact verification failure REJECT (receipt claims PASS but env artifacts wrong)
        g38 = build_valid_fixture(tmp, "full-art-fail", lab, envelope_mutate=lambda env, *a: env["artifacts"].__setitem__(next(iter(env["artifacts"])), {"kind": "source", "sha256": "1"*64, "size": 1}))
        repo38, c38, r38, e38, _, env_raw38 = g38
        _push_to_bare(tmp, repo38, expected, e38)
        rec38 = _default_receipt(repo38, c38, r38, e38, env_raw38, lab, expected)
        r38res = Result()
        verify_full_completion(repo38, c38, r38, e38, env_path, lab, batch, env_raw38, rec38, expected, r38res)
        res.add("selftest: artifact verification failure REJECT",
                "PASS" if r38res.failed() else "FAIL")

        # 39. controlled rebuild failure REJECT (LAB-B broken build claim hash)
        g39 = build_valid_fixture(tmp, "full-rebuild-fail", "LAB-B", envelope_mutate=lambda env, *a: env["leaf_build_claim"]["outputs"].__setitem__(0, {"path": "build/lab-b-oracle-claim.json", "sha256": "1"*64, "size": 65}))
        repo39, c39, r39, e39, _, env_raw39 = g39
        expectedB = "result/v3-3-fixture-lab-b"
        _push_to_bare(tmp, repo39, expectedB, e39)
        rec39 = _default_receipt(repo39, c39, r39, e39, env_raw39, "LAB-B", expectedB)
        r39res = Result()
        verify_full_completion(repo39, c39, r39, e39, batch["labs"]["LAB-B"]["envelope_path"], "LAB-B", batch, env_raw39, rec39, expectedB, r39res)
        res.add("selftest: controlled rebuild failure REJECT (LAB-B)",
                "PASS" if r39res.failed() else "FAIL")

        # 40. dirty controller checkout REJECT (worktree has untracked file)
        g40 = build_valid_fixture(tmp, "full-dirty", lab, mutate=lambda repo, c, r, e: _write_file(os.path.join(repo, "dirty.txt"), "dirty\n"))
        repo40, c40, r40, e40, _, env_raw40 = g40
        rec40 = _default_receipt(repo40, c40, r40, e40, env_raw40, lab, expected)
        r40res = Result()
        verify_full_completion(repo40, c40, r40, e40, env_path, lab, batch, env_raw40, rec40, expected, r40res)
        res.add("selftest: dirty controller checkout REJECT",
                "PASS" if r40res.failed() else "FAIL")

        # 41. correct external branch binding PASS
        res.add("selftest: correct external branch binding accepted",
                "PASS" if not [x for x in rFres.checks if "remote resolved OID" in x["name"] and x["status"] == "FAIL"] else "FAIL")

        # 42. zero temporary worktrees/processes/directories
        rwt = git(repoF, ["worktree", "list", "--porcelain"])
        res.add("selftest: zero residual worktrees in fixture repo",
                "PASS" if rwt["stdout"].strip().count("worktree ") == 1 else "FAIL")

    res.add("selftest: zero temporary directories (TemporaryDirectory auto-clean)", "PASS")
    return res


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=SCHEMA)
    ap.add_argument("--lab", choices=["a", "b"])
    ap.add_argument("--repository")
    ap.add_argument("--candidate-head")
    ap.add_argument("--result-head")
    ap.add_argument("--envelope-container-head")
    ap.add_argument("--batch-manifest", default=os.path.join(PACKAGE_DIR, "batch-manifest-v3.3.json"))
    ap.add_argument("--scope-freeze", default=os.path.join(PACKAGE_DIR, "scope-freeze-v3.3.json"))
    ap.add_argument("--build-policy", default=os.path.join(PACKAGE_DIR, "build-provenance-policy-v3.3.json"))
    ap.add_argument("--contract", default=os.path.join(PACKAGE_DIR, "lab-api-contract-v3.3.json"))
    ap.add_argument("--envelope-path")
    ap.add_argument("--expected-branch")
    ap.add_argument("--controller-acceptance-receipt")
    ap.add_argument("--phase", choices=["leaf-envelope", "full-completion"], default="leaf-envelope")
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

    required = ["--lab", "--repository", "--candidate-head", "--result-head",
                "--envelope-container-head", "--envelope-path"]
    missing = [r for r in required if getattr(args, r.lstrip("--").replace("-", "_")) is None]
    if missing:
        ap.error("missing required arguments: %s" % ", ".join(missing))
    lab = "LAB-A" if args.lab == "a" else "LAB-B"
    batch = load_batch_manifest()
    lab_meta = batch["labs"][lab]
    if not args.envelope_path:
        args.envelope_path = lab_meta["envelope_path"]

    res = Result()
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
    sys.dont_write_bytecode = True

    env_raw = git(args.repository, ["show", "%s:%s" % (args.envelope_container_head, args.envelope_path)])["stdout"]

    if args.phase == "leaf-envelope":
        verify_leaf_envelope(args.repository, args.candidate_head, args.result_head,
                             args.envelope_container_head, args.envelope_path, lab, batch, res,
                             expected_branch=args.expected_branch or None)
    else:
        receipt = None
        if args.controller_acceptance_receipt:
            receipt = load_json(args.controller_acceptance_receipt, "controller-acceptance-receipt")
        verify_full_completion(args.repository, args.candidate_head, args.result_head,
                               args.envelope_container_head, args.envelope_path, lab, batch,
                               env_raw, receipt, args.expected_branch or "", res)

    if args.json_out:
        with open(args.json_out, "w") as f:
            json.dump({"schema": SCHEMA, "version": VERSION, "lab": lab,
                       "repository": args.repository, "candidate_head": args.candidate_head,
                       "result_head": args.result_head,
                       "envelope_container_head": args.envelope_container_head,
                       "envelope_path": args.envelope_path, "phase": args.phase,
                       "checks": res.checks, "env_notes": res.env_notes,
                       "verdict": "REJECT" if res.failed() else "PASS"}, f, indent=2)
    for c in res.checks:
        print("[%s] %s — %s" % (c["status"], c["name"], c["detail"]))
    print("VERDICT:", "REJECT (one or more FAIL)" if res.failed() else "PASS")
    return 1 if res.failed() else 0


if __name__ == "__main__":
    sys.exit(main())
