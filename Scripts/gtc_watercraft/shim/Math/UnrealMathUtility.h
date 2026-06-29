// Headless test shim for Math/UnrealMathUtility.h — a minimal FMath providing
// only the routines the atmosphere pure-cores call. Double-precision throughout,
// matching the cores. Keep additions in sync with what the cores actually use so
// the host build stays an honest stand-in for the engine math.
#pragma once

#include <cmath>
#include <cstdlib>
#include "CoreMinimal.h"

// Engine math constants the cores reference (double-precision spellings).
#ifndef PI
#define PI (3.1415926535897932)
#endif
#ifndef UE_PI
#define UE_PI (3.1415926535897932)
#endif
#ifndef UE_DOUBLE_PI
#define UE_DOUBLE_PI (3.1415926535897932)
#endif
#ifndef DOUBLE_PI
#define DOUBLE_PI (3.1415926535897932)
#endif

#ifndef KINDA_SMALL_NUMBER
#define KINDA_SMALL_NUMBER (1.e-4)
#endif
#ifndef SMALL_NUMBER
#define SMALL_NUMBER (1.e-8)
#endif

struct FMath
{
	template <typename T> static T Abs(T V) { return V < T(0) ? -V : V; }
	template <typename T> static T Min(T A, T B) { return A < B ? A : B; }
	template <typename T> static T Max(T A, T B) { return A > B ? A : B; }
	template <typename T> static T Min3(T A, T B, T C) { return Min(Min(A, B), C); }
	template <typename T> static T Max3(T A, T B, T C) { return Max(Max(A, B), C); }
	template <typename T> static T Sign(T V) { return V > T(0) ? T(1) : (V < T(0) ? T(-1) : T(0)); }
	template <typename T> static T Square(T V) { return V * V; }

	template <typename T> static T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }

	template <typename T, typename A> static T Lerp(T Aa, T Bb, A Alpha)
	{
		return (T)(Aa + (Bb - Aa) * Alpha);
	}

	static double Sqrt(double V) { return std::sqrt(V); }
	static double InvSqrt(double V) { return 1.0 / std::sqrt(V); }
	static double Pow(double A, double B) { return std::pow(A, B); }
	static double Exp(double V) { return std::exp(V); }
	static double Loge(double V) { return std::log(V); }

	static double Sin(double V) { return std::sin(V); }
	static double Cos(double V) { return std::cos(V); }
	static double Tan(double V) { return std::tan(V); }
	static double Asin(double V) { return std::asin(V); }
	static double Acos(double V) { return std::acos(V); }
	static double Atan(double V) { return std::atan(V); }
	static double Atan2(double Y, double X) { return std::atan2(Y, X); }

	static double Fmod(double A, double B) { return std::fmod(A, B); }
	static double Floor(double V) { return std::floor(V); }
	static double Ceil(double V) { return std::ceil(V); }
	static double Round(double V) { return std::round(V); }
	static int32 FloorToInt(double V) { return (int32)std::floor(V); }
	static int32 CeilToInt(double V) { return (int32)std::ceil(V); }
	static int32 RoundToInt(double V) { return (int32)std::llround(V); }
	static int32 TruncToInt(double V) { return (int32)V; }

	static bool IsNearlyZero(double V, double Tol = 1e-8) { return Abs(V) <= Tol; }
	static bool IsNearlyEqual(double A, double B, double Tol = 1e-8) { return Abs(A - B) <= Tol; }

	static double DegreesToRadians(double D) { return D * (UE_DOUBLE_PI / 180.0); }
	static double RadiansToDegrees(double R) { return R * (180.0 / UE_DOUBLE_PI); }

	static double GetMappedRangeValueClamped(double InLo, double InHi, double OutLo, double OutHi, double V)
	{
		if (InHi == InLo) { return OutLo; }
		const double T = Clamp((V - InLo) / (InHi - InLo), 0.0, 1.0);
		return OutLo + (OutHi - OutLo) * T;
	}

	// Deterministic uniform in [0,1) — not used inside cores (their RNG is
	// injected as a Roll), only available if a verifier wants jitter.
	static double FRand() { return (double)std::rand() / ((double)RAND_MAX + 1.0); }
};
