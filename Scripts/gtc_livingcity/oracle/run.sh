#!/usr/bin/env bash
# Compile + run every living-city pure-core oracle out-of-tree with clang++.
# Each oracle links the REAL pure-core .cpp against the minimal CoreMinimal.h
# shim and re-asserts its GTC.* test cases — the verification we can run while
# the live UE editor must stay up (it can't be closed for an UnrealBuildTool
# build; see Scripts/gtc_livingcity/PROGRESS.md). Exits non-zero on any failure.
set -u

cd "$(dirname "$0")"
SHIM="ue_shim"
SRC="../../../Source/GTC_UE5"
OUT="$(mktemp -d)"
CXX="${CXX:-clang++}"
FLAGS="-std=c++17 -O0 -g -Wall -Wextra -Wno-unused-parameter -I $SHIM"
RC=0

# oracle_name : real .cpp deps (space-separated, relative to $SRC)
build_run() {
  local oracle="$1"; shift
  local deps=()
  for d in "$@"; do deps+=("$SRC/$d"); done
  echo "=== $oracle ==="
  if ! $CXX $FLAGS "$oracle" "${deps[@]}" -o "$OUT/${oracle%.cpp}" 2>"$OUT/${oracle%.cpp}.err"; then
    echo "  COMPILE FAILED:"; sed 's/^/    /' "$OUT/${oracle%.cpp}.err"; RC=1; return
  fi
  "$OUT/${oracle%.cpp}" || RC=1
}

build_run lane_path_oracle.cpp World/RoadNetwork/LanePath.cpp
build_run traffic_lane_oracle.cpp AI/Traffic/TrafficLane.cpp Worldcore/TrafficModel.cpp

echo
[ $RC -eq 0 ] && echo "ALL ORACLES PASSED" || echo "SOME ORACLES FAILED"
exit $RC
