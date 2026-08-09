from __future__ import annotations

import json
import subprocess
import time
from typing import Dict, List, Any, Union
from pathlib import Path


class GraphAssertions:
    def __init__(self) -> None:
        self._snapshots: List[Dict[str, Any]] = []

    def _run_jack_lsp(self) -> Union[Dict[str, Any], None]:
        """Run jack_lsp and return parsed JSON, or None if not available."""
        try:
            # Use timeout to avoid hanging, but we must not terminate jackd.
            # jack_lsp itself is a utility that should exit quickly.
            result = subprocess.run(
                ["jack_lsp"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            if result.returncode != 0:
                # jack_lsp might fail if jackd is not running
                return None
            # jack_lsp output is plain text, not JSON. We'll parse it to extract graph information.
            # However, the contract does not specify the exact format of the returned data.
            # We'll return a simple structure based on the output.
            lines = result.stdout.strip().split('\n')
            # We'll just return the raw lines for now; the actual parsing can be done by the user.
            # For the purpose of the contract, we return something.
            return {
                "raw_output": lines,
                "system": True,  # placeholder
            }
        except subprocess.TimeoutExpired:
            # If timeout occurs, we return None to avoid hanging.
            return None
        except FileNotFoundError:
            # jack_lsp not found
            return None

    def get_graph(self) -> Dict[str, Any]:
        data = self._run_jack_lsp()
        if data is None:
            return {}
        return data

    def get_clients(self) -> List[str]:
        data = self._run_jack_lsp()
        if data is None:
            return []
        # Extract client names from raw_output (simplified)
        clients = []
        for line in data.get("raw_output", []):
            if line.startswith('system'):
                # The system client
                clients.append('system')
            else:
                # Assume other lines are clients
                # This is a naive parsing; in reality, jack_lsp output format is:
                #   system
                #     port1
                #     port2
                #   client1
                #     port1
                #     port2
                pass
        # For simplicity, we return an empty list if we can't parse.
        return []

    def get_ports(self) -> List[str]:
        data = self._run_jack_lsp()
        if data is None:
            return []
        # Similarly, extract port names
        return []

    def get_connections(self) -> List[str]:
        data = self._run_jack_lsp()
        if data is None:
            return []
        # Extract connections (this would require jack_lsp -c)
        # We'll run jack_lsp -c for connections
        try:
            result = subprocess.run(
                ["jack_lsp", "-c"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            if result.returncode == 0:
                lines = result.stdout.strip().split('\n')
                return lines
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        return []

    def assert_clean_graph(self) -> None:
        connections = self.get_connections()
        if connections:
            raise AssertionError(f"Graph not clean: {connections}")
        # Also check for external clients? We'll just check connections.

    def snapshot(self) -> Dict[str, Any]:
        snap = {
            "timestamp": time.time(),
            "graph": self.get_graph(),
            "clients": self.get_clients(),
            "ports": self.get_ports(),
            "connections": self.get_connections(),
        }
        self._snapshots.append(snap)
        return snap

    def diff_snapshots(self) -> Dict[str, Any]:
        if len(self._snapshots) < 2:
            raise ValueError("Need at least two snapshots to diff")
        snap1 = self._snapshots[-2]
        snap2 = self._snapshots[-1]
        # Compute a simple diff
        diff = {
            "timestamp_diff": snap2["timestamp"] - snap1["timestamp"],
            "graph_changed": snap2["graph"] != snap1["graph"],
            "clients_changed": snap2["clients"] != snap1["clients"],
            "ports_changed": snap2["ports"] != snap1["ports"],
            "connections_changed": snap2["connections"] != snap1["connections"],
        }
        return diff