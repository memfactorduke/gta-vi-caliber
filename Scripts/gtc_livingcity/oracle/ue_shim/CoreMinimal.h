// Out-of-tree compile shim — a minimal stand-in for UE's CoreMinimal.h, just
// enough to compile and run the GTC living-city pure-core structs with clang++
// headless. NOT a UE header. Only the subset the pure-core uses is provided:
// int32/uint8, a double FVector / FVector2D, and a std::vector-backed TArray.
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <initializer_list>

#include "Math/UnrealMathUtility.h"

#define GTC_UE5_API
#ifndef TEXT
#define TEXT(x) x
#endif

using int32 = std::int32_t;
using uint8 = std::uint8_t;

struct FVector
{
    double X = 0.0, Y = 0.0, Z = 0.0;

    FVector() = default;
    FVector(double InX, double InY, double InZ) : X(InX), Y(InY), Z(InZ) {}

    static const FVector ZeroVector;

    FVector operator+(const FVector& O) const { return FVector(X + O.X, Y + O.Y, Z + O.Z); }
    FVector operator-(const FVector& O) const { return FVector(X - O.X, Y - O.Y, Z - O.Z); }
    FVector operator*(double S) const { return FVector(X * S, Y * S, Z * S); }
    bool operator==(const FVector& O) const { return X == O.X && Y == O.Y && Z == O.Z; }

    double SizeSquared() const { return X * X + Y * Y + Z * Z; }
    double Size() const { return std::sqrt(SizeSquared()); }

    FVector GetSafeNormal(double Tolerance = 1e-8) const
    {
        const double S = SizeSquared();
        if (S < Tolerance) { return ZeroVector; }
        const double L = std::sqrt(S);
        return FVector(X / L, Y / L, Z / L);
    }

    static double Dist(const FVector& A, const FVector& B) { return (A - B).Size(); }
    static double DotProduct(const FVector& A, const FVector& B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
};

inline const FVector FVector::ZeroVector = FVector(0.0, 0.0, 0.0);

struct FVector2D
{
    double X = 0.0, Y = 0.0;

    FVector2D() = default;
    FVector2D(double InX, double InY) : X(InX), Y(InY) {}

    FVector2D operator+(const FVector2D& O) const { return FVector2D(X + O.X, Y + O.Y); }
    FVector2D operator-(const FVector2D& O) const { return FVector2D(X - O.X, Y - O.Y); }
    FVector2D operator*(double S) const { return FVector2D(X * S, Y * S); }

    double SizeSquared() const { return X * X + Y * Y; }
    double Size() const { return std::sqrt(SizeSquared()); }
    double Dot(const FVector2D& O) const { return X * O.X + Y * O.Y; }

    static double Distance(const FVector2D& A, const FVector2D& B) { return (A - B).Size(); }
};

template <typename T>
struct TArray
{
    std::vector<T> V;

    TArray() = default;
    TArray(std::initializer_list<T> L) : V(L) {}

    int32 Num() const { return static_cast<int32>(V.size()); }
    bool IsEmpty() const { return V.empty(); }
    void Add(const T& X) { V.push_back(X); }
    template <typename... Args>
    int32 Emplace(Args&&... A) { V.emplace_back(std::forward<Args>(A)...); return Num() - 1; }
    void Reserve(int32 N) { V.reserve(static_cast<size_t>(N)); }
    void Reset() { V.clear(); }
    void Empty() { V.clear(); }
    // Order-NOT-preserving remove: swap the last element into Index, then shrink.
    void RemoveAtSwap(int32 Index) { V[static_cast<size_t>(Index)] = V.back(); V.pop_back(); }
    T Pop() { T X = V.back(); V.pop_back(); return X; }

    T& operator[](int32 I) { return V[static_cast<size_t>(I)]; }
    const T& operator[](int32 I) const { return V[static_cast<size_t>(I)]; }
    T& Last() { return V.back(); }
    const T& Last() const { return V.back(); }

    // Range-for support.
    typename std::vector<T>::iterator begin() { return V.begin(); }
    typename std::vector<T>::iterator end() { return V.end(); }
    typename std::vector<T>::const_iterator begin() const { return V.begin(); }
    typename std::vector<T>::const_iterator end() const { return V.end(); }
};
