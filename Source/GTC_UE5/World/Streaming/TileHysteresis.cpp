// Copyright Epic Games, Inc. All Rights Reserved.

#include "TileHysteresis.h"

bool FTileHysteresis::ShouldBeResident(bool bCurrentlyResident, double Distance, double LoadRadius, double UnloadRadius)
{
	const double Load = FMath::Max(0.0, LoadRadius);
	// Outer radius can never be inside the inner one; a bad pair degrades to a
	// single threshold at Load rather than inverting the band.
	const double Unload = FMath::Max(Load, FMath::Max(0.0, UnloadRadius));

	if (Distance <= Load)
	{
		return true; // inside the inner radius: load (inclusive edge wins ties)
	}
	if (Distance >= Unload)
	{
		return false; // beyond the outer radius: unload
	}
	return bCurrentlyResident; // in the band: keep current state (no thrash)
}

bool FTileHysteresis::InBand(double Distance, double LoadRadius, double UnloadRadius)
{
	const double Load = FMath::Max(0.0, LoadRadius);
	const double Unload = FMath::Max(Load, FMath::Max(0.0, UnloadRadius));
	return Distance > Load && Distance < Unload;
}

double FTileHysteresis::AdvanceDwell(double CurrentDwell, bool bConditionHeld, double DeltaSeconds)
{
	if (!bConditionHeld)
	{
		return 0.0;
	}
	return CurrentDwell + FMath::Max(0.0, DeltaSeconds);
}
