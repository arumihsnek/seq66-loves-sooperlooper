#!/usr/bin/env python3
"""acceptance-verifier-v3.2.py — Controller-owned acceptance harness for
headless-lab v3.2 dispatch (LAB-A / LAB-B).

OWNERSHIP: this script belongs to the controller/control plane. It is NOT an
owned path of LAB-A or LAB-B and may not be modified by a leaf. Its SHA-256 is
frozen in acceptance-manifest-v3.2.json.

TWO-COMMIT ENVELOPE MODEL (replaces the v3.1 same-commit model):
  C = candidate head (parent of R)
  R = implementation result head; parent(R) == C; contains the owned
      implementation, tests and tracked provenance; NO envelope.
  E = envelope container head; parent(E) == R; contains EXACTLY the frozen
      envelope of that leaf and NO implementation change.
  The remote leaf branch points to E. The ONLY integration-eligible commit is R.
  E is evidence and is never integrated as implementation.

The envelope is non-self-referential: it declares candidate_commit_oid,
implementation_result_commit_oid and implementation_result_tree_oid (Git object
IDs via git_object_format), never the E commit/tree/blob OID, its own SHA-256
or the resolved remote branch SHA. Those are recorded externally by the
controller after push.

Functional gates (carried from v3.1 corrections):
  LAB-A owned/foreign process separation, placeholder rejection, bounded exact
  cleanup; LAB-B silent WAV FAIL, malformed WAV rejection, frame-count contract,
  nontrivial tests, byte-exact .gitignore, no tracked ELF.

Exit codes: 0 = ALL checks PASS; 1 = one or more FAIL; 2 = usage/environment.

--selftest builds REAL temporary git repositories (no network) and proves:
  v3.1 same-commit model rejected; two-commit C->R->E model accepted; wrong
  candidate/result/tree OIDs rejected; E parent != R rejected; envelope present
  in R rejected; implementation change in E rejected; missing envelope in E
  rejected; self container commit field rejected; self tree field rejected;
  envelope self-SHA256 rejected; remote branch mismatch rejected; correct
  external branch binding accepted. Zero residual processes/directories.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import tempfile
import wave

SCHEMA = "acceptance-verifier-v3.2"
VERSION = "3.2.0"

OWNED_LAB_A = [
    "tests/integration/headless_audio_lab/process_supervisor.py",
    "tests/integration/headless_audio_lab/jack_server.py",
    "tests/integration/headless_audio_lab/osc_probe.py",
    "tests/integration/headless_audio_lab/sooperlooper_launcher.py",
    "tests/integration/headless_audio_lab/__init__.py",
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
]
ENVELOPE_LAB_A = "receipts/headless-lab-results-v3.2/lab-a-result.json"
ENVELOPE_LAB_B = "receipts/headless-lab-results-v3.2/lab-b-result.json"

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

V3_DEFECTIVE_PROBE_SNIPPET = (
    "foreign = sup.start_process('foreign', ['/bin/sleep', '5'], {}, '.', '/dev/null', '/dev/null')\n"
    "sup.cleanup(timeout=5.0)\n"
    "foreign_alive = os.path.exists('/proc/%d' % foreign.pid)\n"
    "out['foreign_not_signaled'] = foreign_alive\n"
)

FORBIDDEN_SELF_KEYS = [
    "container_commit_oid",
    "container_tree_oid",
    "envelope_blob_oid",
    "envelope_sha256",
    "self_sha256",
    "remote_branch_resolved_sha",
]


def detect_v3_foreign_probe_pattern(probe_src):
    """Detect the v3 LAB-A acceptance-gate contradiction in probe source."""
    lines = probe_src.splitlines()
    created_via_start = []
    cleanup_indexes = []
    for i, ln in enumerate(lines):
        m = re.search(r"(\w+)\s*=\s*sup\.start_process\s*\(", ln)
        if m:
            created_via_start.append((m.group(1), i))
        if re.search(r"sup\.cleanup\s*\(", ln):
            cleanup_indexes.append(i)
    reasons = []
    for var, idx in created_via_start:
        for j in range(idx, len(lines)):
            ln = lines[j]
            m2 = re.search(r"(\w+)\s*=\s*os\.path\.exists\('/proc/%d'\s*%\s*(\w+)\.pid\)", ln)
            if not m2 or m2.group(2) != var:
                continue
            if not any(c < j for c in cleanup_indexes):
                continue
            nxt = lines[j + 1] if j + 1 < len(lines) else ""
            direct_record = re.search(
                r"out\s*\[\s*['\"][^'\"]+['\"]\s*\]\s*=\s*" + re.escape(m2.group(1)) + r"\s*$",
                nxt)
            reasons.append(
                "start_process-created '%s' checked for survival after cleanup (line %d)%s"
                % (var, j + 1, " [exact v3 signature]" if direct_record else ""))
    return reasons


def build_probe_script(supervisor_import_lines):
    """Build the corrected LAB-A probe script (foreign OUTSIDE supervisor)."""
    return (
        "import json\n"
        "import os\n"
        "import subprocess\n"
        "import sys\n"
        "import time\n"
        + supervisor_import_lines
        + "\n"
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


LAB_A_CONDITIONS = [
    ("owned_pid_positive", lambda d: d.get("owned_pid_positive") is True),
    ("owned_child_alive_before_cleanup", lambda d: d.get("owned_child_alive_before_cleanup") is True),
    ("owned_child_alive_after_cleanup", lambda d: d.get("owned_child_alive_after_cleanup") is False),
    ("cleanup_ok", lambda d: d.get("cleanup_ok") is True),
    ("foreign_alive_after_supervisor_cleanup", lambda d: d.get("foreign_alive_after_supervisor_cleanup") is True),
    ("failure_propagation", lambda d: d.get("failure_propagation") is True),
    ("foreign_registered_in_supervisor", lambda d: d.get("foreign_registered_in_supervisor") is False),
    ("foreign_terminated_by_controller", lambda d: d.get("foreign_terminated_by_controller") is True),
]


class Result:
    def __init__(self):
        self.checks = []
        self.env_notes = []

    def add(self, name, status, detail="", evidence=None):
        self.checks.append({"name": name, "status": status, "detail": detail,
                            "evidence": evidence if evidence is not None else {}})

    def ok(self, name, detail="", evidence=None):
        self.add(name, "PASS", detail, evidence)

    def fail(self, name, detail="", evidence=None):
        self.add(name, "FAIL", detail, evidence)

    def failed(self):
        return any(c["status"] == "FAIL" for c in self.checks)


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_b(b):
    return hashlib.sha256(b).hexdigest()


def run(cmd, cwd, timeout=120, env=None):
    r = subprocess.run(cmd, shell=False, cwd=cwd, capture_output=True, text=True, timeout=timeout,
                       env=env)
    return {"exit": r.returncode, "stdout": r.stdout, "stderr": r.stderr}


def git(repo, args):
    r = run(["git", "-C", repo] + args, repo)
    return r


def git_show(checkout, rev, path):
    r = git(checkout, ["show", "%s:%s" % (rev, path)])
    if r["exit"] != 0:
        return None
    return r["stdout"]


def scan_for_patterns(content, patterns, label):
    hits = []
    for pat, name in patterns:
        if pat.search(content):
            hits.append(name)
    return hits


def load_contract(path):
    with open(path) as f:
        return json.load(f)


def contract_analyze_wav(contract):
    try:
        fns = contract["modules"]["audio_oracle.py"]["functions"]
    except (KeyError, TypeError):
        return None
    for fn in fns:
        if fn.get("name") == "analyze_wav":
            return fn
    return None


# ---------------------------------------------------------------------------
# LAB gates (carried from the v3.1 corrections)
# ---------------------------------------------------------------------------
def run_probe_script(checkout, probe_script, label):
    probe = os.path.join(checkout, "_acceptance_probe_%s.py" % label)
    with open(probe, "w") as f:
        f.write(probe_script)
    env = dict(os.environ)
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    r = run(["python3", probe], checkout, timeout=120, env=env)
    try:
        os.unlink(probe)
    except OSError:
        pass
    d = {}
    if r["exit"] == 0:
        try:
            d = json.loads(r["stdout"].strip().splitlines()[-1])
        except Exception:
            d = {"_parse_error": r["stdout"][-500:]}
    return r, d


def check_lab_a(checkout, contract, res):
    # imports
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
    if ph_hits:
        res.fail("lab-a: placeholder scan", "placeholder patterns found in owned sources",
                 evidence=ph_hits)
    else:
        res.ok("lab-a: placeholder scan", "no placeholder patterns in owned sources")

    probe_src = build_probe_script(
        "sys.path.insert(0, 'tests/integration')\n"
        "from headless_audio_lab.process_supervisor import ProcessSupervisor\n")
    defects = detect_v3_foreign_probe_pattern(probe_src)
    res.add("lab-a: harness probe pattern integrity",
            "PASS" if not defects else "FAIL",
            "shipped corrected probe must not contain the v3 foreign-survival contradiction",
            evidence={"detected": defects})

    r, d = run_probe_script(checkout, probe_src, "lab_a_v32")
    if r["exit"] != 0:
        res.fail("lab-a: functional lifecycle",
                 "probe failed exit=%d stderr=%s" % (r["exit"], r["stderr"][-500:]))
        return
    cond_results = {}
    ok_all = True
    for cond, fn in LAB_A_CONDITIONS:
        val = fn(d)
        cond_results[cond] = {"satisfied": bool(val), "observed": d.get(cond)}
        ok_all = ok_all and bool(val)
    res.add("lab-a: functional lifecycle", "PASS" if ok_all else "FAIL",
            "owned PID>0, owned terminated after cleanup, verify_cleanup()==[], "
            "foreign survives supervisor cleanup, foreign created outside supervisor",
            evidence=cond_results)


def check_lab_b(checkout, contract, res):
    cfn = contract_analyze_wav(contract)
    impl = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/audio_oracle.py")
    if impl is None:
        res.fail("lab-b: analyze_wav signature", "audio_oracle.py missing at HEAD")
        return
    m = re.search(r"def analyze_wav\((.*?)\)\s*->", impl, re.S)
    if not cfn or not m:
        res.fail("lab-b: analyze_wav signature", "cannot parse contract or implementation")
        return
    contract_args = [a["name"] for a in cfn["args"]]
    impl_sig = re.sub(r"\s+", " ", m.group(1))
    missing = [a for a in contract_args if not re.search(r"\b%s\b" % a, impl_sig)]
    res.add("lab-b: analyze_wav signature", "PASS" if not missing else "FAIL",
            "contract args=%s" % contract_args,
            evidence={"impl_sig": impl_sig, "missing": missing})

    tm = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/test_modules.py") or ""
    hits = scan_for_patterns(tm, TRIVIAL_TEST_PATTERNS, "lab-b")
    invokes = "analyze_wav(" in tm
    if hits or not invokes:
        res.fail("lab-b: test_modules negative gates", "hits=%s invokes_analyze_wav=%s" % (hits, invokes))
    else:
        res.ok("lab-b: test_modules negative gates", "no trivial tests; analyze_wav invoked")

    probe = os.path.join(checkout, "_acceptance_probe_lab_b_v32.py")
    with open(probe, "w") as f:
        f.write(
            "import tempfile, wave, os, sys, json\n"
            "sys.path.insert(0, 'tests/integration')\n"
            "from headless_audio_lab.audio_oracle import analyze_wav\n"
            "out = {}\n"
            "fd, p = tempfile.mkstemp(suffix='.wav'); os.close(fd)\n"
            "w = wave.open(p, 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(48000)\n"
            "w.writeframes(b'\\x00\\x00' * 4800); w.close()\n"
            "r = analyze_wav(p, 4800, 4800, 120.0, 4, 48000)\n"
            "d = r.to_dict() if hasattr(r, 'to_dict') else r\n"
            "out['silence_verification'] = d.get('verification')\n"
            "os.unlink(p)\n"
            "fd2, p2 = tempfile.mkstemp(suffix='.wav'); os.write(fd2, b'RIFFnotreallyawav'); os.close(fd2)\n"
            "try:\n"
            "    r2 = analyze_wav(p2, 4800, 4800, 120.0, 4, 48000)\n"
            "    d2 = r2.to_dict() if hasattr(r2, 'to_dict') else r2\n"
            "    out['malformed_result'] = d2.get('verification')\n"
            "except Exception as e:\n"
            "    out['malformed_result'] = 'EXCEPTION:' + type(e).__name__\n"
            "os.unlink(p2)\n"
            "fd3, p3 = tempfile.mkstemp(suffix='.wav'); os.close(fd3)\n"
            "w3 = wave.open(p3, 'wb'); w3.setnchannels(1); w3.setsampwidth(2); w3.setframerate(48000)\n"
            "w3.writeframes(b'\\x01\\x02' * 4800); w3.close()\n"
            "r3 = analyze_wav(p3, 4800, 4800, 120.0, 4, 48000)\n"
            "d3 = r3.to_dict() if hasattr(r3, 'to_dict') else r3\n"
            "out['frame_count'] = d3.get('captured_wav_frames')\n"
            "os.unlink(p3)\n"
            "print(json.dumps(out))\n"
        )
    r = run(["python3", probe], checkout, timeout=120)
    try:
        os.unlink(probe)
    except OSError:
        pass
    if r["exit"] == 0:
        try:
            d = json.loads(r["stdout"].strip().splitlines()[-1])
        except Exception:
            d = {}
        silence_ok = d.get("silence_verification") == "FAIL"
        malformed_ok = str(d.get("malformed_result", "")).startswith(("FAIL", "ERROR", "EXCEPTION"))
        frame_ok = d.get("frame_count") == 4800
        res.add("lab-b: silence always FAIL", "PASS" if silence_ok else "FAIL",
                "silent WAV must produce FAIL", evidence=d)
        res.add("lab-b: malformed WAV FAIL/ERROR", "PASS" if malformed_ok else "FAIL",
                evidence={"malformed_result": d.get("malformed_result")})
        res.add("lab-b: frame count contract", "PASS" if frame_ok else "FAIL",
                "captured_wav_frames=%s expected 4800" % d.get("frame_count"))
    else:
        res.fail("lab-b: audio probes", "probe failed exit=%d stderr=%s" % (r["exit"], r["stderr"][-500:]))

    gi = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/fixtures/.gitignore") or ""
    literal_n = "\\n" in gi
    real_newline = "\n" in gi
    res.add("lab-b: .gitignore byte-exact",
            "PASS" if (not literal_n and real_newline) else "FAIL",
            "literal '\\\\n' rejected; real newlines required", evidence={"repr": repr(gi)})

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
            res.env_notes.append("PyYAML unavailable; scenario %s checked structurally only" % s)
            if not content.strip() or not any(k in content for k in
                                              ["source:", "capture:", "sooperlooper:", "expected:"]):
                bad.append({"scenario": s, "error": "missing required sections"})
    res.add("lab-b: scenarios parseable", "PASS" if not bad else "FAIL", evidence={"bad": bad})

    runner = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/runner.py") or ""
    api_hits = [a for a in ["ProcessSupervisor", "JackServer", "OscProbe", "SooperLooperLauncher",
                            "analyze_wav", "GraphAssertions"] if a in runner]
    res.add("lab-b: runner uses contractual APIs",
            "PASS" if len(api_hits) >= 3 else "FAIL",
            "API hits: %s" % api_hits)


# ---------------------------------------------------------------------------
# Two-commit envelope model verification (C -> R -> E)
# ---------------------------------------------------------------------------
def verify_envelope_model(repo, c_oid, r_oid, e_oid, envelope_path, lab, res):
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

    r_parent = git(repo, ["rev-parse", "%s^" % r_oid])["stdout"].strip()
    e_parent = git(repo, ["rev-parse", "%s^" % e_oid])["stdout"].strip()
    res.add("model: R^ == C", "PASS" if r_parent == c_oid else "FAIL",
            evidence={"r_parent": r_parent, "c": c_oid})
    res.add("model: E^ == R", "PASS" if e_parent == r_oid else "FAIL",
            evidence={"e_parent": e_parent, "r": r_oid})

    env_in_r = git(repo, ["cat-file", "-e", "%s:%s" % (r_oid, envelope_path)])
    res.add("model: envelope absent from R",
            "PASS" if env_in_r["exit"] != 0 else "FAIL",
            "envelope must NOT be committed in the implementation result commit")
    env_in_e = git(repo, ["cat-file", "-e", "%s:%s" % (e_oid, envelope_path)])
    res.add("model: envelope present in E",
            "PASS" if env_in_e["exit"] == 0 else "FAIL",
            "envelope must be committed in the envelope container commit")

    r_tree = git(repo, ["rev-parse", "%s^{tree}" % r_oid])["stdout"].strip()
    e_tree = git(repo, ["rev-parse", "%s^{tree}" % e_oid])["stdout"].strip()

    # diff C..R limited to owned paths
    owned = set(OWNED_LAB_A if lab == "a" else OWNED_LAB_B)
    dr = git(repo, ["diff", "--name-only", c_oid, r_oid])
    r_diff_paths = [ln for ln in dr["stdout"].splitlines() if ln]
    bad_impl = [p for p in r_diff_paths if p not in owned and p != envelope_path]
    res.add("model: diff C..R owned-only", "PASS" if not bad_impl else "FAIL",
            evidence={"changed": r_diff_paths, "out_of_scope": bad_impl})

    # diff R..E exactly the single frozen envelope path
    de = git(repo, ["diff", "--name-only", r_oid, e_oid])
    e_diff_paths = [ln for ln in de["stdout"].splitlines() if ln]
    envelope_only_ok = e_diff_paths == [envelope_path]
    res.add("model: diff R..E envelope-only", "PASS" if envelope_only_ok else "FAIL",
            evidence={"changed": e_diff_paths, "expected": [envelope_path]})

    env_raw = git(repo, ["show", "%s:%s" % (e_oid, envelope_path)])["stdout"]
    try:
        env = json.loads(env_raw)
    except Exception as e:
        res.fail("model: envelope parses", "invalid JSON: %s" % e)
        return

    checks = {
        "candidate_commit_oid == C": env.get("candidate_commit_oid") == c_oid,
        "implementation_result_commit_oid == R": env.get("implementation_result_commit_oid") == r_oid,
        "implementation_result_tree_oid == R^{tree}": env.get("implementation_result_tree_oid") == r_tree,
        "git_object_format matches": env.get("git_object_format") == git(repo, ["rev-parse", "--show-object-format"])["stdout"].strip(),
    }
    expected_map = {
        "candidate_commit_oid == C": c_oid,
        "implementation_result_commit_oid == R": r_oid,
        "implementation_result_tree_oid == R^{tree}": r_tree,
        "git_object_format matches": git(repo, ["rev-parse", "--show-object-format"])["stdout"].strip(),
    }
    observed_map = {
        "candidate_commit_oid == C": env.get("candidate_commit_oid"),
        "implementation_result_commit_oid == R": env.get("implementation_result_commit_oid"),
        "implementation_result_tree_oid == R^{tree}": env.get("implementation_result_tree_oid"),
        "git_object_format matches": env.get("git_object_format"),
    }
    for name, ok_ in checks.items():
        res.add("model: %s" % name, "PASS" if ok_ else "FAIL",
                evidence={"observed": observed_map[name], "expected": expected_map[name]})

    # forbidden self-binding fields
    e_blob = git(repo, ["rev-parse", "%s:%s" % (e_oid, envelope_path)])["stdout"].strip()
    env_sha = sha256_b(env_raw.encode("utf-8"))
    bad_self = []
    for k in FORBIDDEN_SELF_KEYS:
        if k in env:
            bad_self.append("key %s present" % k)
    for k, v in env.items():
        if isinstance(v, str) and v in (e_oid, e_tree, e_blob):
            bad_self.append("field %s binds E identity" % k)
        if isinstance(v, str) and v == env_sha:
            bad_self.append("field %s binds envelope self-sha256" % k)
    res.add("model: no forbidden self-binding fields",
            "PASS" if not bad_self else "FAIL",
            evidence={"bad_self": bad_self, "e_blob_oid": e_blob, "e_tree_oid": e_tree})

    # artifacts: no envelope self-hash; only files present in R
    arts = env.get("artifacts", {})
    bad_art = []
    if envelope_path in arts:
        bad_art.append("artifacts contains the envelope path")
    for apath in arts:
        exists_in_r = git(repo, ["cat-file", "-e", "%s:%s" % (r_oid, apath)])
        if exists_in_r["exit"] != 0:
            bad_art.append("artifact %s not present in R" % apath)
        a = arts[apath]
        if isinstance(a, dict) and a.get("sha256") == env_sha:
            bad_art.append("artifact %s binds envelope self-sha256" % apath)
    res.add("model: artifacts safe (no envelope self-hash, R-only)",
            "PASS" if not bad_art else "FAIL",
            evidence={"bad_art": bad_art})


# ---------------------------------------------------------------------------
# Selftest: real temporary git repositories (no network, no LAB worktrees)
# ---------------------------------------------------------------------------
def _git_quiet(repo, args, ok_exit=True):
    r = git(repo, args)
    if ok_exit and r["exit"] != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), r["stderr"][-300:]))
    return r["stdout"].strip()


def _write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(content)


def _commit(repo, msg):
    _git_quiet(repo, ["add", "-A"])
    _git_quiet(repo, ["commit", "-q", "-m", msg])
    return _git_quiet(repo, ["rev-parse", "HEAD"])


def _init_repo(base, name):
    repo = os.path.join(base, name)
    subprocess.run(["git", "init", "-q", repo], check=True)
    _git_quiet(repo, ["config", "user.name", "selftest"])
    _git_quiet(repo, ["config", "user.email", "selftest@local"])
    return repo


def _default_envelope(repo, c, r_oid, r_tree_oid, lab, batch, leaf, lease,
                      artifacts_extra=None):
    env = {
        "schema": "headless-lab-result-envelope/v3.2",
        "lab": lab,
        "batch_id": batch,
        "leaf_id": leaf,
        "lease_id": lease,
        "candidate_commit_oid": c,
        "implementation_result_commit_oid": r_oid,
        "implementation_result_tree_oid": r_tree_oid,
        "git_object_format": _git_quiet(repo, ["rev-parse", "--show-object-format"]),
        "external_branch_binding": {"remote": "origin",
                                    "planned_branch": "result/selftest-%s-v3_2" % lab.lower()},
        "literal_tests": {"ft-s1": {"evidence": "selftest", "exit_code": 0}},
        "build_provenance": {"source": "selftest fixture"},
        "artifacts": {"README.md": {"kind": "source",
                                    "sha256": sha256_file(os.path.join(repo, "README.md")),
                                    "size": os.path.getsize(os.path.join(repo, "README.md"))}},
    }
    if artifacts_extra:
        env["artifacts"].update(artifacts_extra)
    return env


def _build_good_repo(base, lab, batch, leaf, lease, impl_files, env_path,
                     mutate=None):
    """Build C -> R -> E with a correct non-self-referential envelope.

    mutate(repo, c, r, r_tree, env_dict) may adjust the envelope before commit
    (used by negative fixtures). Returns dict of oids.
    """
    repo = _init_repo(base, "work")
    _write_file(os.path.join(repo, "README.md"), "candidate base\n")
    c = _commit(repo, "C candidate")
    for path, content in impl_files.items():
        _write_file(os.path.join(repo, path), content)
    r = _commit(repo, "R implementation result")
    r_tree = _git_quiet(repo, ["rev-parse", "HEAD^{tree}"])
    env = _default_envelope(repo, c, r, r_tree, lab, batch, leaf, lease)
    if mutate:
        mutate(repo, c, r, r_tree, env)
    _write_file(os.path.join(repo, env_path), json.dumps(env, indent=2) + "\n")
    e = _commit(repo, "E envelope container")
    e_tree = _git_quiet(repo, ["rev-parse", "HEAD^{tree}"])
    e_blob = _git_quiet(repo, ["rev-parse", "HEAD:%s" % env_path])
    return {"repo": repo, "C": c, "R": r, "E": e, "R_tree": r_tree,
            "E_tree": e_tree, "E_blob": e_blob}


def _run_model_check(repo, c, r, e, envelope_path, lab):
    res = Result()
    verify_envelope_model(repo, c, r, e, envelope_path, lab, res)
    return res


def _bare_origin(base, repo, branch, target_oid, point_to=None):
    bare = os.path.join(base, "origin.git")
    subprocess.run(["git", "init", "-q", "--bare", bare], check=True)
    _git_quiet(repo, ["remote", "add", "origin", bare])
    src = target_oid if point_to is None else point_to
    _git_quiet(repo, ["push", "-q", "origin", "%s:refs/heads/%s" % (src, branch)])
    return bare


def _remote_binding_check(bare, branch, expected_oid):
    r = subprocess.run(["git", "ls-remote", bare, branch], capture_output=True, text=True, timeout=60)
    resolved = r.stdout.split()[0] if r.stdout.strip() else None
    return r.returncode == 0 and resolved == expected_oid, resolved


def selftest():
    res = Result()
    lab = "a"
    env_path = ENVELOPE_LAB_A
    impl_files = {
        "tests/integration/headless_audio_lab/__init__.py": "# lab\n",
        "tests/integration/headless_audio_lab/process_supervisor.py":
            "class ProcessSupervisor:\n"
            "    def start_process(self, name, cmd, env, cwd, so, se):\n"
            "        import subprocess\n"
            "        p = subprocess.Popen(cmd, start_new_session=True)\n"
            "        return type('I', (), {'pid': p.pid, 'pgid': p.pid})()\n"
            "    def get_owned_pids(self):\n        return []\n"
            "    def cleanup(self, timeout=5.0):\n        pass\n"
            "    def verify_cleanup(self):\n        return []\n",
    }
    batch = "BATCH-20260805T224640Z-DOGFOOD004-LAB-V3_2"
    leaf = "LEAF-LAB-A-V3_2"
    lease = "LEASE-20260805T224640Z-LAB-A-V3_2"

    def good():
        return _build_good_repo(tempfile.gettempdir(), lab, batch, leaf, lease,
                                impl_files, env_path)

    with tempfile.TemporaryDirectory(prefix="v32-selftest-") as tmp:
        # 1. v3.1 same-commit model rejected (envelope present in R)
        repo1 = _init_repo(os.path.join(tmp, "same"), "work")
        _write_file(os.path.join(repo1, "README.md"), "base\n")
        c1 = _commit(repo1, "C")
        for path, content in impl_files.items():
            _write_file(os.path.join(repo1, path), content)
        _write_file(os.path.join(repo1, env_path),
                    json.dumps(_default_envelope(repo1, c1, "R", "TREE", lab, batch, leaf, lease),
                               indent=2) + "\n")
        head1 = _commit(repo1, "R same-commit with envelope")
        rcheck = _run_model_check(repo1, c1, head1, head1, env_path, lab)
        res.add("selftest: v3.1 same-commit model rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 2. two-commit C->R->E model accepted
        g2 = _build_good_repo(os.path.join(tmp, "good"), lab, batch, leaf, lease,
                              impl_files, env_path)
        rcheck = _run_model_check(g2["repo"], g2["C"], g2["R"], g2["E"], env_path, lab)
        res.add("selftest: two-commit C->R->E model accepted",
                "PASS" if not rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 3. wrong candidate OID rejected
        g3 = _build_good_repo(os.path.join(tmp, "badcand"), lab, batch, leaf, lease,
                              impl_files, env_path,
                              mutate=lambda repo, c, r, rt, env: env.update(
                                  candidate_commit_oid="0" * 40))
        rcheck = _run_model_check(g3["repo"], g3["C"], g3["R"], g3["E"], env_path, lab)
        res.add("selftest: wrong candidate OID rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 4. wrong result commit OID rejected
        g4 = _build_good_repo(os.path.join(tmp, "badr"), lab, batch, leaf, lease,
                              impl_files, env_path,
                              mutate=lambda repo, c, r, rt, env: env.update(
                                  implementation_result_commit_oid="0" * 40))
        rcheck = _run_model_check(g4["repo"], g4["C"], g4["R"], g4["E"], env_path, lab)
        res.add("selftest: wrong result commit OID rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 5. wrong result tree OID rejected
        g5 = _build_good_repo(os.path.join(tmp, "badrt"), lab, batch, leaf, lease,
                              impl_files, env_path,
                              mutate=lambda repo, c, r, rt, env: env.update(
                                  implementation_result_tree_oid="0" * 40))
        rcheck = _run_model_check(g5["repo"], g5["C"], g5["R"], g5["E"], env_path, lab)
        res.add("selftest: wrong result tree OID rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 6. E parent != R rejected
        repo6 = _init_repo(os.path.join(tmp, "badparent"), "work")
        _write_file(os.path.join(repo6, "README.md"), "base\n")
        c6 = _commit(repo6, "C")
        for path, content in impl_files.items():
            _write_file(os.path.join(repo6, path), content)
        r6 = _commit(repo6, "R")
        r6_tree = _git_quiet(repo6, ["rev-parse", "HEAD^{tree}"])
        _write_file(os.path.join(repo6, "extra.txt"), "extra between R and E\n")
        extra6 = _commit(repo6, "M extra between R and E")
        _write_file(os.path.join(repo6, env_path),
                    json.dumps(_default_envelope(repo6, c6, r6, r6_tree, lab, batch, leaf, lease),
                               indent=2) + "\n")
        e6 = _commit(repo6, "E2 on wrong parent")
        rcheck = _run_model_check(repo6, c6, r6, e6, env_path, lab)
        res.add("selftest: E parent != R rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"],
                          "extra_commit": extra6})

        # 7. envelope present in R rejected (same-commit envelope)
        repo7 = _init_repo(os.path.join(tmp, "env-in-r"), "work")
        _write_file(os.path.join(repo7, "README.md"), "base\n")
        c7 = _commit(repo7, "C")
        for path, content in impl_files.items():
            _write_file(os.path.join(repo7, path), content)
        r7 = _commit(repo7, "R impl")
        r7_tree = _git_quiet(repo7, ["rev-parse", "HEAD^{tree}"])
        _write_file(os.path.join(repo7, env_path),
                    json.dumps(_default_envelope(repo7, c7, r7, r7_tree, lab, batch, leaf, lease),
                               indent=2) + "\n")
        r7b = _commit(repo7, "envelope committed in R (same-commit model)")
        rcheck = _run_model_check(repo7, c7, r7b, r7b, env_path, lab)
        res.add("selftest: envelope present in R rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 8. implementation change in E rejected
        g8 = _build_good_repo(os.path.join(tmp, "impl-in-e"), lab, batch, leaf, lease,
                              impl_files, env_path,
                              mutate=lambda repo, c, r, rt, env: (
                                  _write_file(os.path.join(repo, "tests/integration/headless_audio_lab/extra.py"),
                                              "def f():\n    return 2\n"),
                                  _commit(repo, "impl change committed with E")))
        rcheck = _run_model_check(g8["repo"], g8["C"], g8["R"], g8["E"], env_path, lab)
        res.add("selftest: implementation change in E rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 9. missing envelope in E rejected
        repo9 = _init_repo(os.path.join(tmp, "noenv"), "work")
        _write_file(os.path.join(repo9, "README.md"), "base\n")
        c9 = _commit(repo9, "C")
        for path, content in impl_files.items():
            _write_file(os.path.join(repo9, path), content)
        r9 = _commit(repo9, "R")
        _write_file(os.path.join(repo9, "other.txt"), "no envelope here\n")
        e9 = _commit(repo9, "E without envelope")
        rcheck = _run_model_check(repo9, c9, r9, e9, env_path, lab)
        res.add("selftest: missing envelope in E rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 10. self container commit field rejected
        g10 = _build_good_repo(os.path.join(tmp, "selfcommit"), lab, batch, leaf, lease,
                               impl_files, env_path,
                               mutate=lambda repo, c, r, rt, env: env.update(
                                   container_commit_oid="WILL_BE_E"))
        # need the real E oid: rebuild with post-hoc knowledge is impossible, so
        # use a sentinel that the forbidden-key scan rejects by name.
        rcheck = _run_model_check(g10["repo"], g10["C"], g10["R"], g10["E"], env_path, lab)
        res.add("selftest: self container commit field rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 11. self tree field rejected (field value == E tree oid)
        def mutate_self_tree(repo, c, r, rt, env):
            # write the envelope first to learn E tree? Not possible pre-commit.
            # The verifier rejects the forbidden KEY by name regardless of value.
            env["container_tree_oid"] = "WILL_BE_E_TREE"

        g11 = _build_good_repo(os.path.join(tmp, "selftree"), lab, batch, leaf, lease,
                               impl_files, env_path, mutate=mutate_self_tree)
        rcheck = _run_model_check(g11["repo"], g11["C"], g11["R"], g11["E"], env_path, lab)
        res.add("selftest: self tree field rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 12. envelope self-SHA256 rejected (field value == sha256 of envelope bytes)
        def mutate_self_sha(repo, c, r, rt, env):
            env["envelope_sha256"] = sha256_b(json.dumps(env, sort_keys=True).encode("utf-8"))

        g12 = _build_good_repo(os.path.join(tmp, "selfsha"), lab, batch, leaf, lease,
                               impl_files, env_path, mutate=mutate_self_sha)
        rcheck = _run_model_check(g12["repo"], g12["C"], g12["R"], g12["E"], env_path, lab)
        res.add("selftest: envelope self-SHA256 rejected",
                "PASS" if rcheck.failed() else "FAIL",
                evidence={"failing_checks": [c["name"] for c in rcheck.checks if c["status"] == "FAIL"]})

        # 13. remote branch mismatch rejected
        g13 = _build_good_repo(os.path.join(tmp, "remotemismatch"), lab, batch, leaf, lease,
                               impl_files, env_path)
        branch = "result/selftest-a-v3_2"
        bare13 = _bare_origin(os.path.join(tmp, "rm-remote"), g13["repo"], branch, g13["E"],
                              point_to=g13["C"])
        ok13, resolved13 = _remote_binding_check(bare13, branch, g13["E"])
        res.add("selftest: remote branch mismatch rejected",
                "PASS" if not ok13 else "FAIL",
                evidence={"resolved": resolved13, "expected": g13["E"], "actual_is_C": resolved13 == g13["C"]})

        # 14. correct external branch binding accepted
        g14 = _build_good_repo(os.path.join(tmp, "remoteok"), lab, batch, leaf, lease,
                               impl_files, env_path)
        bare14 = _bare_origin(os.path.join(tmp, "ok-remote"), g14["repo"], branch, g14["E"])
        ok14, resolved14 = _remote_binding_check(bare14, branch, g14["E"])
        res.add("selftest: correct external branch binding accepted",
                "PASS" if ok14 else "FAIL",
                evidence={"resolved": resolved14, "expected": g14["E"]})

    # residual dirs check: TemporaryDirectory already removed; verify nothing
    # was left under the model dir
    import glob
    leftovers = glob.glob(os.path.join(tempfile.gettempdir(), "v32-selftest-*"))
    res.add("selftest: zero residual directories",
            "PASS" if not leftovers else "FAIL",
            evidence={"leftovers": leftovers})
    return res


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------
def _clean_checkout_of_r(repo, r_oid, base_tmp):
    """Create a clean detached checkout of the exact R commit (controller scratch)."""
    wt = os.path.join(base_tmp, "checkout-R")
    r = git(repo, ["worktree", "add", "--detach", wt, r_oid])
    if r["exit"] != 0:
        raise RuntimeError("worktree add failed: %s" % r["stderr"][-300:])
    return wt


def main():
    ap = argparse.ArgumentParser(description=SCHEMA)
    ap.add_argument("--lab", choices=["a", "b"])
    ap.add_argument("--repository", help="path to the repository containing C/R/E")
    ap.add_argument("--candidate-head", help="C commit OID")
    ap.add_argument("--result-head", help="R implementation result commit OID")
    ap.add_argument("--envelope-container-head", help="E envelope container commit OID")
    ap.add_argument("--contract", help="path to lab-api-contract-v3.2.json")
    ap.add_argument("--envelope-path", help="repo-relative path of the frozen envelope")
    ap.add_argument("--json-out", help="write findings JSON to path")
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
                "--envelope-container-head", "--envelope-path", "--contract"]
    missing = [req for req in required if getattr(args, req.lstrip("--").replace("-", "_")) is None]
    if missing:
        ap.error("missing required arguments: %s" % ", ".join(missing))

    res = Result()
    repo = args.repository
    c_oid = args.candidate_head
    r_oid = args.result_head
    e_oid = args.envelope_container_head
    envelope_path = args.envelope_path
    lab = args.lab
    contract = load_contract(args.contract)
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
    sys.dont_write_bytecode = True

    # 1. two-commit envelope model verification
    verify_envelope_model(repo, c_oid, r_oid, e_oid, envelope_path, lab, res)

    # 2. functional gates on a clean checkout of the exact R
    tmp = tempfile.mkdtemp(prefix="v32-checkout-")
    wt = None
    try:
        wt = _clean_checkout_of_r(repo, r_oid, tmp)
        st0 = git(wt, ["status", "--porcelain=v1"])
        dirty0 = [ln for ln in st0["stdout"].splitlines() if ln]
        res.add("model: clean worktree at R (on arrival)",
                "PASS" if not dirty0 else "FAIL",
                evidence={"dirty": dirty0})
        if lab == "a":
            check_lab_a(wt, contract, res)
        else:
            check_lab_b(wt, contract, res)
        # clean worktree at R after gates
        st = git(wt, ["status", "--porcelain=v1"])
        dirty = [ln for ln in st["stdout"].splitlines() if ln]
        res.add("model: clean worktree at R (after gates)",
                "PASS" if not dirty else "FAIL",
                evidence={"dirty": dirty})
    finally:
        if wt is not None:
            subprocess.run(["git", "-C", repo, "worktree", "remove", "--force", wt],
                           capture_output=True, timeout=60)
        import shutil
        shutil.rmtree(tmp, ignore_errors=True)

    if args.json_out:
        with open(args.json_out, "w") as f:
            json.dump({"schema": SCHEMA, "version": VERSION,
                       "lab": lab.upper(), "repository": repo,
                       "candidate_head": c_oid, "result_head": r_oid,
                       "envelope_container_head": e_oid,
                       "envelope_path": envelope_path,
                       "checks": res.checks, "env_notes": res.env_notes,
                       "verdict": "REJECT" if res.failed() else "PASS"}, f, indent=2)
    for c in res.checks:
        print("[%s] %s — %s" % (c["status"], c["name"], c["detail"]))
    print("VERDICT:", "REJECT (one or more FAIL)" if res.failed() else "PASS")
    return 1 if res.failed() else 0


if __name__ == "__main__":
    sys.exit(main())
