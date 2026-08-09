"""ProcessSupervisor for LAB-A v3.9."""
import os
import signal
import subprocess
import time
from dataclasses import dataclass
from typing import Dict, List, Optional


@dataclass
class ProcessInfo:
    """Process information."""
    pid: int
    pgid: int


class ProcessSupervisor:
    """Supervises owned processes only."""

    def __init__(self) -> None:
        self._owned_pids: List[int] = []
        self._owned_pgids: List[int] = []
        self._process_info: Dict[int, ProcessInfo] = {}

    def __enter__(self) -> "ProcessSupervisor":
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        self.cleanup(timeout=5.0)

    def start_process(
        self,
        name: str,
        cmd: List[str],
        env: Optional[Dict[str, str]],
        cwd: str,
        stdout_path: str,
        stderr_path: str,
    ) -> ProcessInfo:
        """Start a process and register it as owned.

        Args:
            name: Process name.
            cmd: Command and arguments as a list.
            env: Environment variables. If None, use os.environ.
            cwd: Working directory.
            stdout_path: Path to stdout redirect file.
            stderr_path: Path to stderr redirect file.

        Returns:
            ProcessInfo for the started process.

        Raises:
            subprocess.CalledProcessError if the process fails to start.
        """
        if env is None:
            env = os.environ.copy()
        # Ensure we use os.setsid to create a new process group
        def preexec_fn():
            os.setsid()

        with open(stdout_path, 'wb') as stdout_file, open(stderr_path, 'wb') as stderr_file:
            proc = subprocess.Popen(
                cmd,
                env=env,
                cwd=cwd,
                stdout=stdout_file,
                stderr=stderr_file,
                preexec_fn=preexec_fn,
            )
        # Register the process
        pid = proc.pid
        pgid = os.getpgid(pid)
        self._owned_pids.append(pid)
        self._owned_pgids.append(pgid)
        self._process_info[pid] = ProcessInfo(pid=pid, pgid=pgid)
        return self._process_info[pid]

    def cleanup(self, timeout: float) -> None:
        """Terminate all owned processes with SIGTERM -> wait -> SIGKILL bounded.

        Args:
            timeout: Seconds to wait for SIGTERM before sending SIGKILL.
        """
        # First, SIGTERM all owned processes
        for pid in list(self._owned_pids):
            try:
                os.kill(pid, signal.SIGTERM)
            except ProcessLookupError:
                pass  # Already gone

        # Wait for timeout seconds
        time.sleep(timeout)

        # Then, SIGKILL any remaining
        for pid in list(self._owned_pids):
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass  # Already gone

        # Finally, wait for all to prevent zombies
        for pid in list(self._owned_pids):
            try:
                os.waitpid(pid, 0)
            except ChildProcessError:
                pass  # Already waited

        # Clear owned lists
        self._owned_pids.clear()
        self._owned_pgids.clear()
        self._process_info.clear()

    def verify_cleanup(self) -> List[int]:
        """Return list of owned PIDs that are still running after cleanup.

        Returns:
            List of owned PIDs still alive (should be empty).
        """
        alive = []
        for pid in self._owned_pids:
            try:
                os.kill(pid, 0)  # Check if process exists
                alive.append(pid)
            except ProcessLookupError:
                pass
        return alive

    def get_owned_pids(self) -> List[int]:
        """Return a copy of the owned PIDs list."""
        return list(self._owned_pids)

    def get_owned_pgids(self) -> List[int]:
        """Return a copy of the owned PGIDs list."""
        return list(self._owned_pgids)

    def snapshot(self) -> Dict[str, List[int]]:
        """Return a snapshot of owned PIDs and PGIDs.

        Returns:
            Dict with keys 'owned_pids' and 'owned_pgids'.
        """
        return {
            'owned_pids': list(self._owned_pids),
            'owned_pgids': list(self._owned_pgids),
        }

    def get_process_info(self, name: str) -> Optional[ProcessInfo]:
        """Get process info by name.

        Note: This method is not specified in the baseline but is in the API contract.
        We'll return None as we don't track by name.
        """
        return None
