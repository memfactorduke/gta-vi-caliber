#pragma once
// Shared automation-test tolerances. `inline constexpr` => one ODR-safe definition across all
// translation units, so UE unity-build concatenation can never redefine them. Unified on double.
namespace GtcTest
{
    inline constexpr double Eps = 1e-4;
    inline constexpr double ConvergeEps = 1e-2;
}
