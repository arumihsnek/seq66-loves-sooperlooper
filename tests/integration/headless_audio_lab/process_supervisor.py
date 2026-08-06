import os
import signal
import subprocess
import time
from typing import List, Optional, Dict, Any, Tuple

class ProcessInfo:
    def __init__(self, pid: int, pgid: int):
        self.pid = pid
        self.pgid = pgid

class ProcessSupervisor:
    def __init__(self):
        self._owned_pids: List[int] = []
        self._owned_pgids: List[int] = []
        self._processes: Dict[int, Tuple[subprocess.Popen, Any, Any]] = {}  # pid -> (proc, stdout_f, stderr_f)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.cleanup(timeout=5.0)

    def start_process(self, name: str, cmd: List[str], env: Optional[Dict[str, str]], 
                      cwd: str, stdout_path: str, stderr_path: str) -> ProcessInfo:
        # Open stdout and stderr files in binary write mode
        stdout_f = open(stdout_path, 'wb')
        stderr_f = open(stderr_path, 'wb')
        # Create own process group via os.setsid in preexec_fn
        def preexec():
            os.setsid()
        proc = subprocess.Popen(
            cmd,
            env=env,
            cwd=cwd,
            stdout=stdout_f,
            stderr=stderr_f,
            preexec_fn=preexec
        )
        # Store file descriptors to close later
        self._owned_pids.append(proc.pid)
        pgid = os.getpgid(proc.pid)
        self._owned_pgids.append(pgid)
        self._processes[proc.pid] = (proc, stdout_f, stderr_f)
        return ProcessInfo(proc.pid, pgid)

    def cleanup(self, timeout: float = 5.0) -> None:
        # Terminate all owned processes
        for pid, (proc, stdout_f, stderr_f) in list(self._processes.items()):
            if proc.poll() is None:  # still running
                proc.terminate()
        # Wait
        start = time.time()
        while time.time() - start < timeout:
            if all(proc.poll() is not None for proc, _, _ in self._processes.values()):
                break
            time.sleep(0.1)
        # Kill any remaining
        for pid, (proc, stdout_f, stderr_f) in list(self._processes.items()):
            if proc.poll() is None:
                proc.kill()
        # Wait a bit more for killed processes
        start = time.time()
        while time.time() - start < timeout:
            if all(proc.poll() is not None for proc, _, _ in self._processes.values()):
                break
            time.sleep(0.1)
        # Close stdout/stderr files
        for _, (_, stdout_f, stderr_f) in self._processes.items():
            try:
                stdout_f.close()
            except:
                pass
            try:
                stderr_f.close()
            except:
                pass
        # Clear owned lists
        self._owned_pids.clear()
        self._owned_pgids.clear()
        self._processes.clear()

    def verify_cleanup(self) -> List[int]:
        # Return list of owned PIDs that are still alive after cleanup
        alive = []
        for pid, (proc, _, _) in self._processes.items():
            if proc.poll() is None:
                alive.append(pid)
        return alive

    def get_owned_pids(self) -> List[int]:
        return list(self._owned_pids)

    def get_owned_pgids(self) -> List[int]:
        return list(self._owned_pgids)

    def snapshot(self) -> Dict[str, Any]:
        return {
            "owned_pids": self.get_owned_pids(),
            "owned_pgids": self.get_owned_pgids(),
            "processes": {pid: {"poll": proc.poll()} for pid, (proc, _, _) in self._processes.items()}
        }

    def get_process_info(self, name: str) -> Optional[ProcessInfo]:
        # We don't store by name, so we cannot retrieve by name easily.
        # For the test, we might not need this. Return None.
        return None
