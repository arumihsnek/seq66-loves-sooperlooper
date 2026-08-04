#!/usr/bin/env python3
"""
Process sandbox for the virtual studio.

Manages JACK server, SooperLooper, and auxiliary processes.
Provides start/stop/cleanup with isolation and observability.
"""

import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path


class ProcessSandbox:
    """Manages a set of named processes with lifecycle control."""

    def __init__(self, run_dir, server_name=None):
        self.run_dir = Path(run_dir)
        self.server_name = server_name or f"vs-{os.getpid()}"
        self.processes = {}
        self.logs = {}
        self.run_dir.mkdir(parents=True, exist_ok=True)

    def start(self, name, cmd, env=None, ready_file=None, ready_timeout=10,
              ready_signal=None):
        """Start a named process, optionally wait for readiness."""
        proc_dir = self.run_dir / "process"
        proc_dir.mkdir(exist_ok=True)

        stdout_path = proc_dir / f"{name}.stdout.log"
        stderr_path = proc_dir / f"{name}.stderr.log"

        full_env = os.environ.copy()
        full_env["JACK_SERVER_NAME"] = self.server_name
        if env:
            full_env.update(env)

        stdout_f = open(stdout_path, "w")
        stderr_f = open(stderr_path, "w")

        proc = subprocess.Popen(
            cmd,
            stdout=stdout_f,
            stderr=stderr_f,
            env=full_env,
            preexec_fn=os.setsid,  # New process group
        )

        self.processes[name] = {
            "proc": proc,
            "cmd": cmd,
            "pid": proc.pid,
            "stdout": str(stdout_path),
            "stderr": str(stderr_path),
            "stdout_f": stdout_f,
            "stderr_f": stderr_f,
            "started_at": time.time(),
        }

        # Wait for readiness
        if ready_file:
            self._wait_for_file(ready_file, ready_timeout, name)
        elif ready_signal:
            self._wait_for_signal(proc, ready_signal, ready_timeout, name)

        return proc

    def start_jack(self, sample_rate=48000, period_size=1024, capture=2,
                   playback=2, wait=True):
        """Start JACK dummy server."""
        cmd = [
            "jackd", "-r",  # Non-realtime
            "-d", "dummy",
            "-r", str(sample_rate),
            "-p", str(period_size),
            "-C", str(capture),
            "-P", str(playback),
        ]

        # Set JACK environment
        jack_env = {
            "JACK_SERVER_NAME": self.server_name,
        }

        # Create JACK lock file path
        jack_dir = self.run_dir / "jack"
        jack_dir.mkdir(exist_ok=True)

        self.start(
            "jackd", cmd, env=jack_env,
            ready_timeout=5 if wait else 0,
        )

        # Verify JACK is running
        if wait and self.is_alive("jackd"):
            time.sleep(0.5)
            return True
        return False

    def start_sooperlooper(self, loops=1, channels=2, loop_time=40,
                           osc_port=9951, jack_name=None, wait=True):
        """Start SooperLooper headless."""
        sl_name = jack_name or f"sooperlooper-{self.server_name}"
        cmd = [
            "sooperlooper",
            "-l", str(loops),
            "-c", str(channels),
            "-t", str(loop_time),
            "-p", str(osc_port),
            "-j", sl_name,
            "-S", self.server_name,
            "-q",  # Quiet
        ]

        self.start(
            "sooperlooper", cmd,
            ready_timeout=5 if wait else 0,
        )

        if wait and self.is_alive("sooperlooper"):
            time.sleep(0.5)
            return True
        return False

    def stop(self, name, timeout=5):
        """Stop a named process gracefully, then force-kill if needed."""
        if name not in self.processes:
            return

        info = self.processes[name]
        proc = info["proc"]

        if proc.poll() is not None:
            # Already dead
            self._close_fds(name)
            return

        # Try SIGTERM first
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except (ProcessLookupError, PermissionError):
            pass

        try:
            proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            # Force kill
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                pass

        self._close_fds(name)

    def stop_all(self, timeout=5):
        """Stop all processes in reverse start order."""
        for name in reversed(list(self.processes.keys())):
            self.stop(name, timeout)

    def is_alive(self, name):
        """Check if a named process is still running."""
        if name not in self.processes:
            return False
        return self.processes[name]["proc"].poll() is None

    def get_pid(self, name):
        """Get PID of a named process."""
        if name not in self.processes:
            return None
        return self.processes[name]["pid"]

    def get_exit_code(self, name):
        """Get exit code of a named process."""
        if name not in self.processes:
            return None
        return self.processes[name]["proc"].returncode

    def get_status(self):
        """Get status of all processes."""
        status = {}
        for name, info in self.processes.items():
            proc = info["proc"]
            status[name] = {
                "pid": info["pid"],
                "alive": proc.poll() is None,
                "exit_code": proc.returncode,
                "uptime": time.time() - info["started_at"],
            }
        return status

    def write_status(self):
        """Write process status to JSON."""
        status = self.get_status()
        status_path = self.run_dir / "process" / "status.json"
        with open(status_path, "w") as f:
            json.dump(status, f, indent=2)
        return status

    def cleanup(self):
        """Stop all processes and clean up."""
        self.stop_all()
        # Close any remaining file descriptors
        for name in list(self.processes.keys()):
            self._close_fds(name)

    def _wait_for_file(self, path, timeout, name):
        """Wait for a file to appear (readiness signal)."""
        start = time.time()
        while time.time() - start < timeout:
            if Path(path).exists():
                return True
            if not self.is_alive(name):
                return False
            time.sleep(0.1)
        return False

    def _wait_for_signal(self, proc, signal_type, timeout, name):
        """Wait for output containing a specific signal string."""
        start = time.time()
        stderr_path = self.processes[name]["stderr"]
        while time.time() - start < timeout:
            if not self.is_alive(name):
                return False
            try:
                with open(stderr_path, "r") as f:
                    content = f.read()
                    if signal_type in content:
                        return True
            except FileNotFoundError:
                pass
            time.sleep(0.1)
        return False

    def _close_fds(self, name):
        """Close file descriptors for a process."""
        if name in self.processes:
            info = self.processes[name]
            try:
                info["stdout_f"].close()
            except Exception:
                pass
            try:
                info["stderr_f"].close()
            except Exception:
                pass


class JackConnections:
    """Manage JACK port connections."""

    def __init__(self, server_name):
        self.server_name = server_name

    def list_ports(self):
        """List all JACK ports."""
        r = subprocess.run(
            ["jack_lsp", "-s", self.server_name],
            capture_output=True, text=True, timeout=5
        )
        if r.returncode == 0:
            return [p for p in r.stdout.strip().split("\n") if p]
        return []

    def connect(self, src, dst):
        """Connect two JACK ports."""
        r = subprocess.run(
            ["jack_connect", "-s", self.server_name, src, dst],
            capture_output=True, text=True, timeout=5
        )
        return r.returncode == 0

    def disconnect(self, src, dst):
        """Disconnect two JACK ports."""
        r = subprocess.run(
            ["jack_disconnect", "-s", self.server_name, src, dst],
            capture_output=True, text=True, timeout=5
        )
        return r.returncode == 0

    def get_graph(self):
        """Get current JACK graph as port list."""
        return self.list_ports()


if __name__ == "__main__":
    # Quick test
    import tempfile
    with tempfile.TemporaryDirectory() as tmpdir:
        sb = ProcessSandbox(tmpdir, "test-001")
        print(f"Sandbox: {tmpdir}")
        print(f"Server name: {sb.server_name}")

        # Start JACK
        if sb.start_jack(sample_rate=48000, period_size=1024):
            print("JACK started!")
            ports = JackConnections(sb.server_name).list_ports()
            print(f"Ports: {ports}")
        else:
            print("JACK failed to start")

        sb.stop_all()
        print("Cleaned up")
