// Headless test shim — a minimal stand-in for Unreal's CoreMinimal.h so the
// pure-core .cpp files under Source/GTC_UE5/World/Environment can be compiled and
// run with a plain host clang, no engine. Only what the atmosphere cores touch.
#pragma once

#include <cstdint>
#include <cmath>

// Engine integer aliases used in the cores.
using int8 = std::int8_t;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;

// Module export macro is a no-op in a host build.
#ifndef GTC_UE5_API
#define GTC_UE5_API
#endif
