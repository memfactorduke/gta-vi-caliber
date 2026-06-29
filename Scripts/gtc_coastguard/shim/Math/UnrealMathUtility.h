// Headless test shim for Math/UnrealMathUtility.h — the FMath slice the naval-pursuit
// cores call. Double-precision, matching the cores. Includes CoreMinimal.h so FVector +
// the LWC constants are available in the same TU.
#pragma once

#include <cmath>
#include "CoreMinimal.h"

#ifndef PI
#define PI (3.1415926535897932)
#endif

struct FMath
{
    template <typename T> static T Min(T A, T B) { return A < B ? A : B; }
    template <typename T> static T Max(T A, T B) { return A > B ? A : B; }
    template <typename T> static T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }
    template <typename T> static T Sign(T V) { return V > T(0) ? T(1) : (V < T(0) ? T(-1) : T(0)); }
    template <typename T> static T Square(T V) { return V * V; }

    static double Cos(double R) { return std::cos(R); }
    static double Sin(double R) { return std::sin(R); }
    static double Sqrt(double V) { return std::sqrt(V); }
    static double Abs(double V) { return V < 0 ? -V : V; }
    static double Atan2(double Y, double X) { return std::atan2(Y, X); }
    static double Lerp(double A, double B, double T) { return A + (B - A) * T; }
    static bool IsNearlyEqual(double A, double B, double Tol = 1e-8) { return Abs(A - B) <= Tol; }
};
