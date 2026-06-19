// Copyright Epic Games, Inc. All Rights Reserved.

#include "NightLights.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	// Smoothstep with explicit edges, clamped — C1-continuous 0->1 ramp.
	double SmoothStep(double Edge0, double Edge1, double X)
	{
		if (Edge1 <= Edge0)
		{
			return X < Edge0 ? 0.0 : 1.0;
		}
		const double T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.0, 1.0);
		return T * T * (3.0 - 2.0 * T);
	}
}

bool FNightLights::ShouldBeLit(bool CurrentlyLit, double Daylight, double OnThreshold, double OffThreshold)
{
	const double On = FMath::Min(OnThreshold, OffThreshold);   // dusk-on edge (lower)
	const double Off = FMath::Max(OnThreshold, OffThreshold);  // dawn-off edge (upper)

	if (CurrentlyLit)
	{
		// Stay lit until it is bright enough (daylight climbs past the dawn edge).
		return Daylight < Off;
	}
	// Turn on only once it is dark enough (daylight falls below the dusk edge).
	return Daylight < On;
}

double FNightLights::JitteredThreshold(double BaseThreshold, double Jitter01, double Spread)
{
	const double J = FMath::Clamp(Jitter01, 0.0, 1.0);
	return FMath::Clamp(BaseThreshold + (J - 0.5) * Spread, 0.0, 1.0);
}

double FNightLights::LitFraction(double Daylight, double OnThreshold, double OffThreshold)
{
	const double On = FMath::Min(OnThreshold, OffThreshold);
	const double Off = FMath::Max(OnThreshold, OffThreshold);
	// All lit below the dusk edge, all dark above the dawn edge, smooth in between.
	return 1.0 - SmoothStep(On, Off, Daylight);
}
