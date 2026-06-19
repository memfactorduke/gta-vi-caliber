// Copyright Epic Games, Inc. All Rights Reserved.

#include "OceanShore.h"
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

double FOceanShore::WaterDepth(double WaterSurfaceZ, double SeabedZ)
{
	return WaterSurfaceZ - SeabedZ;
}

double FOceanShore::ShoreBlend01(double Depth, double ShallowDepth)
{
	if (Depth <= 0.0)
	{
		return 0.0; // dry land / exactly at the waterline
	}
	return SmoothStep(0.0, FMath::Max(1e-6, ShallowDepth), Depth);
}

double FOceanShore::ShoreFoam01(double Depth, double FoamDepth)
{
	const double Band = FMath::Max(1e-6, FoamDepth);
	if (Depth <= 0.0 || Depth >= Band)
	{
		return 0.0; // dry land or past the break zone
	}
	// A band that rises from 0 at the waterline, peaks just offshore, falls to 0
	// by FoamDepth — continuous at both ends so it never pops on the sand.
	const double Edge = Band * 0.25;
	const double Rise = SmoothStep(0.0, Edge, Depth);
	const double Fall = 1.0 - SmoothStep(Edge, Band, Depth);
	return FMath::Clamp(Rise * Fall, 0.0, 1.0);
}

double FOceanShore::WetSand01(double TerrainZ, double SwashTopZ, double DryBand)
{
	// Fully wet at/below the swash top, drying out over DryBand above it.
	return 1.0 - SmoothStep(SwashTopZ, SwashTopZ + FMath::Max(1e-6, DryBand), TerrainZ);
}

double FOceanShore::DepthColorFade01(double Depth, double FadeDistance)
{
	if (Depth <= 0.0)
	{
		return 0.0;
	}
	const double Fade = FMath::Max(1e-6, FadeDistance);
	return FMath::Clamp(1.0 - FMath::Exp(-Depth / Fade), 0.0, 1.0);
}
