#!/usr/bin/env python3
"""
Scenario runner for the virtual studio.

Runs declarative scenarios: JACK dummy + source + capture + analyze.
Supports multiple time signatures, bar counts, and signal modes.
"""

import json
import os
import subprocess
import sys
import time
from pathlib import Path

REPO = Path("/home/ubuntu/code/music/seq66-loves-sooperlooper")
VSTUDIO = REPO / "tests/integration/virtual_studio"


def log(msg):
    print(f"[scenario] {msg}", file=sys.stderr)


def run_jack(run_dir, sample_rate=48000, period_size=1024):
    """Start JACK dummy, return True if successful."""
    cmd = ["jackd", "-r", "-d", "dummy", "-r", str(sample_rate), "-p", str(period_size)]
    run_dir.mkdir(parents=True, exist_ok=True)
    stderr_path = run_dir / "process" / "jack.stderr.log"
    stderr_path.parent.mkdir(exist_ok=True)
    stderr_f = open(stderr_path, "w")
    proc = subprocess.Popen(cmd, stderr=stderr_f, preexec_fn=os.setsid)
    time.sleep(1)
    # Verify
    r = subprocess.run(["jack_lsp"], capture_output=True, text=True, timeout=3)
    return r.returncode == 0 and len(r.stdout.strip().split("\n")) >= 4


def run_source(run_dir, config):
    """Start synthetic source, return process."""
    cmd = [
        str(VSTUDIO / "audio" / "synthetic_source"),
        "--mode", config.get("mode", "beats"),
        "--tempo", str(config.get("tempo", 120)),
        "--numerator", str(config.get("numerator", 4)),
        "--denominator", str(config.get("denominator", 4)),
    ]
    stderr_path = run_dir / "process" / "source.stderr.log"
    run_dir.mkdir(parents=True, exist_ok=True)
    stderr_f = open(stderr_path, "w")
    proc = subprocess.Popen(cmd, stderr=stderr_f, preexec_fn=os.setsid)
    time.sleep(0.5)
    return proc


def run_capture(run_dir, wav_path):
    """Start capture, return process."""
    cmd = [
        str(VSTUDIO / "audio" / "deterministic_capture"),
        "--output", str(wav_path),
    ]
    stderr_path = run_dir / "process" / "capture.stderr.log"
    run_dir.mkdir(parents=True, exist_ok=True)
    stderr_f = open(stderr_path, "w")
    proc = subprocess.Popen(cmd, stderr=stderr_f, preexec_fn=os.setsid)
    time.sleep(0.5)
    return proc


def connect_ports(src_l, src_r, dst_l, dst_r):
    """Connect JACK ports."""
    connections = []
    for s, d in [(src_l, dst_l), (src_r, dst_r)]:
        if s and d:
            r = subprocess.run(["jack_connect", s, d], capture_output=True, timeout=3)
            connections.append((s, d, r.returncode == 0))
    return connections


def fix_wav_header(wav_path):
    """Fix WAV header with correct sizes."""
    import struct
    if not wav_path.exists() or wav_path.stat().st_size < 44:
        return False
    size = wav_path.stat().st_size
    with open(wav_path, "r+b") as f:
        f.seek(4)
        f.write(struct.pack("<I", size - 8))
        f.seek(40)
        f.write(struct.pack("<I", size - 44))
    return True


def analyze_wav(wav_path):
    """Run audio analyzer, return result dict."""
    cmd = [sys.executable, str(VSTUDIO / "analyzers" / "audio_analyzer.py"), str(wav_path)]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
    if r.returncode == 0:
        try:
            return json.loads(r.stdout)
        except json.JSONDecodeError:
            return {"verification": "PARSE_ERROR"}
    return {"verification": "ANALYZER_ERROR", "error": r.stderr[:200]}


def stop_process(proc):
    """Stop a process gracefully."""
    if proc and proc.poll() is None:
        try:
            os.killpg(os.getpgid(proc.pid), 15)  # SIGTERM
            proc.wait(timeout=3)
        except Exception:
            try:
                os.killpg(os.getpgid(proc.pid), 9)  # SIGKILL
            except Exception:
                pass


def run_scenario(scenario):
    """Run a single scenario, return result dict."""
    name = scenario.get("name", "unnamed")
    run_dir = VSTUDIO / "runs" / name
    run_dir.mkdir(parents=True, exist_ok=True)

    sr = scenario.get("sample_rate", 48000)
    ps = scenario.get("period_size", 1024)
    duration = scenario.get("duration_seconds", 2)

    log(f"=== {name} ===")
    log(f"  {scenario.get('numerator',4)}/{scenario.get('denominator',4)} @ {scenario.get('tempo',120)} BPM")

    # Start JACK
    if not run_jack(run_dir, sr, ps):
        return {"name": name, "verification": "FAIL", "error": "JACK failed"}

    # Start source
    src_proc = run_source(run_dir, scenario)

    # Wait for source to register
    time.sleep(0.5)

    # Get port names
    r = subprocess.run(["jack_lsp"], capture_output=True, text=True, timeout=3)
    ports = r.stdout.strip().split("\n") if r.returncode == 0 else []
    src_l = next((p for p in ports if "synthetic_source:out_l" in p), None)
    src_r = next((p for p in ports if "synthetic_source:out_r" in p), None)

    # Start capture
    wav_path = run_dir / "audio" / "captured.wav"
    wav_path.parent.mkdir(exist_ok=True)
    cap_proc = run_capture(run_dir, wav_path)

    # Wait for capture to register
    time.sleep(0.5)

    # Get capture ports
    r = subprocess.run(["jack_lsp"], capture_output=True, text=True, timeout=3)
    ports = r.stdout.strip().split("\n") if r.returncode == 0 else []
    cap_l = next((p for p in ports if "deterministic_capture:in_l" in p), None)
    cap_r = next((p for p in ports if "deterministic_capture:in_r" in p), None)

    # Connect source -> capture (direct mode)
    conns = connect_ports(src_l, src_r, cap_l, cap_r)

    # Record
    log(f"  Recording {duration}s...")
    time.sleep(duration)

    # Stop
    stop_process(cap_proc)
    stop_process(src_proc)
    time.sleep(1)

    # Fix WAV header
    fix_wav_header(wav_path)

    # Analyze
    analysis = analyze_wav(wav_path)

    # Write trace
    trace = {
        "scenario": name,
        "config": scenario,
        "connections": [(s, d, ok) for s, d, ok in conns],
        "analysis": analysis,
    }
    with open(run_dir / "result.json", "w") as f:
        json.dump(trace, f, indent=2)

    verification = analysis.get("verification", "UNKNOWN")
    log(f"  Result: {verification}")

    return {
        "name": name,
        "verification": verification,
        "analysis": analysis,
        "run_dir": str(run_dir),
    }


# Catalog of scenarios
CATALOG = [
    # 4/4
    {"name": "4-4-1bar", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 2, "mode": "beats"},
    {"name": "4-4-2bar", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 3, "mode": "beats"},
    {"name": "4-4-4bar", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 5, "mode": "beats"},
    {"name": "4-4-8bar", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 9, "mode": "beats"},
    # 3/4
    {"name": "3-4-1bar", "numerator": 3, "denominator": 4, "tempo": 120, "duration_seconds": 2, "mode": "beats"},
    {"name": "3-4-2bar", "numerator": 3, "denominator": 4, "tempo": 120, "duration_seconds": 3, "mode": "beats"},
    {"name": "3-4-4bar", "numerator": 3, "denominator": 4, "tempo": 120, "duration_seconds": 5, "mode": "beats"},
    {"name": "3-4-8bar", "numerator": 3, "denominator": 4, "tempo": 120, "duration_seconds": 9, "mode": "beats"},
    # 5/4
    {"name": "5-4-1bar", "numerator": 5, "denominator": 4, "tempo": 120, "duration_seconds": 2, "mode": "beats"},
    {"name": "5-4-2bar", "numerator": 5, "denominator": 4, "tempo": 120, "duration_seconds": 3, "mode": "beats"},
    {"name": "5-4-4bar", "numerator": 5, "denominator": 4, "tempo": 120, "duration_seconds": 5, "mode": "beats"},
    {"name": "5-4-8bar", "numerator": 5, "denominator": 4, "tempo": 120, "duration_seconds": 9, "mode": "beats"},
    # 6/8
    {"name": "6-8-1bar", "numerator": 6, "denominator": 8, "tempo": 120, "duration_seconds": 2, "mode": "beats"},
    {"name": "6-8-2bar", "numerator": 6, "denominator": 8, "tempo": 120, "duration_seconds": 3, "mode": "beats"},
    {"name": "6-8-4bar", "numerator": 6, "denominator": 8, "tempo": 120, "duration_seconds": 5, "mode": "beats"},
    {"name": "6-8-8bar", "numerator": 6, "denominator": 8, "tempo": 120, "duration_seconds": 9, "mode": "beats"},
    # 7/8
    {"name": "7-8-1bar", "numerator": 7, "denominator": 8, "tempo": 120, "duration_seconds": 2, "mode": "beats"},
    {"name": "7-8-2bar", "numerator": 7, "denominator": 8, "tempo": 120, "duration_seconds": 3, "mode": "beats"},
    {"name": "7-8-4bar", "numerator": 7, "denominator": 8, "tempo": 120, "duration_seconds": 5, "mode": "beats"},
    {"name": "7-8-8bar", "numerator": 7, "denominator": 8, "tempo": 120, "duration_seconds": 9, "mode": "beats"},
    # Signal modes
    {"name": "sine-4-4", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 2, "mode": "sine"},
    {"name": "noise-4-4", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 2, "mode": "noise"},
    {"name": "chirp-4-4", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 2, "mode": "chirp"},
    {"name": "markers-4-4", "numerator": 4, "denominator": 4, "tempo": 120, "duration_seconds": 2, "mode": "markers"},
]


def main():
    if len(sys.argv) > 1:
        # Run specific scenarios
        names = sys.argv[1:]
        catalog = [s for s in CATALOG if s["name"] in names]
        if not catalog:
            print(f"No matching scenarios for: {names}")
            sys.exit(1)
    else:
        # Run all
        catalog = CATALOG

    results = []
    for scenario in catalog:
        # Clean up between scenarios
        for name in ["jackd"]:
            r = subprocess.run(["pgrep", "-f", name], capture_output=True, text=True)
            for pid in r.stdout.strip().split("\n"):
                if pid:
                    try:
                        os.kill(int(pid), 9)
                    except Exception:
                        pass
        time.sleep(0.5)

        result = run_scenario(scenario)
        results.append(result)

    # Summary
    passed = sum(1 for r in results if r["verification"] == "PASS")
    total = len(results)
    print(f"\n=== {passed}/{total} scenarios passed ===")

    # Write summary
    summary_path = VSTUDIO / "runs" / "catalog-summary.json"
    with open(summary_path, "w") as f:
        json.dump({
            "total": total,
            "passed": passed,
            "failed": total - passed,
            "results": [{"name": r["name"], "verification": r["verification"]} for r in results],
        }, f, indent=2)

    sys.exit(0 if passed == total else 1)


if __name__ == "__main__":
    main()
