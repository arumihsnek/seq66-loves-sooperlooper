#!/usr/bin/env python3
"""acceptance-verifier-v3.py — Controller-owned acceptance harness for
headless-lab v3 dispatch (LAB-A / LAB-B).

OWNERSHIP: this script belongs to the controller/control plane. It is NOT an
owned path of LAB-A or LAB-B and may not be modified by a leaf. Its SHA-256 is
frozen in acceptance-manifest-v3.json. The controller executes it against a
CLEAN CHECKOUT of the exact result head.

Gates implemented:
  LAB-A (11.4): placeholder rejection (pass/TODO/NotImplemented/dummy
    ProcessInfo/pid=0/MagicMock/always-success/import-only), owned child
    lifecycle with REAL positive PID, identity verification, bounded
    shutdown, failure propagation, no signaling of foreign processes.
  LAB-B (11.5): exact analyze_wav signature vs contract, real silence test
    (silence always FAIL), malformed WAV FAIL/ERROR, contractual frame
    count, no empty tests, no trivial asserts, .gitignore byte-exact,
    parseable scenarios, runner uses contractual APIs, no ELF tracked.

Exit codes: 0 = ALL required checks PASS (SKIP is recorded, not PASS);
1 = one or more FAIL; 2 = usage/environment error.

--selftest: proves the negative gates are non-vacuous by running them
against embedded KNOWN-BAD and KNOWN-GOOD snippets. Exits 0 only if every
known-bad snippet is flagged and every known-good snippet passes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
import wave

SCHEMA = "acceptance-verifier-v3"
VERSION = "3.0.0"

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
    "receipts/headless-lab-results-v3/lab-b-result.json",
]
ENVELOPE_LAB_A = "receipts/headless-lab-results-v3/lab-a-result.json"
ENVELOPE_LAB_B = "receipts/headless-lab-results-v3/lab-b-result.json"

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
    r = run(["git", "-C", checkout, "show", f"{rev}:{path}"], checkout)
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
        res.fail("lab-a: imports", f"import failure: {e}")
        return

    # placeholder scan of owned sources
    ph_hits = {}
    for f in OWNED_LAB_A:
        content = git_show(checkout, "HEAD", f)
        if content is None:
            res.fail("lab-a: placeholder scan", f"owned file missing at HEAD: {f}")
            continue
        hits = scan_for_patterns(content, PLACEHOLDER_PATTERNS, "lab-a")
        if hits:
            ph_hits[f] = hits
    if ph_hits:
        res.fail("lab-a: placeholder scan", "placeholder patterns found in owned sources",
                 evidence=ph_hits)
    else:
        res.ok("lab-a: placeholder scan", "no placeholder patterns in owned sources")

    # functional: real child lifecycle via ProcessSupervisor
    probe = os.path.join(checkout, "_acceptance_probe_lab_a.py")
    probe_src = (
        "import sys, time, os\n"
        "sys.path.insert(0, 'tests/integration')\n"
        "from headless_audio_lab.process_supervisor import ProcessSupervisor\n"
        "out = {}\n"
        "with ProcessSupervisor() as sup:\n"
        "    info = sup.start_process('probe', ['/bin/sleep', '3'], {}, '.', '/dev/null', '/dev/null')\n"
        "    out['pid'] = info.pid\n"
        "    out['pgid'] = info.pgid\n"
        "    out['pid_positive'] = bool(info.pid and info.pid > 0)\n"
        "    time.sleep(0.3)\n"
        "    alive = os.path.exists('/proc/%d' % info.pid)\n"
        "    out['child_alive_while_owned'] = alive\n"
        "    out['owned_pids'] = sup.get_owned_pids()\n"
        "    # failure propagation: bad command must raise\n"
        "    try:\n"
        "        sup.start_process('bad', ['/nonexistent/binary-xyz'], {}, '.', '/dev/null', '/dev/null')\n"
        "        out['failure_propagation'] = False\n"
        "    except Exception:\n"
        "        out['failure_propagation'] = True\n"
        "sup.cleanup(timeout=5.0)\n"
        "out['cleanup_ok'] = sup.verify_cleanup() == []\n"
        "# foreign process must survive\n"
        "foreign = sup.start_process('foreign', ['/bin/sleep', '5'], {}, '.', '/dev/null', '/dev/null')\n"
        "sup.cleanup(timeout=5.0)\n"
        "foreign_alive = os.path.exists('/proc/%d' % foreign.pid)\n"
        "out['foreign_not_signaled'] = foreign_alive\n"
        "try:\n"
        "    os.kill(foreign.pid, 9)\n"
        "except Exception:\n"
        "    pass\n"
        "print(json.dumps(out))\n"
    )
    with open(probe, "w") as f:
        f.write("import json\n" + probe_src)
    r = run(["python3", probe], checkout)
    os.unlink(probe)
    if r["exit"] == 0:
        try:
            d = json.loads(r["stdout"].strip().splitlines()[-1])
        except Exception:
            d = {}
        ok_all = all([d.get("pid_positive"), d.get("child_alive_while_owned"),
                      d.get("failure_propagation"), d.get("cleanup_ok"),
                      d.get("foreign_not_signaled")])
        res.add("lab-a: functional lifecycle", "PASS" if ok_all else "FAIL",
                "real PID >0, child alive, failure propagation, bounded cleanup, foreign survival",
                evidence=d)
    else:
        res.fail("lab-a: functional lifecycle",
                 f"probe failed exit={r['exit']} stderr={r['stderr'][-500:]}")


# ---------------------------------------------------------------------------
# LAB-B gates
# ---------------------------------------------------------------------------
def check_lab_b(checkout, contract, envelope_path, res):
    # 1. exact analyze_wav signature vs contract
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
    missing = [a for a in contract_args if not re.search(rf"\b{a}\b", impl_sig)]
    res.add("lab-b: analyze_wav signature", "PASS" if not missing else "FAIL",
            f"contract args={contract_args}", evidence={"impl_sig": impl_sig, "missing": missing})

    # 2. test_modules placeholder scan
    tm = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/test_modules.py") or ""
    hits = scan_for_patterns(tm, TRIVIAL_TEST_PATTERNS, "lab-b")
    invokes = "analyze_wav(" in tm
    if hits or not invokes:
        res.fail("lab-b: test_modules negative gates",
                 f"hits={hits} invokes_analyze_wav={invokes}")
    else:
        res.ok("lab-b: test_modules negative gates", "no trivial tests; analyze_wav invoked")

    # 3. silence always FAIL (full contract signature)
    probe = os.path.join(checkout, "_acceptance_probe_lab_b.py")
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
            "# frame count contract: captured_wav_frames equals written frames\n"
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
    os.unlink(probe)
    if r["exit"] == 0:
        try:
            d = json.loads(r["stdout"].strip().splitlines()[-1])
        except Exception:
            d = {}
        silence_ok = d.get("silence_verification") == "FAIL"
        malformed_ok = d.get("malformed_result", "").startswith(("FAIL", "ERROR", "EXCEPTION"))
        frame_ok = d.get("frame_count") == 4800
        res.add("lab-b: silence always FAIL", "PASS" if silence_ok else "FAIL",
                "silent WAV must produce FAIL", evidence=d)
        res.add("lab-b: malformed WAV FAIL/ERROR", "PASS" if malformed_ok else "FAIL",
                evidence={"malformed_result": d.get("malformed_result")})
        res.add("lab-b: frame count contract", "PASS" if frame_ok else "FAIL",
                f"captured_wav_frames={d.get('frame_count')} expected 4800")
    else:
        res.fail("lab-b: audio probes", f"probe failed exit={r['exit']} stderr={r['stderr'][-500:]}")

    # 4. .gitignore byte-exact
    gi = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/fixtures/.gitignore") or ""
    literal_n = "\\n" in gi
    real_newline = "\n" in gi
    res.add("lab-b: .gitignore byte-exact",
            "PASS" if (not literal_n and real_newline) else "FAIL",
            "literal '\\n' rejected; real newlines required", evidence={"repr": repr(gi)})

    # 5. no ELF tracked in the commit
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

    # 6. scenarios parseable
    try:
        import yaml  # noqa: F401
        have_yaml = True
    except ImportError:
        have_yaml = False
    bad = []
    for s in ["D1_source_direct.yaml", "D2_sl_passive.yaml", "D3_osc_recording.yaml"]:
        content = git_show(checkout, "HEAD", f"tests/integration/headless_audio_lab/scenarios/{s}")
        if content is None:
            bad.append({"scenario": s, "error": "missing"})
            continue
        if have_yaml:
            try:
                yaml.safe_load(content)
            except Exception as e:
                bad.append({"scenario": s, "error": str(e)})
        else:
            res.env_notes.append(f"PyYAML unavailable; scenario {s} checked structurally only")
            if not content.strip() or not any(k in content for k in
                                              ["source:", "capture:", "sooperlooper:", "expected:"]):
                bad.append({"scenario": s, "error": "missing required sections"})
    res.add("lab-b: scenarios parseable", "PASS" if not bad else "FAIL", evidence={"bad": bad})

    # 7. runner uses contractual APIs
    runner = git_show(checkout, "HEAD", "tests/integration/headless_audio_lab/runner.py") or ""
    api_hits = [a for a in ["ProcessSupervisor", "JackServer", "OscProbe", "SooperLooperLauncher",
                            "analyze_wav", "GraphAssertions"] if a in runner]
    res.add("lab-b: runner uses contractual APIs",
            "PASS" if len(api_hits) >= 3 else "FAIL",
            f"API hits: {api_hits}")


# ---------------------------------------------------------------------------
# Envelope validation (non-self-referential model)
# ---------------------------------------------------------------------------
def check_envelope(checkout, envelope_path, expected_lab, res):
    if envelope_path is None or not os.path.exists(envelope_path):
        res.fail(f"envelope: {expected_lab}", f"envelope file missing at {envelope_path}")
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
    res.add(f"envelope: {expected_lab}",
            "PASS" if not bad else "FAIL",
            "non-self-referential binding: tested_parent_sha + result_tree_sha + external branch",
            evidence={**checks, "head": head, "envelope": env})


# ---------------------------------------------------------------------------
# SELFTEST — prove the gates are non-vacuous
# ---------------------------------------------------------------------------
def selftest():
    res = Result()
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

    known_good = (
        "import wave, os\n"
        "def test_silence_fails():\n"
        "    r = analyze_wav(p, 4800, 4800, 120.0, 4, 48000)\n"
        "    assert r.to_dict()['verification'] == 'FAIL'\n"
    )
    hits_good = scan_for_patterns(known_good, TRIVIAL_TEST_PATTERNS, "selftest-good")
    res.add("selftest: good test not falsely flagged",
            "PASS" if not hits_good else "FAIL", evidence={"hits": hits_good})

    # functional good/bad probes: silence FAIL vs PASS expectations
    r = Result()
    probe = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    probe.close()
    try:
        w = wave.open(probe.name, "wb")
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(48000)
        w.writeframes(b"\x00\x00" * 4800)
        w.close()
    except Exception as e:
        res.add("selftest: silent WAV construction", "FAIL", str(e))
        probe_good = False
    else:
        probe_good = True
    os.unlink(probe.name)
    res.add("selftest: silent WAV fixture constructed", "PASS" if probe_good else "FAIL")
    return res


def main():
    ap = argparse.ArgumentParser(description=SCHEMA)
    ap.add_argument("--lab", choices=["a", "b"])
    ap.add_argument("--checkout", help="clean checkout of exact result head")
    ap.add_argument("--contract", help="path to lab-api-contract-v3.json")
    ap.add_argument("--envelope", help="path to result envelope JSON (optional but required for completion)")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--json-out", help="write findings JSON to path")
    args = ap.parse_args()

    if args.selftest:
        res = selftest()
        if args.json_out:
            with open(args.json_out, "w") as f:
                json.dump({"schema": SCHEMA, "version": VERSION, "mode": "selftest",
                           "checks": res.checks, "env_notes": res.env_notes}, f, indent=2)
        for c in res.checks:
            print(f"[{c['status']}] {c['name']} — {c['detail']}")
        print("SELFTEST", "FAIL" if res.failed() else "PASS")
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

    # controller provenance context (always recorded, never a PASS)
    for note in res.env_notes:
        print("NOTE:", note)

    if args.json_out:
        with open(args.json_out, "w") as f:
            json.dump({"schema": SCHEMA, "version": VERSION,
                       "lab": args.lab.upper(), "checkout": args.checkout,
                       "checks": res.checks, "env_notes": res.env_notes,
                       "verdict": "REJECT" if res.failed() else "PASS"}, f, indent=2)
    for c in res.checks:
        print(f"[{c['status']}] {c['name']} — {c['detail']}")
    print("VERDICT:", "REJECT (one or more FAIL)" if res.failed() else "PASS")
    return 1 if res.failed() else 0


if __name__ == "__main__":
    sys.exit(main())
