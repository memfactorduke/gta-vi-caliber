// Copyright Epic Games, Inc. All Rights Reserved.

#include "TileLodSelect.h"

namespace
{
	// Normalise the three band edges to non-decreasing (running max), so a
	// misordered config degrades to "this band is empty" rather than inverting
	// the cascade. T[0]=Full, T[1]=HLOD, T[2]=Impostor outer distances.
	void NormaliseBands(const FTileLodBands& Bands, double OutT[3])
	{
		OutT[0] = Bands.FullDist;
		OutT[1] = FMath::Max(OutT[0], Bands.HlodDist);
		OutT[2] = FMath::Max(OutT[1], Bands.ImpostorDist);
	}
}

ETileLod FTileLodSelect::Select(double Distance, const FTileLodBands& Bands)
{
	double T[3];
	NormaliseBands(Bands, T);

	if (Distance <= T[0])
	{
		return ETileLod::Full;
	}
	if (Distance <= T[1])
	{
		return ETileLod::HLOD;
	}
	if (Distance <= T[2])
	{
		return ETileLod::Impostor;
	}
	return ETileLod::Unloaded;
}

ETileLod FTileLodSelect::SelectWithHysteresis(double Distance, ETileLod Current, const FTileLodBands& Bands, double Margin)
{
	double T[3];
	NormaliseBands(Bands, T);
	const double M = FMath::Max(0.0, Margin);

	int32 L = static_cast<int32>(Current);

	// Coarsen (toward Unloaded) while clearly past the boundary above L. T[L] is
	// the outer edge of band L; Unloaded (L==3) has no coarser level.
	while (L < 3 && Distance > T[L] + M)
	{
		++L;
	}
	// Refine (toward Full) while clearly inside the boundary below L. T[L-1]
	// separates L from the next finer band.
	while (L > 0 && Distance < T[L - 1] - M)
	{
		--L;
	}

	return static_cast<ETileLod>(L);
}
