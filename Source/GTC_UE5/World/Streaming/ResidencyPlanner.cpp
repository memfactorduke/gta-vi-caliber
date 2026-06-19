// Copyright Epic Games, Inc. All Rights Reserved.

#include "ResidencyPlanner.h"

#include "StreamingGrid.h"
#include "TileHysteresis.h"
#include "TileStreamPriority.h"

FStreamPlan FResidencyPlanner::Plan(const FResidencyPlanInput& In, const TArray<FIntPoint>& Resident)
{
	FStreamPlan Out;

	const TSet<FIntPoint> ResidentSet(Resident);
	const FIntPoint Center = FStreamingGrid::WorldToTile(In.CameraPos, In.TileSize);
	const TArray<FIntPoint> Window = FStreamingGrid::TilesInWindow(Center, In.WindowRadius);
	const TSet<FIntPoint> WindowSet(Window);

	// Resident tiles that fell entirely outside the candidate window are always
	// released — they are further than the window reaches.
	for (const FIntPoint& Tile : Resident)
	{
		if (!WindowSet.Contains(Tile))
		{
			Out.ToUnload.Add(Tile);
		}
	}

	// Walk the window: keep, unload, or queue-to-load each candidate.
	struct FLoadCandidate
	{
		FIntPoint Tile;
		double EffectiveDistance;
	};
	TArray<FLoadCandidate> Loads;

	for (const FIntPoint& Tile : Window)
	{
		const bool bResident = ResidentSet.Contains(Tile);
		const double Dist = FStreamingGrid::CenterDistance(Tile, In.CameraPos, In.TileSize);
		const bool bWantResident = FTileHysteresis::ShouldBeResident(bResident, Dist, In.LoadRadius, In.UnloadRadius);

		if (bResident && !bWantResident)
		{
			Out.ToUnload.Add(Tile); // pushed out of the band
		}
		else if (!bResident && bWantResident)
		{
			const double Eff = FTileStreamPriority::EffectiveDistance(
				In.CameraPos, In.Velocity, FStreamingGrid::TileCenter(Tile, In.TileSize), In.LookAheadSeconds);
			Loads.Add({Tile, Eff});
		}
		// else: resident & wanted (keep), or not resident & not wanted (skip)
	}

	// Most urgent (smallest anticipatory distance) first; stable tile-coord
	// tie-break for determinism.
	Loads.Sort([](const FLoadCandidate& A, const FLoadCandidate& B)
	{
		if (A.EffectiveDistance != B.EffectiveDistance)
		{
			return A.EffectiveDistance < B.EffectiveDistance;
		}
		if (A.Tile.X != B.Tile.X)
		{
			return A.Tile.X < B.Tile.X;
		}
		return A.Tile.Y < B.Tile.Y;
	});

	// Per-frame budget cap (count axis); <= 0 means unbounded.
	int32 Take = Loads.Num();
	if (In.MaxLoadsPerTick > 0)
	{
		Take = FMath::Min(In.MaxLoadsPerTick, Loads.Num());
	}
	Out.ToLoad.Reserve(Take);
	for (int32 I = 0; I < Take; ++I)
	{
		Out.ToLoad.Add(Loads[I].Tile);
	}

	// Deterministic unload order (release order is functionally irrelevant).
	Out.ToUnload.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		if (A.X != B.X)
		{
			return A.X < B.X;
		}
		return A.Y < B.Y;
	});

	return Out;
}
