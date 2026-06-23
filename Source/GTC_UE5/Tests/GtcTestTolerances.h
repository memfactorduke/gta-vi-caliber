#pragma once
#include "CoreMinimal.h"
// Shared automation-test tolerances and helpers. `inline`/`inline constexpr` => one ODR-safe
// definition across all translation units, so UE unity-build concatenation can never redefine them
// (the reason the per-file SpeedEps/VecNear copies previously collided). Unified on double.
namespace GtcTest
{
    inline constexpr double Eps = 1e-4;
    inline constexpr double ConvergeEps = 1e-2;
    inline constexpr double SpeedEps = 1e-3;

    // Component-wise vector near-equality within Tol.
    inline bool VecNear(const FVector& A, const FVector& B, double Tol = 1e-4)
    {
        return FMath::Abs(A.X - B.X) <= Tol && FMath::Abs(A.Y - B.Y) <= Tol
            && FMath::Abs(A.Z - B.Z) <= Tol;
    }
}
