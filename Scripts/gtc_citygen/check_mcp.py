#!/usr/bin/env python3
"""Pre-apply gate (host-side). Run BEFORE any editor apply.

    python3 check_mcp.py

Checks two preconditions and prints a clear GO / NO-GO:
  1. validation_report.json status == PASS  (never apply an invalid layout)
  2. Unreal MCP reachable on 127.0.0.1:8000  (the editor must have been launched
     with -ModelContextProtocolStartServer; we NEVER launch/relaunch it ourselves)

This does NOT touch the editor; it only probes the port. The PIE check
(EditorAppToolset.IsPIERunning) happens inside the apply payload once MCP is up.
"""

import json
import os
import socket

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))


def main():
    cfg = json.load(open(os.path.join(HERE, "slice_config.json")))
    out_dir = os.path.join(REPO, cfg["output_dir_rel"])
    report_path = os.path.join(out_dir, "validation_report.json")

    ok_valid = False
    if os.path.exists(report_path):
        rep = json.load(open(report_path))
        ok_valid = rep.get("status") == "PASS"
        print("validation_report: %s (%d violations)" % (rep.get("status"), rep.get("violation_count", -1)))
    else:
        print("validation_report: MISSING — run generate_slice.py first")

    ok_mcp = False
    try:
        with socket.create_connection(("127.0.0.1", 8000), timeout=2):
            ok_mcp = True
        print("MCP :8000: reachable")
    except OSError:
        print("MCP :8000: UNREACHABLE")

    print()
    if ok_valid and ok_mcp:
        print("GO — preconditions met. Apply may proceed (still confirm PIE is OFF in the payload).")
        return 0
    print("NO-GO.")
    if not ok_valid:
        print("  - Fix the layout: run generate_slice.py until validation PASSES.")
    if not ok_mcp:
        print("  - Launch UE with -ModelContextProtocolStartServer, then re-run. Do NOT relaunch from here.")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
