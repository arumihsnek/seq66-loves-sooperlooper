#!/usr/bin/env python3
"""
Environment probe for the virtual studio.

Detects and records the execution environment in machine-readable JSON.
Produces a profile that subsequent components consume.
"""

import json
import os
import platform
import subprocess
import sys
from pathlib import Path
from datetime import datetime, timezone


def run(cmd, timeout=5):
    """Run a command safely, return (stdout, stderr, exit_code)."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return r.stdout.strip(), r.stderr.strip(), r.returncode
    except FileNotFoundError:
        return "", "command not found", -1
    except subprocess.TimeoutExpired:
        return "", "timeout", -2


def detect_jack():
    """Detect JACK server capabilities."""
    info = {
        "available": False,
        "version": None,
        "dummy_backend": False,
        "dummy_options": {},
        "realtime": False,
        "sample_rates_tested": [],
        "period_sizes_tested": [],
    }

    out, err, rc = run(["jackd", "--version"])
    if rc != 0:
        return info

    info["available"] = True
    info["version"] = out.split("\n")[0] if out else None

    # Check dummy backend
    out, err, rc = run(["jackd", "-d", "dummy", "--help"])
    combined = out + err
    if "dummy" in combined.lower() or "jackdmp" in combined.lower():
        info["dummy_backend"] = True
        for line in combined.split("\n"):
            if "--rate" in line:
                info["dummy_options"]["rate"] = True
            if "--period" in line:
                info["dummy_options"]["period"] = True
            if "--capture" in line:
                info["dummy_options"]["capture"] = True
            if "--playback" in line:
                info["dummy_options"]["playback"] = True

    # Test actual dummy start
    for rate in [44100, 48000, 96000]:
        out, err, rc = run(
            ["timeout", "2", "jackd", "-r", "-d", "dummy", "-r", str(rate), "-p", "1024"],
            timeout=3
        )
        if "no message buffer" in err or "started" in err.lower() or rc in (0, 124, 137):
            info["sample_rates_tested"].append(rate)

    for period in [64, 128, 256, 512, 1024, 2048]:
        out, err, rc = run(
            ["timeout", "2", "jackd", "-r", "-d", "dummy", "-r", "48000", "-p", str(period)],
            timeout=3
        )
        if "no message buffer" in err or "started" in err.lower() or rc in (0, 124, 137):
            info["period_sizes_tested"].append(period)

    return info


def detect_sooperlooper():
    """Detect SooperLooper."""
    info = {
        "available": False,
        "version": None,
        "binary": None,
        "can_headless": False,
    }

    for binary in ["sooperlooper", "sooperlooper-server"]:
        out, err, rc = run([binary, "--version"])
        if rc in (0, 1):
            info["available"] = True
            info["binary"] = binary
            text = out or err
            for line in text.split("\n"):
                if "SooperLooper" in line or "sooperlooper" in line.lower():
                    info["version"] = line.strip()
                    break
            if not info["version"]:
                info["version"] = text.split("\n")[0] if text else "unknown"
            break

    if info["available"]:
        # Check if it can run headless (no display needed)
        out, err, rc = run(["timeout", "2", info["binary"], "--help"])
        if "--jack-name" in (out + err):
            info["can_headless"] = True

    return info


def detect_seq66():
    """Detect Seq66 build status."""
    info = {
        "source_available": False,
        "build_available": False,
        "binary": None,
        "build_system": None,
    }

    repo = Path("/home/ubuntu/code/music/seq66-loves-sooperlooper")
    if (repo / "meson.build").exists():
        info["source_available"] = True
        info["build_system"] = "meson"
    if (repo / "seq66.pro").exists():
        info["source_available"] = True
        info["build_system"] = "qmake"

    # Check for built binary
    for pattern in ["seq66", "seq66cli/seq66cli", "Seq66cli/seq66cli"]:
        p = repo / pattern
        if p.exists() and os.access(p, os.X_OK):
            info["build_available"] = True
            info["binary"] = str(p)
            break

    return info


def detect_liblo():
    """Detect liblo (OSC library)."""
    info = {
        "available": False,
        "version": None,
        "headers": False,
    }

    out, err, rc = run(["pkg-config", "--modversion", "liblo"])
    if rc == 0 and out:
        info["available"] = True
        info["version"] = out

    info["headers"] = Path("/usr/include/lo/lo.h").exists()
    return info


def detect_alsa():
    """Detect ALSA sequencer."""
    info = {
        "available": False,
        "sequencer_device": False,
        "aconnect": False,
    }

    info["sequencer_device"] = Path("/dev/snd/seq").exists()
    info["aconnect"] = bool(run(["which", "aconnect"])[0])

    out, err, rc = run(["aplay", "--version"])
    if rc == 0:
        info["available"] = True

    return info


def detect_pipewire():
    """Detect PipeWire."""
    info = {
        "available": False,
        "version": None,
        "jack_compatible": False,
    }

    out, err, rc = run(["pipewire", "--version"])
    if rc == 0 or "pipewire" in out.lower():
        info["available"] = True
        for line in (out + err).split("\n"):
            if "pipewire" in line.lower() and any(c.isdigit() for c in line):
                info["version"] = line.strip()
                break

    out, err, rc = run(["pw-jack", "--help"])
    info["jack_compatible"] = rc == 0 or "pw-jack" in out.lower()

    return info


def detect_realtime():
    """Detect realtime capabilities."""
    info = {
        "has_realtime_group": False,
        "mlock_available": False,
        "memlock_limit": None,
        "uid": os.getuid(),
    }

    # Check realtime group
    out, err, rc = run(["id", "-Gn"])
    if "audio" in out or "realtime" in out:
        info["has_realtime_group"] = True

    # Check memlock limit
    out, err, rc = run(["ulimit", "-l"])
    if rc == 0:
        info["memlock_limit"] = out

    # Check mlock capability
    try:
        import resource
        info["mlock_available"] = True
    except ImportError:
        pass

    return info


def detect_tools():
    """Detect optional tools."""
    tools = {}
    for name, cmd in [
        ("sox", ["sox", "--version"]),
        ("ffmpeg", ["ffmpeg", "-version"]),
        ("jack_lsp", ["jack_lsp", "--help"]),
        ("jack_connect", ["jack_connect", "--help"]),
        ("jack_disconnect", ["jack_disconnect", "--help"]),
        ("jack_bufsize", ["jack_bufsize", "--help"]),
        ("cmake", ["cmake", "--version"]),
        ("python3", ["python3", "--version"]),
        ("pkg_config", ["pkg-config", "--version"]),
    ]:
        out, err, rc = run(cmd, timeout=3)
        tools[name] = rc == 0 or rc == 1  # some tools exit 1 on --help

    return tools


def detect_conflicts():
    """Detect existing JACK servers or processes that could conflict."""
    info = {
        "existing_jack_servers": 0,
        "existing_sooperlooper": 0,
        "shm_segments": [],
    }

    out, err, rc = run(["pgrep", "-a", "jackd"])
    if out:
        info["existing_jack_servers"] = len(out.strip().split("\n"))

    out, err, rc = run(["pgrep", "-a", "sooperlooper"])
    if out:
        info["existing_sooperlooper"] = len(out.strip().split("\n"))

    # Check /dev/shm for JACK segments
    shm = Path("/dev/shm")
    if shm.exists():
        for f in shm.iterdir():
            if "jack" in f.name.lower():
                info["shm_segments"].append(f.name)

    return info


def main():
    probe = {
        "probe_version": "1.0.0",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "system": {
            "os": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
            "python": platform.python_version(),
            "hostname": platform.node(),
        },
        "jack": detect_jack(),
        "sooperlooper": detect_sooperlooper(),
        "seq66": detect_seq66(),
        "liblo": detect_liblo(),
        "alsa": detect_alsa(),
        "pipewire": detect_pipewire(),
        "realtime": detect_realtime(),
        "tools": detect_tools(),
        "conflicts": detect_conflicts(),
    }

    # Determine profile
    if probe["jack"]["available"] and probe["jack"]["dummy_backend"]:
        if probe["sooperlooper"]["available"]:
            probe["profile"] = "oci-jack-dummy"
        else:
            probe["profile"] = "oci-jack-dummy-no-sl"
    else:
        probe["profile"] = "oci-no-jack"

    # Write output
    outdir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(".")
    outdir.mkdir(parents=True, exist_ok=True)
    outfile = outdir / "environment.json"

    with open(outfile, "w") as f:
        json.dump(probe, f, indent=2)

    print(json.dumps(probe, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
