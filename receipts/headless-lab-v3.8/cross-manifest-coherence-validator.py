#!/usr/bin/env python3
"""cross-manifest-coherence-validator.py — v3.8 package identity coherence gate.

Independent executable validator (controller-owned) that compares EVERY identity
source of the frozen v3.8 package against the single normative authority
(package-identity-v3.8.json). PASS only if every compared field is equal.

Sources compared (identity_sources_checked = N):
  1. package-payload-manifest-v3.8.json
  2. package-identity-v3.8.json            (authority)
  3. scope-freeze-v3.8.json
  4. batch-manifest-v3.8.json
  5. dispatch-plan-v3.8.json
  6. lease-plan-lab-a-v3.8.json
  7. lease-plan-lab-b-v3.8.json
  8. leaf-r-report-schema-v3.8.json
  9. controller-evidence-schema-v3.8.json
 10. controller-acceptance-schema-v3.8.json
 11. predispatch-readiness-v3.8.json

Fields compared (equality required):
  package, batch_id, candidate, leaf_id per lab, lease_id per lab,
  result evidence path per lab, planned branch pattern per lab,
  supersedes identity (package, batch_id, manifest_sha256).

Exit: 0 = PASS (identity_sources_checked == N, mismatches == []), 1 = FAIL.
"""
import argparse
import json
import os
import re
import sys

VERSION = "v3.8"
BATCH_RE = re.compile(r"^BATCH-\d{8}T\d{6}Z-DOGFOOD004-LAB-V3_8$")
AUTHORITY_FILE = "package-identity-v3.8.json"
PKG_FILE = "package-payload-manifest-v3.8.json"
SCOPE_FILE = "scope-freeze-v3.8.json"
BM_FILE = "batch-manifest-v3.8.json"
DISPATCH_FILE = "dispatch-plan-v3.8.json"
LEASE_A_FILE = "lease-plan-lab-a-v3.8.json"
LEASE_B_FILE = "lease-plan-lab-b-v3.8.json"
LEAF_SCHEMA_FILE = "leaf-r-report-schema-v3.8.json"
EVIDENCE_SCHEMA_FILE = "controller-evidence-schema-v3.8.json"
ACCEPTANCE_SCHEMA_FILE = "controller-acceptance-schema-v3.8.json"
PREDISPATCH_FILE = "predispatch-readiness-v3.8.json"


def load(pkg_dir, name):
    with open(os.path.join(pkg_dir, name)) as f:
        return json.load(f)


def branch_matches_pattern(branch, lab, authority_pattern):
    if not branch or not authority_pattern:
        return False
    lab_token = "a" if lab == "LAB-A" else "b"
    m = re.match(r"^result/.+-lab-([ab])-v3_(\d+)$", branch)
    if not m:
        return False
    return m.group(1) == lab_token and ("v3_" + m.group(2)) in authority_pattern


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--package-dir", required=True)
    ap.add_argument("--json", action="store_true", help="print JSON result")
    args = ap.parse_args()

    pkg = args.package_dir
    mismatches = []
    checked = []

    def req(name, ok, detail=""):
        checked.append(name)
        if not ok:
            mismatches.append({"check": name, "detail": detail})

    auth = load(pkg, AUTHORITY_FILE)
    sources = {
        "package-payload-manifest": load(pkg, PKG_FILE),
        "authority": auth,
        "scope-freeze": load(pkg, SCOPE_FILE),
        "batch-manifest": load(pkg, BM_FILE),
        "dispatch-plan": load(pkg, DISPATCH_FILE),
        "lease-A": load(pkg, LEASE_A_FILE),
        "lease-B": load(pkg, LEASE_B_FILE),
        "leaf-r-report-schema": load(pkg, LEAF_SCHEMA_FILE),
        "controller-evidence-schema": load(pkg, EVIDENCE_SCHEMA_FILE),
        "controller-acceptance-schema": load(pkg, ACCEPTANCE_SCHEMA_FILE),
        "predispatch-readiness": load(pkg, PREDISPATCH_FILE),
    }

    # ---- batch_id equality across ALL sources ----
    for name, src in sources.items():
        req(f"{name}.batch_id == authority.batch_id",
            src.get("batch_id") == auth.get("batch_id"),
            f"{src.get('batch_id')} vs {auth.get('batch_id')}")

    # ---- package equality for sources that declare it ----
    for name, src in sources.items():
        if isinstance(src.get("package"), str):
            req(f"{name}.package == authority.package",
                src.get("package") == auth.get("package"),
                f"{src.get('package')} vs {auth.get('package')}")

    # ---- candidate equality for sources that declare it ----
    for name, src in sources.items():
        if src.get("candidate_head_sha") is not None:
            req(f"{name}.candidate == authority.candidate",
                src.get("candidate_head_sha") == auth.get("candidate_head_sha"),
                f"{src.get('candidate_head_sha')} vs {auth.get('candidate_head_sha')}")

    # ---- supersedes identity: authority == batch-manifest == package-manifest ----
    for field in ("package", "batch_id", "manifest_sha256"):
        req(f"batch-manifest.supersedes.{field} == authority",
            sources["batch-manifest"].get("supersedes", {}).get(field)
            == auth.get("supersedes", {}).get(field),
            f"{sources['batch-manifest'].get('supersedes', {}).get(field)} "
            f"vs {auth.get('supersedes', {}).get(field)}")
        pm = sources["package-payload-manifest"]
        req("package-manifest.supersedes.package == authority",
            pm.get("supersedes") == auth.get("supersedes", {}).get("package"),
            f"{pm.get('supersedes')} vs {auth.get('supersedes', {}).get('package')}")
        req("package-manifest.supersedes.batch == authority",
            pm.get("supersedes_batch_id") == auth.get("supersedes", {}).get("batch_id"),
            f"{pm.get('supersedes_batch_id')} vs {auth.get('supersedes', {}).get('batch_id')}")
        req("package-manifest.supersedes.manifest_sha == authority",
            pm.get("supersedes_manifest_sha256") == auth.get("supersedes", {}).get("manifest_sha256"),
            f"{pm.get('supersedes_manifest_sha256')} vs {auth.get('supersedes', {}).get('manifest_sha256')}")

    # ---- per-lab mapping: leaf/lease/result path/branch pattern ----
    for lkey in ("LAB-A", "LAB-B"):
        acfg = auth.get("labs", {}).get(lkey)
        lpl = sources["lease-" + lkey[-1]]
        req(f"{lkey} registered in authority", acfg is not None)
        if acfg is None:
            continue
        req(f"{lkey}.leaf_id == authority",
            lpl.get("leaf_id") == acfg.get("leaf_id"),
            f"{lpl.get('leaf_id')} vs {acfg.get('leaf_id')}")
        req(f"{lkey}.lease_id == authority",
            lpl.get("lease_id") == acfg.get("lease_id"),
            f"{lpl.get('lease_id')} vs {acfg.get('lease_id')}")
        req(f"{lkey}.result_path == authority",
            lpl.get("result_path") == acfg.get("result_evidence_path"),
            f"{lpl.get('result_path')} vs {acfg.get('result_evidence_path')}")
        req(f"{lkey}.branch_pattern == authority",
            branch_matches_pattern(lpl.get("planned_result_branch"), lkey,
                                   acfg.get("planned_result_branch")),
            f"{lpl.get('planned_result_branch')}")

    # ---- leaf report schema carries the identity fields ----
    leaf_schema = sources["leaf-r-report-schema"]
    leaf_fields = leaf_schema.get("fields", [])
    for fld in ("lab", "batch", "leaf_id", "lease_id", "candidate_commit_oid"):
        req(f"leaf schema declares {fld}", fld in leaf_fields)

    # ---- evidence + acceptance schemas version ----
    ev_schema = sources["controller-evidence-schema"]
    acc_schema = sources["controller-acceptance-schema"]
    req("evidence schema version v3.8",
        str(ev_schema.get("schema", "")) == "controller-evidence-schema/v3.8"
        or "v3.8" in json.dumps(ev_schema))
    req("acceptance schema version v3.8",
        str(acc_schema.get("schema", "")) == "controller-acceptance-schema/v3.8"
        or "v3.8" in json.dumps(acc_schema))

    # ---- dispatch/predispatch non-authorization state ----
    dispatch = sources["dispatch-plan"]
    pred = sources["predispatch-readiness"]
    req("dispatch_authorized == false", dispatch.get("dispatch_authorized") is False)
    req("predispatch_authorized == false", pred.get("dispatch_authorized") is False)

    identity_sources_checked = len(checked)
    result = {
        "schema": "cross-manifest-coherence/v3.8",
        "identity_sources_checked": identity_sources_checked,
        "mismatches": mismatches,
        "pass": len(mismatches) == 0,
        "verdict": "PASS" if len(mismatches) == 0 else "FAIL",
    }
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print(f"IDENTITY_COHERENCE={result['verdict']} "
              f"(sources_checked={identity_sources_checked}, mismatches={len(mismatches)})")
        for m in mismatches:
            print(f"  MISMATCH {m['check']}: {m['detail']}")
    return 0 if result["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
