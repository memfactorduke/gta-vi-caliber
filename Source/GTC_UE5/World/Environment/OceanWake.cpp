// Copyright Epic Games, Inc. All Rights Reserved.

#include "OceanWake.h"
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

double FOceanWake::FroudeNumber(double Speed, double HullLength, double Gravity)
{
	if (HullLength <= 1e-6 || Gravity <= 0.0)
	{
		return 0.0;
	}
	return FMath::Max(0.0, Speed) / FMath::Sqrt(Gravity * HullLength);
}

double FOceanWake::WakeHalfAngleDeg(double Speed, double HullLength, double Gravity)
{
	const double Fr = FroudeNumber(Speed, HullLength, Gravity);
	if (Fr <= NarrowingFroude)
	{
		return KelvinHalfAngleDeg;
	}
	// Above the threshold the visible wake narrows ~1/Fr. Use the tangent so it is
	// continuous at Fr = NarrowingFroude (factor 1 there) and shrinks smoothly.
	const double KelvinRad = FMath::DegreesToRadians(KelvinHalfAngleDeg);
	const double Narrowed = FMath::Atan((NarrowingFroude / Fr) * FMath::Tan(KelvinRad));
	return FMath::RadiansToDegrees(Narrowed);
}

double FOceanWake::WakeStrength01(double Speed, double PlaningSpeed)
{
	if (PlaningSpeed <= 1e-6)
	{
		return Speed > 0.0 ? 1.0 : 0.0;
	}
	return SmoothStep(0.0, PlaningSpeed, FMath::Max(0.0, Speed));
}

double FOceanWake::BowFoamRate(double Speed, double PlaningSpeed, double MaxRate)
{
	const double Strength = WakeStrength01(Speed, PlaningSpeed);
	// Foam scales with both how hard the bow is pushing (strength) and how fast it
	// is throwing water (speed), normalised by the planing speed and capped.
	const double SpeedNorm = PlaningSpeed > 1e-6 ? FMath::Clamp(Speed / PlaningSpeed, 0.0, 1.0) : (Speed > 0.0 ? 1.0 : 0.0);
	return FMath::Clamp(Strength * SpeedNorm, 0.0, 1.0) * FMath::Max(0.0, MaxRate);
}

double FOceanWake::WhitecapFoam01(double Jacobian, double FoamJacobian)
{
	// Foam onsets as the surface folds: full coverage at/below Jacobian 0, none
	// once the sea is smoother than the onset threshold.
	const double Onset = FMath::Max(1e-6, FoamJacobian);
	return 1.0 - SmoothStep(0.0, Onset, FMath::Clamp(Jacobian, 0.0, Onset));
}

double FOceanWake::TrailFade01(double AgeSeconds, double LifetimeSeconds)
{
	if (LifetimeSeconds <= 1e-6)
	{
		return 0.0;
	}
	return FMath::Clamp(1.0 - AgeSeconds / LifetimeSeconds, 0.0, 1.0);
}
