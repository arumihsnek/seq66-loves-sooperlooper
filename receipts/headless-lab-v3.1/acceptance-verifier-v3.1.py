#!/usr/bin/env python3
"""acceptance-verifier-v3.1.py — Controller-owned acceptance harness for
headless-lab v3.1 dispatch (LAB-A / LAB-B).

OWNERSHIP: this script belongs to the controller/control plane. It is NOT an
owned path of LAB-A or LAB-B and may not be modified by a leaf. Its SHA-256 is
frozen in acceptance-manifest-v3.1.json. The controller executes it against a
CLEAN CHECKOUT of the exact result head.

Gates implemented:
  LAB-A: placeholder rejection (pass/TODO/NotImplemented/dummy
    ProcessInfo/pid=0/MagicMock/always-success/import-only), owned child
    lifecycle with REAL positive PID, identity verification, bounded shutdown,
    failure propagation, AND the CORRECTED foreign-process control:
      * the foreign-survival probe process is created OUTSIDE the supervisor
        via subprocess.Popen(start_new_session=True);
      * cleanup(timeout) must terminate ALL and ONLY owned processes
        (owned child terminated, verify_cleanup() == []);
      * the foreign process must SURVIVE supervisor cleanup and is torn down
        by the controller through its own Popen object (TERM, bounded wait,
        KILL exact fallback);
      * harness probe pattern integrity: the shipped probe must NOT create a
        process via start_process and expect it to survive cleanup.
  LAB-B: exact analyze_wav signature vs contract, real silence test (silence
    always FAIL), malformed WAV FAIL/ERROR, contractual frame count, no empty
    tests, no trivial asserts, .gitignore byte-exact, parseable scenarios,
    runner uses contractual APIs, no ELF tracked.

Exit codes: 0 = ALL required checks PASS (SKIP is recorded, not PASS);
1 = one or more FAIL; 2 = usage/environment error.

--selftest: proves the negative gates are non-vacuous: known-bad LAB-A
placeholder rejected, known-bad LAB-B trivial tests rejected, known-good
nontrivial LAB-B test not falsely rejected, the v3 defective foreign-process
pattern DETECTED/REJECTED, the v3.1 corrected foreign-process pattern ACCEPTED,
external process survives supervisor cleanup in a controlled fixture, owned
process does NOT survive supervisor cleanup, and all temporary children are
cleaned at the end. Selftest FAILS if: foreign is created by
supervisor.start_process; owned survives cleanup; foreign is killed by
supervisor cleanup; or a temporary process remains after the test.

--controlled-ownership-probe: runs the corrected LAB-A probe logic against a
fixture supervisor module (--probe-supervisor PATH) and exits 0 iff every
condition passes. Used to prove the corrected gate is satisfiable by a
contract-conformant supervisor without any leaf implementation.
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

SCHEMA = "acceptance-verifier-v3.1"
VERSION = "3.1.0"

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
    "receipts/headless-lab-results-v3.1/lab-b-result.json",
]
ENVELOPE_LAB_A = "receipts/headless-lab-results-v3.1/lab-a-result.json"
ENVELOPE_LAB_B = "receipts/headless-lab-results-v3.1/lab-b-result.json"

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

# The EXACT v3 defective sequence (preserved from acceptance-verifier-v3.py at
# the v3 reviewed head), used by selftest to prove the detector is non-vacuous.
V3_DEFECTIVE_PROBE_SNIPPET = (
    "foreign = sup.start_process('foreign', ['/bin/sleep', '5'], {}, '.', '/dev/null', '/dev/null')\n"
    "sup.cleanup(timeout=5.0)\n"
    "foreign_alive = os.path.exists('/proc/%d' % foreign.pid)\n"
    "out['foreign_not_signaled'] = foreign_alive\n"
)


def detect_v3_foreign_probe_pattern(probe_src):
    """Detect the v3 LAB-A acceptance-gate contradiction in probe source.

    Defect signature: a process created via sup.start_process(...) is OWNED,
    so cleanup(timeout) MUST terminate it; a probe that then checks that same
    process for SURVIVAL after sup.cleanup(...) contradicts the contract.

    R1 (exact v3 signature): <var> = sup.start_process(...); sup.cleanup(...);
        <var>_alive = os.path.exists('/proc/%d' % <var>.pid);
        out['...'] = <var>_alive
    R2 (generic): any <var> created via sup.start_process(...) assigned to a
        BARE name on an os.path.exists('/proc/%d' % <var>.pid) line that
        follows a sup.cleanup(...) line. The corrected v3.1 probe only writes
        os.path.exists(...) inline inside out[...] = ..., so it is NOT flagged.

    Returns a list of human-readable reasons; empty list == no defect.
    """
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


# ---------------------------------------------------------------------------
# Corrected LAB-A probe
# ---------------------------------------------------------------------------
def build_probe_script(supervisor_import_lines):
    """Build the corrected LAB-A probe script.

    The foreign-survival probe process is created OUTSIDE ProcessSupervisor via
    subprocess.Popen(start_new_session=True) and is NEVER registered as owned.
    The owned process is created via start_process. cleanup(timeout) must
    terminate the owned process; the foreign process must survive. The foreign
    process is torn down in a finally block through its own Popen object with
    bounded waits and an exact KILL fallback.
    """
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


def run_probe_script(checkout, probe_script, label):
    """Write + run the probe script; returns (exit, out_dict, stderr_tail)."""
    probe = os.path.join(checkout, "_acceptance_probe_%s.py" % label)
    with open(probe, "w") as f:
        f.write(probe_script)
    r = run(["python3", probe], checkout)
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

    def skip(self, name, detail=""):
        self.add(name, "SKIP", detail)

    def failed(self):
        return any(c["status"] == "FAIL" for c in self.checks)


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(cmd, cwd, timeout=120):
    r = subprocess.run(cmd, shell=False, cwd=cwd, capture_output=True, text=True, timeout=timeout)
    return {"exit": r.returncode, "stdout": r.stdout, "stderr": r.stderr}


def git_show(checkout, rev, path):
    r = run(["git", "-C", checkout, "show", "%s:%s" % (rev, path)], checkout)
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
# LAB-A gates
# ---------------------------------------------------------------------------
def check_lab_a(checkout, contract, envelope_path, res):
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

    # placeholder scan of owned sources
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

    # harness probe pattern integrity: the shipped probe must NOT contain the
    # v3 defective pattern (foreign created via start_process and expected to
    # survive cleanup)
    probe_src = build_probe_script(
        "sys.path.insert(0, 'tests/integration')\n"
        "from headless_audio_lab.process_supervisor import ProcessSupervisor\n")
    defects = detect_v3_foreign_probe_pattern(probe_src)
    res.add("lab-a: harness probe pattern integrity",
            "PASS" if not defects else "FAIL",
            "shipped corrected probe must not contain the v3 foreign-survival contradiction",
            evidence={"detected": defects})

    # functional: corrected lifecycle probe
    r, d = run_probe_script(checkout, probe_src, "lab_a_v31")
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
            "owned PID>0, owned alive before cleanup, owned terminated after cleanup, "
            "verify_cleanup()==[], foreign survives supervisor cleanup, failure propagation, "
            "foreign created outside supervisor and torn down by controller",
            evidence=cond_results)


# ---------------------------------------------------------------------------
# LAB-B gates (unchanged semantics from v3)
# ---------------------------------------------------------------------------
def check_lab_b(checkout, contract, envelope_path, res):
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

    probe = os.path.join(checkout, "_acceptance_probe_lab_b_v31.py")
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
    r = run(["python3", probe], checkout)
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

    r = run(["git", "-C", checkout, "ls-tree", "-r", "HEAD"], checkout)
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
# Envelope validation (non-self-referential model)
# ---------------------------------------------------------------------------
def check_envelope(checkout, envelope_path, expected_lab, res):
    if envelope_path is None or not os.path.exists(envelope_path):
        res.fail("envelope: %s" % expected_lab, "envelope file missing at %s" % envelope_path)
        return
    with open(envelope_path) as f:
        env = json.load(f)
    head = run(["git", "-C", checkout, "rev-parse", "HEAD"], checkout)["stdout"].strip()
    head_tree = run(["git", "-C", checkout, "rev-parse", "HEAD^{tree}"], checkout)["stdout"].strip()
    parent = run(["git", "-C", checkout, "rev-parse", "HEAD^"], checkout)
    parent_sha = parent["stdout"].strip() if parent["exit"] == 0 else None
    checks = {
        "envelope_lab_matches": env.get("lab") == expected_lab,
        "tested_parent_sha_matches": env.get("tested_parent_sha") == parent_sha,
        "result_tree_sha_matches": env.get("result_tree_sha") == head_tree,
        "container_commit_bound_externally": bool(env.get("external_branch_binding")),
        "artifact_hashes_present": isinstance(env.get("artifacts"), dict) and len(env["artifacts"]) > 0,
    }
    bad = [k for k, v in checks.items() if not v]
    res.add("envelope: %s" % expected_lab,
            "PASS" if not bad else "FAIL",
            "non-self-referential binding: tested_parent_sha + result_tree_sha + external branch",
            evidence={**checks, "head": head, "envelope": env})


# ---------------------------------------------------------------------------
# Embedded contract-conformant fixture (selftest + controlled probe fallback)
# ---------------------------------------------------------------------------
class _FixtureProcessInfo:
    def __init__(self, pid, pgid):
        self.pid = pid
        self.pgid = pgid


class _FixtureSupervisor:
    """Minimal contract-conformant supervisor embedded for selftest.

    start_process registers every process it creates as owned; get_owned_pids
    reports those PIDs; cleanup terminates all and only registered owned PIDs;
    verify_cleanup returns no remaining owned PIDs.
    """

    def __init__(self):
        self._owned = {}

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.cleanup(timeout=5.0)
        return False

    def start_process(self, name, cmd, env, cwd, stdout_path, stderr_path):
        stdout = open(stdout_path, "w") if stdout_path not in (None, "/dev/null") else subprocess.DEVNULL
        stderr = open(stderr_path, "w") if stderr_path not in (None, "/dev/null") else subprocess.DEVNULL
        proc = subprocess.Popen(list(cmd), stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr,
                                env=env if env else None, cwd=cwd, start_new_session=True)
        self._owned[name] = proc
        return _FixtureProcessInfo(pid=proc.pid, pgid=os.getpgid(proc.pid))

    def get_owned_pids(self):
        return sorted(p.pid for p in self._owned.values())

    def get_owned_pgids(self):
        return sorted(os.getpgid(p.pid) for p in self._owned.values())

    def cleanup(self, timeout=5.0):
        for name, proc in list(self._owned.items()):
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=timeout)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=timeout)
            if proc.poll() is not None:
                del self._owned[name]

    def verify_cleanup(self):
        return [pid for pid in self.get_owned_pids() if os.path.exists("/proc/%d" % pid)]


def _selftest_ownership_experiment():
    """Run owned + foreign lifecycle against the embedded fixture.

    Returns (evidence, ok). Selftest fails if: owned survives cleanup, foreign
    is killed by supervisor cleanup, verify_cleanup() != [], or a temporary
    process remains after the test.
    """
    evidence = {}
    pids = []
    try:
        sup = _FixtureSupervisor()
        info = sup.start_process("owned", ["/bin/sleep", "30"], {}, ".", "/dev/null", "/dev/null")
        pids.append(info.pid)
        foreign = subprocess.Popen(["/bin/sleep", "60"], start_new_session=True,
                                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                                   stderr=subprocess.DEVNULL)
        pids.append(foreign.pid)
        evidence["owned_alive_before"] = os.path.exists("/proc/%d" % info.pid)
        evidence["foreign_alive_before"] = os.path.exists("/proc/%d" % foreign.pid)
        evidence["foreign_registered_in_supervisor"] = foreign.pid in sup.get_owned_pids()
        sup.cleanup(timeout=5.0)
        evidence["owned_alive_after"] = os.path.exists("/proc/%d" % info.pid)
        evidence["foreign_alive_after"] = os.path.exists("/proc/%d" % foreign.pid)
        evidence["verify_cleanup"] = sup.verify_cleanup()
        if foreign.poll() is None:
            foreign.terminate()
            try:
                foreign.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                foreign.kill()
                foreign.wait(timeout=5.0)
        evidence["foreign_terminated_by_controller"] = foreign.poll() is not None
    finally:
        evidence["remaining_children"] = [p for p in pids if os.path.exists("/proc/%d" % p)]
    ok = (evidence["owned_alive_before"] is True
          and evidence["owned_alive_after"] is False
          and evidence["foreign_alive_after"] is True
          and evidence["verify_cleanup"] == []
          and evidence["foreign_terminated_by_controller"] is True
          and evidence["remaining_children"] == [])
    return evidence, ok


# ---------------------------------------------------------------------------
# SELFTEST — prove the gates are non-vacuous
# ---------------------------------------------------------------------------
def selftest():
    res = Result()
    # 1. known-bad LAB-A placeholder rejected
    known_bad_lab_a = (
        "class ProcessSupervisor:\n"
        "    def start_process(self, name, cmd, env, cwd, stdout_path, stderr_path):\n"
        "        # We are not actually starting a process, but we must return a ProcessInfo.\n"
        "        return ProcessInfo(pid=0, pgid=0)\n"
        "    def cleanup(self, timeout):\n"
        "        pass\n"
    )
    hits_a = scan_for_patterns(known_bad_lab_a, PLACEHOLDER_PATTERNS, "selftest-a")
    res.add("selftest: LAB-A placeholder detector flags dummy ProcessInfo",
            "PASS" if hits_a else "FAIL", evidence={"hits": hits_a})

    # 2. known-bad LAB-B trivial tests rejected
    known_bad_tests = (
        "class TestAudioOracle(unittest.TestCase):\n"
        "    def test_analyze_wav_silence_fails(self):\n"
        "        pass\n"
        "class TestX(unittest.TestCase):\n"
        "    def test_can_be_imported(self):\n"
        "        self.assertTrue(True)\n"
    )
    hits_b = scan_for_patterns(known_bad_tests, TRIVIAL_TEST_PATTERNS, "selftest-b")
    res.add("selftest: LAB-B trivial-test detector flags pass/import-only",
            "PASS" if hits_b else "FAIL", evidence={"hits": hits_b})

    # 3. known-good nontrivial LAB-B test not falsely rejected
    known_good = (
        "import wave, os\n"
        "def test_silence_fails():\n"
        "    r = analyze_wav(p, 4800, 4800, 120.0, 4, 48000)\n"
        "    assert r.to_dict()['verification'] == 'FAIL'\n"
    )
    hits_good = scan_for_patterns(known_good, TRIVIAL_TEST_PATTERNS, "selftest-good")
    res.add("selftest: good test not falsely flagged",
            "PASS" if not hits_good else "FAIL", evidence={"hits": hits_good})

    # 4. v3 defective foreign-process pattern detected/rejected
    v3_hits = detect_v3_foreign_probe_pattern(V3_DEFECTIVE_PROBE_SNIPPET)
    res.add("selftest: v3 defective foreign-process pattern detected/rejected",
            "PASS" if v3_hits else "FAIL",
            evidence={"reasons": v3_hits, "snippet": V3_DEFECTIVE_PROBE_SNIPPET})

    # 5. v3.1 corrected foreign-process pattern accepted
    corrected_snippet = build_probe_script(
        "sys.path.insert(0, 'tests/integration')\n"
        "from headless_audio_lab.process_supervisor import ProcessSupervisor\n")
    corrected_hits = detect_v3_foreign_probe_pattern(corrected_snippet)
    res.add("selftest: v3.1 corrected foreign-process pattern accepted",
            "PASS" if not corrected_hits else "FAIL",
            evidence={"reasons": corrected_hits})

    # 6+7+8. controlled fixture: external survives, owned does not, all cleaned
    evidence, ok = _selftest_ownership_experiment()
    res.add("selftest: external process survives supervisor cleanup in controlled fixture",
            "PASS" if evidence["foreign_alive_after"] is True else "FAIL", evidence=evidence)
    res.add("selftest: owned process does not survive supervisor cleanup",
            "PASS" if evidence["owned_alive_after"] is False else "FAIL", evidence=evidence)
    res.add("selftest: all temporary children cleaned at end",
            "PASS" if evidence["remaining_children"] == [] else "FAIL", evidence=evidence)
    res.add("selftest: controlled fixture ownership experiment overall",
            "PASS" if ok else "FAIL", evidence=evidence)

    # 9. foreign created by supervisor.start_process is DETECTED (would break
    # the ownership semantics; the detector must flag it)
    res.add("selftest: foreign created by start_process is detected",
            "PASS" if v3_hits else "FAIL",
            evidence={"reasons": v3_hits})

    # silent WAV fixture construction (kept from v3)
    probe = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    probe.close()
    try:
        w = wave.open(probe.name, "wb")
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(48000)
        w.writeframes(b"\x00\x00" * 4800)
        w.close()
        probe_good = True
    except Exception as e:
        res.add("selftest: silent WAV construction", "FAIL", str(e))
        probe_good = False
    try:
        os.unlink(probe.name)
    except OSError:
        pass
    res.add("selftest: silent WAV fixture constructed", "PASS" if probe_good else "FAIL")
    return res


# ---------------------------------------------------------------------------
# Controlled ownership probe (corrected probe against a fixture supervisor)
# ---------------------------------------------------------------------------
def controlled_ownership_probe(probe_supervisor_path):
    res = Result()
    import_lines = (
        "import importlib.util\n"
        "_spec = importlib.util.spec_from_file_location('_probe_fixture_sup', %r)\n"
        "_mod = importlib.util.module_from_spec(_spec)\n"
        "_spec.loader.exec_module(_mod)\n"
        "ProcessSupervisor = _mod.ProcessSupervisor\n" % os.path.abspath(probe_supervisor_path)
    )
    probe_src = build_probe_script(import_lines)
    defects = detect_v3_foreign_probe_pattern(probe_src)
    res.add("controlled probe: harness probe pattern integrity",
            "PASS" if not defects else "FAIL",
            evidence={"detected": defects})
    r, d = run_probe_script(os.getcwd(), probe_src, "controlled_ownership")
    if r["exit"] != 0:
        res.fail("controlled probe: corrected LAB-A probe vs fixture",
                 "probe failed exit=%d stderr=%s" % (r["exit"], r["stderr"][-500:]))
        return res, None
    cond_results = {}
    ok_all = True
    for cond, fn in LAB_A_CONDITIONS:
        val = fn(d)
        cond_results[cond] = {"satisfied": bool(val), "observed": d.get(cond)}
        ok_all = ok_all and bool(val)
    res.add("controlled probe: corrected LAB-A probe vs fixture",
            "PASS" if ok_all else "FAIL",
            "all eight corrected conditions satisfied against the contract-conformant fixture",
            evidence=cond_results)
    return res, d


def main():
    ap = argparse.ArgumentParser(description=SCHEMA)
    ap.add_argument("--lab", choices=["a", "b"])
    ap.add_argument("--checkout", help="clean checkout of exact result head")
    ap.add_argument("--contract", help="path to lab-api-contract-v3.1.json")
    ap.add_argument("--envelope", help="path to result envelope JSON (optional but required for completion)")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--controlled-ownership-probe", action="store_true",
                    help="run the corrected LAB-A probe against a fixture supervisor")
    ap.add_argument("--probe-supervisor", help="path to fixture supervisor module (.py)")
    ap.add_argument("--json-out", help="write findings JSON to path")
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

    if args.controlled_ownership_probe:
        if not args.probe_supervisor:
            ap.error("--controlled-ownership-probe requires --probe-supervisor PATH")
        res, _d = controlled_ownership_probe(args.probe_supervisor)
        if args.json_out:
            with open(args.json_out, "w") as f:
                json.dump({"schema": SCHEMA, "version": VERSION,
                           "mode": "controlled-ownership-probe",
                           "probe_supervisor": os.path.abspath(args.probe_supervisor),
                           "checks": res.checks,
                           "verdict": "REJECT" if res.failed() else "PASS"}, f, indent=2)
        for c in res.checks:
            print("[%s] %s — %s" % (c["status"], c["name"], c["detail"]))
        print("CONTROLLED_PROBE:", "FAIL" if res.failed() else "PASS")
        return 1 if res.failed() else 0

    if not args.lab or not args.checkout:
        ap.error("--lab and --checkout are required unless --selftest")

    res = Result()
    contract = load_contract(args.contract) if args.contract else None
    if args.lab == "a":
        check_lab_a(args.checkout, contract, args.envelope, res)
        if args.envelope:
            check_envelope(args.checkout, args.envelope, "LAB-A", res)
    else:
        check_lab_b(args.checkout, contract, args.envelope, res)
        if args.envelope:
            check_envelope(args.checkout, args.envelope, "LAB-B", res)

    for note in res.env_notes:
        print("NOTE:", note)

    if args.json_out:
        with open(args.json_out, "w") as f:
            json.dump({"schema": SCHEMA, "version": VERSION,
                       "lab": args.lab.upper(), "checkout": args.checkout,
                       "checks": res.checks, "env_notes": res.env_notes,
                       "verdict": "REJECT" if res.failed() else "PASS"}, f, indent=2)
    for c in res.checks:
        print("[%s] %s — %s" % (c["status"], c["name"], c["detail"]))
    print("VERDICT:", "REJECT (one or more FAIL)" if res.failed() else "PASS")
    return 1 if res.failed() else 0


if __name__ == "__main__":
    sys.exit(main())
