// Out-of-tree compile shim — a minimal stand-in for UE's FMath, just enough to
// compile and run the GTC living-city pure-core structs with clang++ headless.
// NOT a UE header. Only the subset the pure-core uses is provided.
#pragma once

#include <cmath>
#include <limits>

// Match real UE: KINDA_SMALL_NUMBER is a FLOAT, so a bare FMath::Max(KINDA_SMALL_NUMBER, <double>)
// fails template deduction here exactly as under UBT (forcing callers to spell the type).
#ifndef KINDA_SMALL_NUMBER
#define KINDA_SMALL_NUMBER (1.e-4f)
#endif

struct FMath
{
    template <typename T>
    static T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }

    static double Abs(double V) { return std::fabs(V); }

    template <typename T>
    static T Max(T A, T B) { return A > B ? A : B; }
    template <typename T>
    static T Min(T A, T B) { return A < B ? A : B; }

    static double Sqrt(double V) { return std::sqrt(V); }
    static double Square(double V) { return V * V; }
    static double Pow(double B, double E) { return std::pow(B, E); }

    static bool IsNearlyZero(double V, double Tol = 1e-8) { return std::fabs(V) <= Tol; }
    static bool IsNearlyEqual(double A, double B, double Tol = 1e-8) { return std::fabs(A - B) <= Tol; }
    static bool IsFinite(double V) { return std::isfinite(V); }

    // Templated so FVector and double both lerp through operator-/+/*(double).
    template <typename T>
    static T Lerp(const T& A, const T& B, double T01) { return A + (B - A) * T01; }

    static double Fmod(double X, double Y) { return std::fmod(X, Y); }
    static double Atan2(double Y, double X) { return std::atan2(Y, X); }
    static double Cos(double V) { return std::cos(V); }
    static double Sin(double V) { return std::sin(V); }
    static double Sign(double V) { return V > 0.0 ? 1.0 : (V < 0.0 ? -1.0 : 0.0); }

    static double FloorToDouble(double V) { return std::floor(V); }
    static double CeilToDouble(double V) { return std::ceil(V); }
    static int FloorToInt(double V) { return static_cast<int>(std::floor(V)); }
    static int CeilToInt(double V) { return static_cast<int>(std::ceil(V)); }
    static int RoundToInt(double V) { return static_cast<int>(std::floor(V + 0.5)); }
};
