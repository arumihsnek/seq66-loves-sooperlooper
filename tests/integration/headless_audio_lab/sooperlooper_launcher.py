"""SooperLooperLauncher for LAB-A v3.9."""
import subprocess
import time
from typing import Optional, List


class SooperLooperLauncher:
    """Launches and manages SooperLooper via ProcessSupervisor."""

    def __init__(
        self,
        supervisor: 'ProcessSupervisor',
        sl_cmd: str = "sooperlooper",
        sl_args: Optional[List[str]] = None,
        run_dir: str = ".",
    ) -> None:
        self.supervisor = supervisor
        self.sl_cmd = sl_cmd
        self.sl_args = sl_args or []
        self.run_dir = run_dir
        self._process_info: Optional['ProcessInfo'] = None

    def launch(self) -> 'ProcessInfo':
        """Launch SooperLooper owned by the supervisor.

        Returns:
            ProcessInfo for the launched SooperLooper process.
        """
        cmd = [self.sl_cmd] + self.sl_args
        # We'll start the process with the supervisor.
        # We'll use:
        #   name: "sooperlooper"
        #   cmd: as above
        #   env: None
        #   cwd: self.run_dir
        #   stdout_path: /dev/null
        #   stderr_path: a file in self.run_dir for SooperLooper stderr
        stderr_path = f"{self.run_dir}/sooperlooper.err"
        # Ensure the run_dir exists
        import os
        os.makedirs(self.run_dir, exist_ok=True)
        # Start the process via supervisor
        self._process_info = self.supervisor.start_process(
            name="sooperlooper",
            cmd=cmd,
            env=None,
            cwd=self.run_dir,
            stdout_path="/dev/null",
            stderr_path=stderr_path,
        )
        return self._process_info

    def readiness_ping(self) -> bool:
        """Check SooperLooper readiness via OSC ping.

        Returns:
            True if SooperLooper responds to OSC ping, False otherwise.
        """
        # We'll create an OscProbe with default settings and ping.
        from .osc_probe import OscProbe
        probe = OscProbe()
        return probe.ping()

    def cleanup(self) -> None:
        """Clean up the SooperLooper process owned by the supervisor."""
        if self._process_info is not None:
            # We'll terminate the process by asking the supervisor to cleanup.
            # This assumes the supervisor is only managing this one process.
            self.supervisor.cleanup(timeout=5.0)
            self._process_info = None
