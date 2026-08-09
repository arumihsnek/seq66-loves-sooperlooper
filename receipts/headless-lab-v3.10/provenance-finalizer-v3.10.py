#!/usr/bin/env python3
"""Controller-owned fail-closed provenance finalizer for v3.10.

It delegates only to the transcript-bound verifier and never edits, stages, or
commits a leaf worktree. A failed provenance check produces no E artifact.
"""
from __future__ import annotations
import argparse, json, subprocess, sys
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--receipt", required=True)
    ap.add_argument("--transcript", required=True)
    ap.add_argument("--repository", required=True)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()
    verifier = Path(__file__).with_name("transcript-bound-verifier-v3.10.py")
    p = subprocess.run([sys.executable, str(verifier), "--receipt", args.receipt,
                        "--transcript", args.transcript, "--repository", args.repository,
                        "--json"], capture_output=True, text=True)
    try:
        result = json.loads(p.stdout)
    except json.JSONDecodeError:
        result = {"verdict": "REJECT", "failures": [{"code": "VERIFIER_OUTPUT_INVALID", "detail": p.stdout[-500:]}]}
    result["finalizer"] = "provenance-finalizer-v3.10"
    result["E_creation_allowed"] = result.get("verdict") == "PASS"
    print(json.dumps(result, indent=1, sort_keys=True))
    return 0 if result["E_creation_allowed"] else 1

if __name__ == "__main__":
    raise SystemExit(main())
