// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldStreamingSubsystem.h"
#include "StreamingGrid.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

void UWorldStreamingSubsystem::Deinitialize()
{
	Residency.Reset();
	bHavePrev = false;
	PlanAccum = 0.0;
	Super::Deinitialize();
}

bool UWorldStreamingSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UWorldStreamingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldStreamingSubsystem, STATGROUP_Tickables);
}

APawn* UWorldStreamingSubsystem::GetPlayerPawn() const
{
	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			return PC->GetPawn();
		}
	}
	return nullptr;
}

bool UWorldStreamingSubsystem::SamplePlayerXY(FVector2D& OutXY) const
{
	if (const APawn* Pawn = GetPlayerPawn())
	{
		const FVector Loc = Pawn->GetActorLocation();
		OutXY = FVector2D(Loc.X, Loc.Y);
		return true;
	}
	return false;
}

void UWorldStreamingSubsystem::Tick(float DeltaTime)
{
	if (!bEnableStreaming)
	{
		return;
	}

	// Throttle planning to PlanIntervalSec; the camera moves little between ticks.
	PlanAccum += DeltaTime;
	if (PlanAccum < FMath::Max(0.0, PlanIntervalSec))
	{
		return;
	}

	FVector2D PlayerXY;
	if (!SamplePlayerXY(PlayerXY))
	{
		// No player yet (pre-possession); hold the clock so the first real sample
		// gets a sane dt rather than a giant one.
		PlanAccum = 0.0;
		return;
	}

	const double Elapsed = PlanAccum;
	PlanAccum = 0.0;

	// Planar velocity from the sample delta (zero on the first plan).
	FVector2D Velocity = FVector2D::ZeroVector;
	if (bHavePrev && Elapsed > KINDA_SMALL_NUMBER)
	{
		Velocity = (PlayerXY - PrevPlayerXY) / Elapsed;
	}
	PrevPlayerXY = PlayerXY;
	bHavePrev = true;

	FResidencyPlanInput In;
	In.CameraPos = PlayerXY;
	In.Velocity = Velocity;
	In.TileSize = TileSizeCm;
	In.WindowRadius = WindowRadius;
	In.LoadRadius = LoadRadiusCm;
	In.UnloadRadius = UnloadRadiusCm;
	In.LookAheadSeconds = LookAheadSeconds;
	In.MaxLoadsPerTick = MaxLoadsPerTick;

	const FTileLodBands Bands{FullDistCm, HlodDistCm, ImpostorDistCm};

	const FStreamingResidency::FStepResult Result = Residency.Step(In, Bands, LodMarginCm);

	// Drive the World Partition seam.
	for (const FIntPoint& Tile : Result.Plan.ToLoad)
	{
		OnTileLoad(Tile, FStreamingGrid::TileCenter(Tile, TileSizeCm));
	}
	for (const FIntPoint& Tile : Result.Plan.ToUnload)
	{
		OnTileUnload(Tile);
	}
	for (const FStreamingResidency::FBandChange& Change : Result.BandChanges)
	{
		OnTileLodChanged(Change.Tile, Change.Lod);
	}

	// Debug readout.
	LastLoadCount = Result.Plan.ToLoad.Num();
	LastUnloadCount = Result.Plan.ToUnload.Num();
	PlayerTile = FStreamingGrid::WorldToTile(PlayerXY, TileSizeCm);
}
