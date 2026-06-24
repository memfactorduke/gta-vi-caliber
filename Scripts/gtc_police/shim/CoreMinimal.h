// Minimal host-clang shim of the slice of UE's CoreMinimal the police pure-core
// touches, so Scripts/gtc_police/*_verify.cpp can compile the REAL shipping cores
// (World/Police/PoliceResponse.cpp, AI/PoliceDispatch/PoliceDispatch.cpp,
// PoliceEscalation.cpp, PoliceSpawnPlan.cpp) with a plain host compiler instead of
// the engine — the same hook-safe pattern as Scripts/gtc_atmosphere/shim. It fakes
// ONLY what those cores use (FVector, FMath, TArray, the LWC constants); it is not
// a general UE emulation. FRandomStream lives in Math/RandomStream.h.
#pragma once

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>
#include <algorithm>

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

// --- FMath (only the helpers the cores call) ---------------------------------
struct FMath
{
    template <typename T> static T Min(T A, T B) { return A < B ? A : B; }
    template <typename T> static T Max(T A, T B) { return A > B ? A : B; }
    template <typename T> static T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }

    static double Cos(double R) { return std::cos(R); }
    static double Sin(double R) { return std::sin(R); }
    static double Sqrt(double V) { return std::sqrt(V); }
    static double Abs(double V) { return V < 0 ? -V : V; }
    static double Lerp(double A, double B, double T) { return A + (B - A) * T; }
};

// --- TArray (the slice the cores use: initializer-list ctor, Add/Num/[]/Contains, range-for) ---
template <typename T>
class TArray
{
public:
    TArray() = default;
    TArray(std::initializer_list<T> Init) : Data(Init) {}

    void Reserve(int32 N) { Data.reserve(static_cast<size_t>(N < 0 ? 0 : N)); }
    void Add(const T& V) { Data.push_back(V); }
    int32 Num() const { return static_cast<int32>(Data.size()); }

    T& operator[](int32 I) { return Data[static_cast<size_t>(I)]; }
    const T& operator[](int32 I) const { return Data[static_cast<size_t>(I)]; }

    bool Contains(const T& V) const
    {
        return std::find(Data.begin(), Data.end(), V) != Data.end();
    }

    typename std::vector<T>::iterator begin() { return Data.begin(); }
    typename std::vector<T>::iterator end() { return Data.end(); }
    typename std::vector<T>::const_iterator begin() const { return Data.begin(); }
    typename std::vector<T>::const_iterator end() const { return Data.end(); }

private:
    std::vector<T> Data;
};
