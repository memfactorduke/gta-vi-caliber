// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurfaceWetness.h"
#include "Math/UnrealMathUtility.h"

double FSurfaceWetness::Equilibrium(double RainIntensity)
{
	return FMath::Clamp(RainIntensity, 0.0, 1.0);
}

double FSurfaceWetness::Step(double Wetness, double RainIntensity, double DeltaSeconds,
	double WetRate, double DryRate)
{
	const double Target = FMath::Clamp(RainIntensity, 0.0, 1.0);
	const double W = FMath::Clamp(Wetness, 0.0, 1.0);

	// Rising toward a wetter target is fast; falling (drying) is slow.
	const double Rate = (Target > W) ? FMath::Max(0.0, WetRate) : FMath::Max(0.0, DryRate);

	// Exponential approach; clamp the per-step fraction so a huge dt can't overshoot.
	const double Alpha = FMath::Clamp(Rate * FMath::Max(0.0, DeltaSeconds), 0.0, 1.0);
	const double Next = W + (Target - W) * Alpha;
	return FMath::Clamp(Next, 0.0, 1.0);
}
