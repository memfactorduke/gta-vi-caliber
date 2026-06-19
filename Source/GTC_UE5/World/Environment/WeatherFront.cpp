// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeatherFront.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	// Smoothstep with explicit edges, clamped — C1-continuous 0->1 ramp.
	double FrontSmoothStep(double Edge0, double Edge1, double X)
	{
		if (Edge1 <= Edge0)
		{
			return X < Edge0 ? 0.0 : 1.0;
		}
		const double T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.0, 1.0);
		return T * T * (3.0 - 2.0 * T);
	}
}

double FWeatherFront::FrontLineAt(double FrontStart, double Speed, double Time)
{
	return FrontStart + Speed * Time;
}

double FWeatherFront::SignedDistance(double Along, double FrontStart, double Speed, double Time)
{
	return Along - FrontLineAt(FrontStart, Speed, Time);
}

double FWeatherFront::Intensity01(double SignedDist, double Width)
{
	const double Half = FMath::Max(1e-6, Width) * 0.5;
	// Far ahead (SignedDist >> 0) -> smoothstep 1 -> intensity 0 (clear).
	// Far behind (SignedDist << 0) -> smoothstep 0 -> intensity 1 (full weather).
	return 1.0 - FrontSmoothStep(-Half, Half, SignedDist);
}

FWorldGlobals FWeatherFront::Globals(double Intensity01In, double DaylightFactor, double StormDarkening)
{
	const double I = FMath::Clamp(Intensity01In, 0.0, 1.0);
	const double Day = FMath::Clamp(DaylightFactor, 0.0, 1.0);
	const double Storm = FMath::Max(0.0, StormDarkening);

	FWorldGlobals G;
	G.CloudCoverage = I;                              // clouds build with the front
	G.RainIntensity = FrontSmoothStep(0.5, 1.0, I);        // rain only past the midpoint
	G.Wetness = G.RainIntensity;                      // target wetness tracks rain (lag is SurfaceWetness's job)
	// Night darkness, lifted by storm gloom so a heavy front dims even at noon.
	G.NightAmount = FMath::Clamp((1.0 - Day) + I * Storm, 0.0, 1.0);
	return G;
}
