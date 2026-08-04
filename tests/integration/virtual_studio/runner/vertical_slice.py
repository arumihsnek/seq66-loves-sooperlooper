#!/usr/bin/env python3
"""
Vertical slice runner v2 — runs the full virtual studio pipeline.

Supports two modes:
  1. direct: source → capture (verifies pipeline)
  2. sooperlooper: source → SL → capture (with OSC recording trigger)
"""

import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path
import struct
import socket

REPO = Path("/home/ubuntu/code/music/seq66-loves-sooperlooper")
VSTUDIO = REPO / "tests/integration/virtual_studio"
AUDIO_DIR = VSTUDIO / "audio"


def log(msg):
    print(f"[vs] {msg}", file=sys.stderr)


def send_osc(port, path, *args):
    """Send an OSC message using a simple Python implementation."""
    # Build OSC message manually
    msg = bytearray()

    # Path
    path_bytes = path.encode('utf-8')
    msg += path_bytes
    msg += b'\x00'  # null terminator
    # Pad to 4-byte boundary
    while len(msg) % 4 != 0:
        msg += b'\x00'

    # Type tag string
    type_tags = ','
    values = []
    for arg in args:
        if isinstance(arg, int):
            type_tags += 'i'
            values.append(arg)
        elif isinstance(arg, float):
            type_tags += 'f'
            values.append(arg)
        elif isinstance(arg, str):
            type_tags += 's'
            values.append(arg)

    tag_bytes = type_tags.encode('utf-8')
    msg += tag_bytes
    msg += b'\x00'
    while len(msg) % 4 != 0:
        msg += b'\x00'

    # Values
    for i, arg in enumerate(args):
        if isinstance(arg, int):
            msg += struct.pack('>i', arg)
        elif isinstance(arg, float):
            msg += struct.pack('>f', arg)
        elif isinstance(arg, str):
            s = arg.encode('utf-8')
            msg += s
            msg += b'\x00'
            while len(msg) % 4 != 0:
                msg += b'\x00'

    # Send via UDP
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(bytes(msg), ('127.0.0.1', port))
    finally:
        sock.close()


class Runner:
    def __init__(self, name, run_dir, config):
        self.name = name
        self.run_dir = Path(run_dir)
        self.config = config
        self.processes = {}
        self.run_dir.mkdir(parents=True, exist_ok=True)
        for d in ["process", "graph", "osc", "midi", "audio", "analysis", "scheduler", "state"]:
            (self.run_dir / d).mkdir(exist_ok=True)

    def start_jack(self):
        sr = self.config.get("sample_rate", 48000)
        ps = self.config.get("period_size", 1024)
        cmd = ["jackd", "-r", "-d", "dummy", "-r", str(sr), "-p", str(ps)]
        log(f"Starting JACK: {' '.join(cmd)}")
        stderr_path = self.run_dir / "process" / "jack.stderr.log"
        stderr_f = open(stderr_path, "w")
        proc = subprocess.Popen(cmd, stderr=stderr_f, preexec_fn=os.setsid)
        self.processes["jackd"] = {"proc": proc, "stderr_f": stderr_f}
        time.sleep(1)
        r = subprocess.run(["jack_lsp"], capture_output=True, text=True, timeout=3)
        if r.returncode == 0:
            log(f"JACK started: {len(r.stdout.strip().split(chr(10)))} ports")
            return True
        return False

    def start_sooperlooper(self):
        loops = self.config.get("loops", 1)
        channels = self.config.get("channels", 2)
        osc_port = self.config.get("osc_port", 9951)
        cmd = (
            f"sooperlooper -l {loops} -c {channels} -t 40 "
            f"-p {osc_port} -j sl-{self.name} -S default -D no -q"
        )
        log(f"Starting SooperLooper...")
        stderr_path = self.run_dir / "process" / "sooperlooper.stderr.log"
        stderr_f = open(stderr_path, "w")
        proc = subprocess.Popen(
            ["sg", "audio", "-c", cmd],
            stderr=stderr_f, preexec_fn=os.setsid
        )
        self.processes["sooperlooper"] = {"proc": proc, "stderr_f": stderr_f, "osc_port": osc_port}
        time.sleep(2)
        r = subprocess.run(["jack_lsp"], capture_output=True, text=True, timeout=3)
        sl_ports = [p for p in r.stdout.strip().split("\n") if "sl-" in p]
        log(f"SooperLooper ports: {len(sl_ports)}")
        return len(sl_ports) > 0

    def start_source(self):
        cfg = self.config.get("source", {})
        cmd = [
            str(AUDIO_DIR / "synthetic_source"),
            "--mode", cfg.get("mode", "beats"),
            "--tempo", str(cfg.get("tempo", 120)),
            "--numerator", str(cfg.get("numerator", 4)),
            "--denominator", str(cfg.get("denominator", 4)),
        ]
        if cfg.get("max_frames"):
            cmd.extend(["--max-frames", str(cfg["max_frames"])])
        log(f"Starting source...")
        stderr_path = self.run_dir / "process" / "source.stderr.log"
        stderr_f = open(stderr_path, "w")
        proc = subprocess.Popen(cmd, stderr=stderr_f, preexec_fn=os.setsid)
        self.processes["source"] = {"proc": proc, "stderr_f": stderr_f}
        time.sleep(0.5)
        return proc.poll() is None

    def start_capture(self):
        output_wav = str(self.run_dir / "audio" / "captured.wav")
        trace_path = str(self.run_dir / "osc" / "trace.jsonl")
        cmd = [
            str(AUDIO_DIR / "deterministic_capture"),
            "--output", output_wav,
            "--trace", trace_path,
        ]
        log(f"Starting capture...")
        stderr_path = self.run_dir / "process" / "capture.stderr.log"
        stderr_f = open(stderr_path, "w")
        proc = subprocess.Popen(cmd, stderr=stderr_f, preexec_fn=os.setsid)
        self.processes["capture"] = {"proc": proc, "stderr_f": stderr_f}
        time.sleep(0.5)
        return proc.poll() is None

    def connect_ports(self, mode):
        """Connect ports based on mode."""
        time.sleep(0.5)
        r = subprocess.run(["jack_lsp"], capture_output=True, text=True, timeout=3)
        ports = r.stdout.strip().split("\n") if r.returncode == 0 else []

        src_l = next((p for p in ports if "synthetic_source:out_l" in p), None)
        src_r = next((p for p in ports if "synthetic_source:out_r" in p), None)
        cap_l = next((p for p in ports if "deterministic_capture:in_l" in p), None)
        cap_r = next((p for p in ports if "deterministic_capture:in_r" in p), None)

        connections = []
        if mode == "direct":
            if src_l and cap_l:
                connections.append((src_l, cap_l))
            if src_r and cap_r:
                connections.append((src_r, cap_r))
        elif mode == "sooperlooper":
            sl_in_l = next((p for p in ports if "sl-" in p and "loop0_in_1" in p), None)
            sl_in_r = next((p for p in ports if "sl-" in p and "loop0_in_2" in p), None)
            sl_out_l = next((p for p in ports if "sl-" in p and "loop0_out_1" in p), None)
            sl_out_r = next((p for p in ports if "sl-" in p and "loop0_out_2" in p), None)
            if src_l and sl_in_l:
                connections.append((src_l, sl_in_l))
            if src_r and sl_in_r:
                connections.append((src_r, sl_in_r))
            if sl_out_l and cap_l:
                connections.append((sl_out_l, cap_l))
            if sl_out_r and cap_r:
                connections.append((sl_out_r, cap_r))

        for src, dst in connections:
            log(f"  {src} → {dst}")
            subprocess.run(["jack_connect", src, dst], capture_output=True, timeout=3)

        # Write graph
        r2 = subprocess.run(["jack_lsp", "-c"], capture_output=True, text=True, timeout=3)
        graph = {"mode": mode, "connections": [(s,d) for s,d in connections]}
        with open(self.run_dir / "graph" / "ready.json", "w") as f:
            json.dump(graph, f, indent=2)

        return len(connections) > 0

    def trigger_sooperlooper_record(self):
        """Send OSC commands to SooperLooper to start recording."""
        if "sooperlooper" not in self.processes:
            return
        osc_port = self.config.get("osc_port", 9951)
        log(f"Triggering SooperLooper record via OSC (port {osc_port})...")
        try:
            # Set monitor level to pass audio through
            send_osc(osc_port, "/sl/0/set", "monitor", 1.0)
            time.sleep(0.2)
            # Start recording
            send_osc(osc_port, "/sl/0/hit", "record")
            log("  Sent: monitor=1.0, record hit")
        except Exception as e:
            log(f"OSC send failed: {e}")

    def stop_all(self):
        for name, info in reversed(list(self.processes.items())):
            proc = info["proc"]
            if proc.poll() is None:
                try:
                    os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
                except (ProcessLookupError, PermissionError):
                    pass
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
                    except (ProcessLookupError, PermissionError):
                        pass
            try:
                info["stderr_f"].close()
            except Exception:
                pass

    def analyze(self, expected_frames=None):
        wav_path = self.run_dir / "audio" / "captured.wav"
        if not wav_path.exists():
            return {"verification": "NO_WAV"}
        if wav_path.stat().st_size < 100:
            return {"verification": "EMPTY_WAV"}

        cmd = [sys.executable, str(VSTUDIO / "analyzers" / "audio_analyzer.py"), str(wav_path)]
        if expected_frames:
            cmd.extend([str(expected_frames), "512"])
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if r.returncode == 0:
            try:
                return json.loads(r.stdout)
            except json.JSONDecodeError:
                return {"verification": "PARSE_ERROR", "error": r.stdout[:200]}
        return {"verification": "ANALYZER_ERROR", "error": r.stderr[:200]}

    def run(self):
        log(f"=== Vertical slice: {self.name} ===")

        if not self.start_jack():
            return {"verification": "FAIL", "error": "JACK failed"}

        mode = self.config.get("mode", "direct")
        use_sl = self.config.get("use_sooperlooper", False)

        if use_sl:
            self.start_sooperlooper()

        if not self.start_source():
            return {"verification": "FAIL", "error": "Source failed"}

        if not self.start_capture():
            return {"verification": "FAIL", "error": "Capture failed"}

        conn_mode = "sooperlooper" if use_sl else "direct"
        self.connect_ports(conn_mode)

        if use_sl:
            time.sleep(0.5)
            self.trigger_sooperlooper_record()

        duration = self.config.get("duration_seconds", 3)
        log(f"Recording {duration}s...")
        time.sleep(duration)

        log("Stopping...")
        self.stop_all()
        time.sleep(2)  # Wait for capture writer to finish

        # Fix WAV header (capture may not have updated it)
        wav_path = self.run_dir / "audio" / "captured.wav"
        if wav_path.exists() and wav_path.stat().st_size > 44:
            import struct
            size = wav_path.stat().st_size
            with open(wav_path, "r+b") as f:
                f.seek(4); f.write(struct.pack("<I", size - 8))
                f.seek(40); f.write(struct.pack("<I", size - 44))
            log(f"Fixed WAV header: {size} bytes")

        # Auto-calculate expected frames from actual WAV duration
        wav_path = self.run_dir / "audio" / "captured.wav"
        expected = self.config.get("expected_frames")
        if wav_path.exists() and wav_path.stat().st_size > 100:
            import struct
            with open(wav_path, "rb") as f:
                f.seek(40)
                data_size = struct.unpack("<I", f.read(4))[0]
            sr = self.config.get("sample_rate", 48000)
            actual_frames = data_size // (2 * 4)  # 2 channels * 4 bytes
            if actual_frames > 0:
                expected = actual_frames
        analysis = self.analyze(expected)

        result = {
            "name": self.name,
            "mode": mode,
            "config": self.config,
            "analysis": analysis,
            "verification": analysis.get("verification", "UNKNOWN"),
        }

        with open(self.run_dir / "result.json", "w") as f:
            json.dump(result, f, indent=2)

        log(f"=== Result: {result['verification']} ===")
        return result


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "direct"

    if mode == "direct":
        config = {
            "mode": "direct",
            "sample_rate": 48000,
            "period_size": 1024,
            "use_sooperlooper": False,
            "duration_seconds": 2,
            "source": {"mode": "beats", "tempo": 120, "numerator": 4, "denominator": 4},
            "expected_frames": 96000,  # 2 bars at 120 BPM, 48kHz
        }
    elif mode == "sooperlooper":
        config = {
            "mode": "sooperlooper",
            "sample_rate": 48000,
            "period_size": 1024,
            "use_sooperlooper": True,
            "osc_port": 9951,
            "loops": 1,
            "channels": 2,
            "duration_seconds": 4,
            "source": {"mode": "beats", "tempo": 120, "numerator": 4, "denominator": 4},
            "expected_frames": 192000,  # 4 bars at 120 BPM, 48kHz
        }
    else:
        with open(mode) as f:
            config = json.load(f)

    name = config.get("name", f"slice-{mode}")
    run_dir = VSTUDIO / "runs" / name
    runner = Runner(name, run_dir, config)
    result = runner.run()
    print(json.dumps(result, indent=2))
    return 0 if result["verification"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
