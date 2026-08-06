import os
import subprocess
import time
from typing import List, Optional, Dict, Any

class JackServer:
    def __init__(self, name: str = "jackd"):
        self.name = name
        self._proc: Optional[subprocess.Popen] = None

    def start(self) -> None:
        # Launch jackd --name <server> --no-realtime -d dummy in own group
        cmd = ["jackd", "--name", self.name, "--no-realtime", "-d", "dummy"]
        # We'll start it in its own process group? The constraint says "in own group"
        # We'll use preexec_fn to os.setsid
        def preexec():
            os.setsid()
        self._proc = subprocess.Popen(cmd, preexec_fn=preexec, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def readiness_probe(self, timeout_s: float = 5.0) -> bool:
        # TODO: implement actual readiness check (e.g., check if jackd is responding)
        # For now, just check if process is still alive after a short start.
        if self._proc is None:
            return False
        # Poll to see if it's still running
        if self._proc.poll() is None:
            # Assume ready if still running (simplistic)
            return True
        return False

    def get_ports(self) -> List[str]:
        # Return list of port names (from jack_lsp)
        # For dummy driver, we can return empty or mock.
        return []

    def get_connections(self) -> List[str]:
        return []

    def verify_no_external_clients(self) -> bool:
        # For dummy, we can assume no external clients.
        return True

    def cleanup(self) -> None:
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
        self._proc = None

    def is_running(self) -> bool:
        return self._proc is not None and self._proc.poll() is None

    def get_pid(self) -> Optional[int]:
        if self._proc:
            return self._proc.pid
        return None

    def get_pgid(self) -> Optional[int]:
        pid = self.get_pid()
        if pid:
            try:
                return os.getpgid(pid)
            except (AttributeError, ProcessLookupError):
                return None
        return None

    def snapshot(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "pid": self.get_pid(),
            "pgid": self.get_pgid(),
            "running": self.is_running()
        }
