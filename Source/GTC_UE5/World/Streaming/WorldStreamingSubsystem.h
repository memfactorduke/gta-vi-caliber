// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "StreamingResidency.h"
#include "WorldStreamingSubsystem.generated.h"

/**
 * UWorldStreamingSubsystem — the M3 streaming adapter. It is the runtime driver
 * that finally puts the tested streaming pure-cores to work: each tick it samples
 * the player (camera proxy) position and velocity, builds the FResidencyPlanInput
 * policy from its tunables, steps the FStreamingResidency brain (which composes
 * grid + hysteresis + priority + budget via FResidencyPlanner and tracks per-tile
 * LOD bands), and forwards the resulting load / unload / band-change events to the
 * actual World Partition cell streaming.
 *
 * That last hop — talking to World Partition — is intentionally a NARROW VIRTUAL
 * SEAM (OnTileLoad / OnTileUnload / OnTileLodChanged), empty by default. The cell
 * load/unload + HLOD/impostor swap is live-editor runtime work (it needs a
 * WP-authored map and the WorldPartition runtime), so per the architecture golden
 * rule we don't pull a speculative WorldPartition module dependency into this
 * headless-clean module: a live-wired subclass overrides the seam to drive WP. The
 * planning half above the seam is fully exercised in a normal Game/PIE build and is
 * unit-tested headless through FStreamingResidency.
 *
 * Ticks via FTickableGameObject, Game/PIE worlds only — the same shape as
 * UGTCTrafficSubsystem / UGTCCrowdSubsystem (the other stream-around-the-player
 * world subsystems). Tunables are in centimetres (UE world units), TileSize default
 * 12800 = 128 m per the M3 streaming design direction.
 */
UCLASS()
class GTC_UE5_API UWorldStreamingSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// --- Tuning (centimetres) ---------------------------------------------------

	/** Master switch; also gates the tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	bool bEnableStreaming = true;

	/** World units per tile edge (cm). 12800 = 128 m, the M3 cell cadence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	double TileSizeCm = 12800.0;

	/** Chebyshev radius of the candidate window around the player, in tiles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	int32 WindowRadius = 6;

	/** Load a tile once it is within this distance of the player (hysteresis inner). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	double LoadRadiusCm = 38400.0; // 3 tiles

	/** Release a tile once it is beyond this distance (hysteresis outer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	double UnloadRadiusCm = 64000.0; // 5 tiles

	/** Anticipation horizon: tiles in the travel direction this far ahead load first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	double LookAheadSeconds = 1.0;

	/** Per-tick load cap — the main-thread instancing budget. <= 0 means unbounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	int32 MaxLoadsPerTick = 4;

	/** How often to re-plan (s). The planner is cheap; throttling keeps it off the
	 *  hot path between meaningful camera moves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming")
	double PlanIntervalSec = 0.1;

	// --- LOD detail bands (centimetres, outer distance of each band) ------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming|LOD")
	double FullDistCm = 25600.0; // <= 2 tiles: full mesh

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming|LOD")
	double HlodDistCm = 51200.0; // <= 4 tiles: HLOD proxy

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming|LOD")
	double ImpostorDistCm = 89600.0; // <= 7 tiles: impostor; beyond -> unloaded. 0 disables bands.

	/** Symmetric dead-zone around each LOD threshold so a boundary tile doesn't flip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Streaming|LOD")
	double LodMarginCm = 2560.0;

	// --- UWorldSubsystem / FTickableGameObject ----------------------------------

	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bEnableStreaming; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

	// --- Debug / HUD readout (P6) -----------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "GTC|Streaming")
	int32 GetResidentTileCount() const { return Residency.Num(); }

	UFUNCTION(BlueprintCallable, Category = "GTC|Streaming")
	int32 GetLastLoadCount() const { return LastLoadCount; }

	UFUNCTION(BlueprintCallable, Category = "GTC|Streaming")
	int32 GetLastUnloadCount() const { return LastUnloadCount; }

	UFUNCTION(BlueprintCallable, Category = "GTC|Streaming")
	FIntPoint GetPlayerTile() const { return PlayerTile; }

protected:
	// --- World Partition seam (override in a live-wired subclass) ----------------
	// Defaults are no-ops: the planning above runs in any build; only the actual WP
	// cell hop needs the live editor + a WP-authored map.

	/** Begin streaming in the cell for Tile (its centre in world cm is provided). */
	virtual void OnTileLoad(const FIntPoint& Tile, const FVector2D& CenterCm) {}

	/** Release the cell for Tile. */
	virtual void OnTileUnload(const FIntPoint& Tile) {}

	/** A resident tile's detail band changed (drive the HLOD/impostor swap). */
	virtual void OnTileLodChanged(const FIntPoint& Tile, ETileLod Lod) {}

private:
	APawn* GetPlayerPawn() const;

	/** Player XY in cm, or false if there's no player pawn yet. */
	bool SamplePlayerXY(FVector2D& OutXY) const;

	FStreamingResidency Residency;

	FVector2D PrevPlayerXY = FVector2D::ZeroVector;
	bool bHavePrev = false;
	double PlanAccum = 0.0;

	// Last-plan stats for the debug HUD.
	int32 LastLoadCount = 0;
	int32 LastUnloadCount = 0;
	FIntPoint PlayerTile = FIntPoint::ZeroValue;
};
