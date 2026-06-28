#!/usr/bin/env bash
# Build & run the host-side FSmartSharpen unit test (no Unreal engine needed).
set -euo pipefail
cd "$(dirname "$0")"
clang++ -std=c++17 -I fake -I ../../../Source/GTC_UE5/Camera \
    host_test_smart_sharpen.cpp -o /tmp/gtc_smart_sharpen_host_test
exec /tmp/gtc_smart_sharpen_host_test
