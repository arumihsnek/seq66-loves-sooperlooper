"""JackServer for LAB-A v3.9."""
import os
import signal
import subprocess
import time
from typing import Dict, List, Optional


class JackServer:
    """Controls a JACK server instance."""

    def __init__(self, server_name: str = "jackd") -> None:
        self.server_name = server_name
        self._process: Optional[subprocess.Popen] = None
        self._pid: Optional[int] = None
        self._pgid: Optional[int] = None

    def start(self) -> None:
        """Start the JACK server with dummy driver in own process group."""
        if self.is_running():
            return
        # Command: jackd --name <server> --no-realtime -d dummy
        cmd = [
            "jackd",
            "--name",
            self.server_name,
            "--no-realtime",
            "-d",
            "dummy",
        ]
        # Use os.setsid to create a new process group
        def preexec_fn():
            os.setsid()

        self._process = subprocess.Popen(
            cmd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            preexec_fn=preexec_fn,
        )
        # Get the PID and PGID
        self._pid = self._process.pid
        try:
            self._pgid = os.getpgid(self._pid)
        except ProcessLookupError:
            # If process already died, we'll let is_running() handle it
            pass

    def readiness_probe(self, timeout_s: float) -> bool:
        """Check if the JACK server is ready to accept connections.

        Args:
            timeout_s: Seconds to wait for readiness.

        Returns:
            True if server is ready, False otherwise.
        """
        if not self.is_running():
            return False
        # Simple readiness check: try to get ports via jack_lsp
        # We'll try a few times within the timeout
        start = time.time()
        while time.time() - start < timeout_s:
            try:
                # Use jack_lsp to see if server is responding
                result = subprocess.run(
                    ["jack_lsp"],
                    capture_output=True,
                    text=True,
                    timeout=1.0,
                )
                if result.returncode == 0:
                    # If we get a response, server is ready
                    return True
            except (subprocess.TimeoutExpired, FileNotFoundError):
                pass
            time.sleep(0.1)
        return False

    def get_ports(self) -> List[str]:
        """Get list of JACK ports.

        Returns:
            List of port names.
        """
        if not self.is_running():
            return []
        try:
            result = subprocess.run(
                ["jack_lsp"],
                capture_output=True,
                text=True,
                timeout=2.0,
            )
            if result.returncode == 0:
                # Each line is a port; strip whitespace
                ports = [line.strip() for line in result.stdout.splitlines() if line.strip()]
                return ports
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        return []

    def get_connections(self) -> List[str]:
        """Get list of JACK connections.

        Returns:
            List of connection strings in format "source:destination".
        """
        if not self.is_running():
            return []
        try:
            result = subprocess.run(
                ["jack_lsp", "-c"],
                capture_output=True,
                text=True,
                timeout=2.0,
            )
            if result.returncode == 0:
                connections = []
                for line in result.stdout.splitlines():
                    line = line.strip()
                    if line:
                        connections.append(line)
                return connections
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        return []

    def verify_no_external_clients(self) -> bool:
        """Verify that there are no external clients connected to the JACK server.

        Returns:
            True if no external clients, False otherwise.
        """
        if not self.is_running():
            return True  # No server, no clients
        try:
            # We'll check for connections. If there are any connections, we assume
            # there are clients (external or not). Since we don't track our own,
            # we'll be conservative and say if there are connections, there might be external.
            # However, the constraint expects us to verify no external clients.
            # We'll use jack_lsp -c to see connections and if there are none, return True.
            result = subprocess.run(
                ["jack_lsp", "-c"],
                capture_output=True,
                text=True,
                timeout=2.0,
            )
            if result.returncode == 0:
                # If there are any connections, there are clients.
                # We'll assume any connection means external clients (since we don't track).
                lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
                return len(lines) == 0
            else:
                # If we can't get connections, we cannot verify, so return False.
                return False
        except (subprocess.TimeoutExpired, FileNotFoundError):
            return False

    def cleanup(self) -> None:
        """Stop the JACK server and clean up owned process only."""
        if self._process is not None:
            try:
                # Send SIGTERM
                os.kill(self._pid, signal.SIGTERM)
                # Wait a bit
                try:
                    self._process.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    # Send SIGKILL
                    os.kill(self._pid, signal.SIGKILL)
                    self._process.wait(timeout=5.0)
            except ProcessLookupError:
                pass  # Already gone
            finally:
                self._process = None
                self._pid = None
                self._pgid = None

    def is_running(self) -> bool:
        """Check if the JACK server process is running.

        Returns:
            True if running, False otherwise.
        """
        if self._pid is None:
            return False
        try:
            os.kill(self._pid, 0)  # Check if process exists
            return True
        except ProcessLookupError:
            return False

    def get_pid(self) -> Optional[int]:
        """Get the JACK server process ID.

        Returns:
            PID or None if not running.
        """
        return self._pid

    def get_pgid(self) -> Optional[int]:
        """Get the JACK server process group ID.

        Returns:
            PGID or None if not running.
        """
        return self._pgid

    def snapshot(self) -> Dict[str, Optional[int]]:
        """Get a snapshot of the JACK server state.

        Returns:
            Dict with keys 'pid' and 'pgid'.
        """
        return {
            'pid': self._pid,
            'pgid': self._pgid,
        }
