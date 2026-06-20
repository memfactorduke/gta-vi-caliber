#!/usr/bin/env bash
# Headless verifier for the police pure-cores (the spawn-plan composition).
#
# Each *_verify.cpp #includes the REAL shipping core .cpp from Source/GTC_UE5 and
# re-asserts the same invariants the GTC.* automation tests check — so the actual
# game math is exercised with a plain host clang instead of the engine. The
# in-editor UE automation runner (UnrealEditor-Cmd … Automation RunTests
# GTC.AI.PoliceDispatch) is intentionally NOT invoked: the editor-protect hook +
# the one-editor rule forbid launching it here.
#
# Usage: Scripts/gtc_police/run_checks.sh
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHIM="$HERE/shim"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

CXX="${CXX:-clang++}"
FLAGS=(-std=c++17 -O1 -Wall -Wextra -Wno-unused-parameter -I"$SHIM")

fail=0
shopt -s nullglob
verifiers=("$HERE"/*_verify.cpp)
if [ ${#verifiers[@]} -eq 0 ]; then
	echo "no *_verify.cpp found in $HERE"
	exit 1
fi

for src in "${verifiers[@]}"; do
	name="$(basename "$src" .cpp)"
	bin="$OUT/$name"
	if ! "$CXX" "${FLAGS[@]}" "$src" -o "$bin" 2> "$OUT/$name.log"; then
		echo "COMPILE FAIL: $name"
		cat "$OUT/$name.log"
		fail=1
		continue
	fi
	if "$bin"; then
		echo "PASS: $name"
	else
		echo "FAIL: $name"
		fail=1
	fi
done

if [ "$fail" -ne 0 ]; then
	echo "=== police checks FAILED ==="
	exit 1
fi
echo "=== all police checks PASSED ==="
