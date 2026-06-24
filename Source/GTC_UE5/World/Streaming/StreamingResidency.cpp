// Copyright Epic Games, Inc. All Rights Reserved.

#include "StreamingResidency.h"
#include "StreamingGrid.h"

FStreamingResidency::FStepResult FStreamingResidency::Step(
	const FResidencyPlanInput& In, const FTileLodBands& Bands, double LodMargin)
{
	FStepResult Result;
	Result.Plan = FResidencyPlanner::Plan(In, ResidentTiles);

	// Release first, then admit — so the resident set the band pass walks below is
	// exactly this tick's set. Order-preserving removes/adds keep Resident() stable.
	for (const FIntPoint& Tile : Result.Plan.ToUnload)
	{
		ResidentTiles.Remove(Tile);
		TileBands.Remove(Tile);
	}
	for (const FIntPoint& Tile : Result.Plan.ToLoad)
	{
		if (!ResidentTiles.Contains(Tile))
		{
			ResidentTiles.Add(Tile);
		}
	}

	// Re-evaluate each resident tile's detail band off its current band (hysteresis).
	// Disabled bands (no impostor range configured) hold every tile at full detail.
	const bool bBandsEnabled = Bands.ImpostorDist > 0.0;
	const double Margin = FMath::Max(0.0, LodMargin);
	for (const FIntPoint& Tile : ResidentTiles)
	{
		ETileLod NewLod = ETileLod::Full;
		if (bBandsEnabled)
		{
			const double Dist = FStreamingGrid::CenterDistance(Tile, In.CameraPos, In.TileSize);
			if (const ETileLod* Current = TileBands.Find(Tile))
			{
				NewLod = FTileLodSelect::SelectWithHysteresis(Dist, *Current, Bands, Margin);
			}
			else
			{
				NewLod = FTileLodSelect::Select(Dist, Bands);
			}
		}

		// A freshly admitted tile starts Unloaded, so its first real band is reported.
		ETileLod& Stored = TileBands.FindOrAdd(Tile, ETileLod::Unloaded);
		if (Stored != NewLod)
		{
			Stored = NewLod;
			Result.BandChanges.Add({Tile, NewLod});
		}
	}

	return Result;
}

ETileLod FStreamingResidency::BandOf(const FIntPoint& Tile) const
{
	const ETileLod* Found = TileBands.Find(Tile);
	return Found ? *Found : ETileLod::Unloaded;
}

void FStreamingResidency::Reset()
{
	ResidentTiles.Reset();
	TileBands.Reset();
}
