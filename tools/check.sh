#!/usr/bin/env bash
# Local gate == CI: build the editor target with UnrealBuildTool, then run the
# in-module GTC.* automation suite. This is the Definition-of-Done check from
# AGENTS.md / CLAUDE.md. Exits non-zero on a build failure, any failed test, or
# zero tests run, so it can gate a PR or a pre-push hook.
#
# Usage:   tools/check.sh
# Engine:  defaults to UE 5.7 at the standard install path; override with
#          UE="/path/to/UE_5.7" tools/check.sh
#
# Notes:
#  - Runs a clean default (unity) build, the same configuration CI/packaging use,
#    so unity-build ODR collisions are caught (they do not show in incremental
#    adaptive-unity dev builds).
#  - The editor runs head-less with -nullrhi (no GPU), so no shaders compile.
#  - Requires the project's plugins to be installed (e.g. CesiumForUnreal from
#    the .uproject MarketplaceURL); a missing required plugin fails the build.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJ="$ROOT/GTC_UE5.uproject"
UE="${UE:-/Users/Shared/Epic Games/UE_5.7}"
TARGET="GTC_UE5Editor"
PLATFORM="Mac"   # extend with `uname` for Linux/Windows runners when CI lands

fail() { echo "CHECK FAIL: $*" >&2; exit 1; }
[ -f "$PROJ" ] || fail "no uproject at $PROJ"
[ -d "$UE" ]   || fail "engine not found at '$UE' (set UE=/path/to/UE_5.7)"

echo "== [1/2] Build $TARGET $PLATFORM Development =="
"$UE/Engine/Build/BatchFiles/$PLATFORM/Build.sh" "$TARGET" "$PLATFORM" Development \
  -project="$PROJ" -waitmutex || fail "build failed"

echo "== [2/2] GTC.* automation (head-less, -nullrhi) =="
REPORT="$ROOT/Saved/Automation/Check"
rm -rf "$REPORT"; mkdir -p "$REPORT"
"$UE/Engine/Binaries/$PLATFORM/UnrealEditor-Cmd" "$PROJ" \
  -ExecCmds="Automation RunTests GTC; Quit" \
  -unattended -nopause -nullrhi -nosplash -nop4 -log \
  -ReportExportPath="$REPORT" || true   # runner exit code is unreliable; trust the report

IDX="$REPORT/index.json"
[ -f "$IDX" ] || fail "no automation report at $IDX (did the tests compile, did any match 'GTC'?)"
IDX="$IDX" python3 - <<'PY' || exit 1
import json, os, sys
d = json.load(open(os.environ["IDX"], encoding="utf-8-sig"))  # report may carry a UTF-8 BOM
tests = d.get("tests", [])
passed = sum(1 for t in tests if t.get("state") == "Success")
failed = sum(1 for t in tests if t.get("state") != "Success")
print(f"  automation: {len(tests)} tests, {passed} passed, {failed} failed")
if not tests:
    print("CHECK FAIL: no tests ran (wrong prefix or tests did not compile)", file=sys.stderr)
    sys.exit(1)
if failed:
    for t in tests:
        if t.get("state") != "Success":
            print("  FAILED:", t.get("fullTestPath") or t.get("testDisplayName"), file=sys.stderr)
    sys.exit(1)
PY
echo "CHECK PASS"
