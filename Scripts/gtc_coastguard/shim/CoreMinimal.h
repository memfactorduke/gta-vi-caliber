// Minimal host-clang shim of the CoreMinimal slice the naval-pursuit cores touch, so
// Scripts/gtc_coastguard/*_verify.cpp can compile the REAL shipping cores
// (AI/Pursuit/NavalPursuit.cpp + the reused AI/Pursuit/PursuitTactics.cpp) with a plain
// host compiler instead of the engine — the same hook-safe pattern as the other
// Scripts/gtc_*/shim dirs. Fakes ONLY what those cores use (FVector incl Dot, the LWC
// constants); FMath lives in Math/UnrealMathUtility.h so the cores' include resolves.
#pragma once

#include <cmath>
#include <cstdint>

using int32 = std::int32_t;
using uint8 = std::uint8_t;

#define GTC_UE5_API
#ifndef FORCEINLINE
#define FORCEINLINE inline
#endif

// Long-World-Coordinate double constants the cores reference.
static constexpr double UE_DOUBLE_PI = 3.141592653589793238462643383279502884;
static constexpr double UE_DOUBLE_TWO_PI = 2.0 * UE_DOUBLE_PI;

// --- FVector (double precision, as the cores assume) -------------------------
struct FVector
{
    double X = 0.0, Y = 0.0, Z = 0.0;

    FVector() = default;
    FVector(double InX, double InY, double InZ) : X(InX), Y(InY), Z(InZ) {}

    FVector operator+(const FVector& O) const { return FVector(X + O.X, Y + O.Y, Z + O.Z); }
    FVector operator-(const FVector& O) const { return FVector(X - O.X, Y - O.Y, Z - O.Z); }
    FVector operator*(double S) const { return FVector(X * S, Y * S, Z * S); }

    double Size() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    double Dot(const FVector& O) const { return X * O.X + Y * O.Y + Z * O.Z; }

    FVector GetSafeNormal(double Tolerance = 1e-8) const
    {
        const double S = Size();
        return S > Tolerance ? FVector(X / S, Y / S, Z / S) : FVector(0.0, 0.0, 0.0);
    }

    bool IsNearlyZero(double Tol = 1e-4) const
    {
        return std::fabs(X) <= Tol && std::fabs(Y) <= Tol && std::fabs(Z) <= Tol;
    }

    static const FVector ZeroVector;
};
inline const FVector FVector::ZeroVector = FVector(0.0, 0.0, 0.0);
