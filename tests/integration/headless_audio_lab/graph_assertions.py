"""
Graph assertions for headless audio lab.
"""
from __future__ import annotations

import json
import logging
import subprocess
import time
from typing import Optional, Set, List, Tuple, Dict, Any

logger = logging.getLogger(__name__)

class GraphAssertions:
    def __init__(self, server_name: str = "jackd"):
        self.server_name = server_name
        self._snapshots: Dict[str, Dict[str, Any]] = {}
    
    def _run_jack_lsp(self, args: Optional[list] = None) -> Optional[str]:
        cmd = ['jack_lsp']
        if args:
            cmd.extend(args)
        if self.server_name:
            pass
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                return result.stdout
            else:
                logger.error(f"jack_lsp failed: {result.stderr}")
                return None
        except subprocess.TimeoutExpired:
            logger.error("jack_lsp timed out")
            return None
        except Exception as e:
            logger.error(f"Error running jack_lsp: {e}")
            return None
    
    def get_graph(self) -> dict:
        output = self._run_jack_lsp()
        if output is None:
            return {}
        graph = {}
        for line in output.splitlines():
            line = line.strip()
            if not line:
                continue
            if ' -> ' in line:
                port, connections = line.split(' -> ', 1)
                port = port.strip()
                conns = [c.strip() for c in connections.split(',')]
                graph[port] = conns
            else:
                port = line.strip()
                graph[port] = []
        return graph
    
    def get_clients(self) -> set[str]:
        graph = self.get_graph()
        clients = set()
        for port in graph.keys():
            if ':' in port:
                client = port.split(':', 1)[0]
                clients.add(client)
            else:
                clients.add(port)
        return clients
    
    def get_ports(self) -> set[str]:
        return set(self.get_graph().keys())
    
    def get_connections(self) -> list[tuple[str, str]]:
        graph = self.get_graph()
        connections = []
        for source, dests in graph.items():
            for dest in dests:
                connections.append((source, dest))
        return connections
    
    def assert_exact_clients(self, expected: set[str]) -> None:
        actual = self.get_clients()
        if actual != expected:
            raise AssertionError(f"Client mismatch: expected {expected}, got {actual}")
    
    def assert_exact_ports(self, expected: set[str]) -> None:
        actual = self.get_ports()
        if actual != expected:
            raise AssertionError(f"Port mismatch: expected {expected}, got {actual}")
    
    def assert_connections(self, expected: list[tuple[str, str]]) -> None:
        actual = self.get_connections()
        actual_sorted = sorted(actual)
        expected_sorted = sorted(expected)
        if actual_sorted != expected_sorted:
            raise AssertionError(f"Connection mismatch: expected {expected_sorted}, got {actual_sorted}")
    
    def assert_no_connections(self) -> None:
        connections = self.get_connections()
        if connections:
            raise AssertionError(f"Expected no connections, got {connections}")
    
    def assert_port_exists(self, port_name: str) -> None:
        if port_name not in self.get_ports():
            raise AssertionError(f"Port '{port_name}' not found in graph")
    
    def assert_port_connected(self, port_name: str) -> None:
        graph = self.get_graph()
        if port_name not in graph:
            raise AssertionError(f"Port '{port_name}' not found in graph")
        if not graph[port_name]:
            raise AssertionError(f"Port '{port_name}' has no connections")
    
    def assert_port_not_connected(self, port_name: str) -> None:
        graph = self.get_graph()
        if port_name not in graph:
            raise AssertionError(f"Port '{port_name}' not found in graph")
        if graph[port_name]:
            raise AssertionError(f"Port '{port_name}' has connections: {graph[port_name]}")
    
    def snapshot_before(self, label: str) -> None:
        self._snapshots[f"{label}_before"] = self.get_graph()
    
    def snapshot_after(self, label: str) -> None:
        self._snapshots[f"{label}_after"] = self.get_graph()
    
    def diff_snapshots(self, label_a: str, label_b: str) -> dict:
        key_a = f"{label_a}_before"
        key_b = f"{label_b}_after"
        snap_a = self._snapshots.get(key_a, {})
        snap_b = self._snapshots.get(key_b, {})
        return {
            'before': snap_a,
            'after': snap_b,
            'added_ports': set(snap_b.keys()) - set(snap_a.keys()),
            'removed_ports': set(snap_a.keys()) - set(snap_b.keys())
        }
    
    def assert_clean_graph(self, expected_system_ports: Optional[set[str]] = None) -> None:
        graph = self.get_graph()
        for port, conns in graph.items():
            if conns:
                raise AssertionError(f"Port '{port}' has connections: {conns}")
        if expected_system_ports is not None:
            actual_ports = set(graph.keys())
            if actual_ports != expected_system_ports:
                raise AssertionError(f"Ports mismatch: expected {expected_system_ports}, got {actual_ports}")
