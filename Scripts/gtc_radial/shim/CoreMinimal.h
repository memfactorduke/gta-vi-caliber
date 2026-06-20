// Minimal host-clang shim of the slice of UE's CoreMinimal that the radial-menu
// pure-core (Source/GTC_UE5/UI/Radial/RadialMenu.cpp) touches, so
// Scripts/gtc_radial/radial_menu_verify.cpp can compile the REAL shipping core
// with a plain host compiler instead of the engine — the same hook-safe pattern
// as Scripts/gtc_police/shim. It fakes ONLY what the radial core uses (int32,
// INDEX_NONE, the PI constants, FVector2D, the handful of FMath helpers); it is
// not a general UE emulation.
#pragma once

#include <cmath>
#include <cstdint>

using int32 = std::int32_t;
using uint8 = std::uint8_t;

#define GTC_UE5_API
#ifndef FORCEINLINE
#define FORCEINLINE inline
#endif

static constexpr int32 INDEX_NONE = -1;

static constexpr double UE_DOUBLE_PI = 3.141592653589793238462643383279502884;
static constexpr double UE_DOUBLE_TWO_PI = 2.0 * UE_DOUBLE_PI;

// --- FVector2D (the slice the core uses) -------------------------------------
struct FVector2D
{
    double X = 0.0, Y = 0.0;

    FVector2D() = default;
    FVector2D(double InX, double InY) : X(InX), Y(InY) {}

    FVector2D operator*(double S) const { return FVector2D(X * S, Y * S); }

    double Size() const { return std::sqrt(X * X + Y * Y); }

    static const FVector2D ZeroVector;
};
inline const FVector2D FVector2D::ZeroVector = FVector2D(0.0, 0.0);

// --- FMath (only the helpers the core calls) ---------------------------------
struct FMath
{
    static double Fmod(double A, double B) { return std::fmod(A, B); }
    static double Sin(double R) { return std::sin(R); }
    static double Cos(double R) { return std::cos(R); }
    static double Atan2(double Y, double X) { return std::atan2(Y, X); }

    static int32 RoundToInt(double V)
    {
        return static_cast<int32>(std::lround(V));
    }

    static bool IsNearlyZero(double V, double Tol = 1e-8) { return std::fabs(V) <= Tol; }
    static bool IsNearlyEqual(double A, double B, double Tol = 1e-8) { return std::fabs(A - B) <= Tol; }
};
