// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTCPlaceRegistrySubsystem.generated.h"

/**
 * UGTCPlaceRegistrySubsystem — turns the abstract "place" strings the NPC brain
 * speaks (FNpcIntent::Place: "home", "diner", "bar", "park", "office", "gym",
 * "street", "restroom") into concrete world locations a citizen can walk to.
 *
 * The decision layer (FNpcMind / FNpcArchetypes) is deliberately scene-free: it
 * only ever names a *kind* of place. Something has to map that name onto the map,
 * and that something is per-world (the diner in this level is not the diner in
 * the next), so this is a UWorldSubsystem — created with the world, gone with it.
 *
 * Authored points register themselves (a diner actor, a park volume, a sidewalk
 * spline sampler — any of them calls RegisterPlace on BeginPlay). Until the level
 * has authored POIs, FindNearest still answers with a deterministic synthesized
 * point derived from the place name, so the city is alive and legible the moment
 * you press play instead of every citizen freezing for lack of a destination. The
 * fallback is clearly flagged (bSynthesized) so the spawner/HUD can tell a real
 * POI from a stand-in, and it is stable per (name, search origin grid cell) so an
 * NPC doesn't get a new "home" every time it re-queries.
 *
 * Pure registry + nearest-of-kind query; no ticking, no ownership of the citizens
 * that read it.
 */
USTRUCT()
struct FGTCPlace
{
    GENERATED_BODY()

    /** The kind of place ("home", "diner", ...). Lower-cased on registration. */
    UPROPERTY()
    FName Kind;

    /** World location the citizen heads to. */
    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    /** Optional cap on how many citizens treat this as their current destination
     *  (e.g. a four-stool diner). 0 = unlimited. Advisory; the registry tracks
     *  occupancy so the spawner can spread crowds across POIs of the same kind. */
    UPROPERTY()
    int32 Capacity = 0;
};

/** Result of a place query: where to go, and whether it was a real POI. */
USTRUCT(BlueprintType)
struct FGTCPlaceQueryResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Places")
    FVector Location = FVector::ZeroVector;

    /** True when no authored POI of the kind existed and the point was synthesized. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Places")
    bool bSynthesized = false;

    /** True when at least *some* destination (real or synthesized) was produced. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Places")
    bool bValid = false;

    /** Live occupancy of the chosen POI (0 for synthesized points). Lets a citizen
     *  read how busy its destination is (FNpcCrowding) and react. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Places")
    int32 Occupancy = 0;

    /** Capacity of the chosen POI (0 = uncapped / synthesized). Paired with
     *  Occupancy for busyness classification. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Places")
    int32 Capacity = 0;
};

UCLASS()
class GTC_UE5_API UGTCPlaceRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // --- Tuning -----------------------------------------------------------------

    /** Radius (cm) of the synthesized fallback ring around the world origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Places")
    double SynthesizedRingRadius = 6000.0;

    /** Grid cell (cm) the fallback snaps the query origin to, so re-queries from
     *  roughly the same spot return the same stand-in destination. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Places")
    double SynthesizedCellSize = 4000.0;

    /** When true, lay down a believable spread of POIs (home/office/diner/bar/park/
     *  gym/restroom/street) the first time EnsureSeeded is called, so a level with
     *  no hand-authored markers — like CityBeachStrip — still gives citizens real,
     *  spread-out destinations instead of every drive resolving to a stand-in.
     *  Hand-placed markers coexist (nearest-of-kind still wins). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Places")
    bool bAutoSeedCity = true;

    // --- Registration (called by POI actors / authoring) ------------------------

    /** Add a place of `Kind` at `Location`. Returns a handle for later removal. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Places")
    int32 RegisterPlace(FName Kind, const FVector& Location, int32 Capacity = 0);

    /** Remove a previously registered place by its handle. No-op on a bad handle. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Places")
    void UnregisterPlace(int32 Handle);

    /** How many authored POIs of a kind currently exist (excludes synthesized). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Places")
    int32 CountOfKind(FName Kind) const;

    // --- Queries (called by citizens) -------------------------------------------

    /**
     * Nearest authored place of `Kind` to `From`. If none is registered, returns a
     * deterministic synthesized point (bSynthesized = true). bValid is false only
     * if Kind is None.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Places")
    FGTCPlaceQueryResult FindNearest(FName Kind, const FVector& From) const;

    /**
     * Like FindNearest, but biases away from POIs already at capacity so a crowd
     * spreads across same-kind POIs instead of all piling into the closest one.
     * Falls back to the nearest (and then synthesized) when all are full.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Places")
    FGTCPlaceQueryResult FindAvailable(FName Kind, const FVector& From) const;

    // --- Occupancy bookkeeping (advisory, for crowd spreading) -------------------

    /** Note that a citizen has claimed / released a real POI location of a kind. */
    void NotePlaceClaimed(FName Kind, const FVector& Location);
    void NotePlaceReleased(FName Kind, const FVector& Location);

    /**
     * Lay down the default city POI layout around `Center` (the player/town anchor)
     * if bAutoSeedCity is set and nothing has been seeded yet. Idempotent. Points
     * are projected onto the NavMesh when one exists so seeded POIs sit on walkable
     * ground. The crowd director calls this once it knows where the player is.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Places")
    void EnsureSeeded(const FVector& Center);

    /** Whether the auto-seed has already run (so callers don't re-trigger it). */
    bool IsSeeded() const { return bSeeded; }

private:
    /** Deterministic stand-in point for a kind near a query origin. */
    FVector SynthesizeLocation(FName Kind, const FVector& From) const;

    /** Handle -> place. TMap keeps removal O(1) and handles stable across removals. */
    UPROPERTY()
    TMap<int32, FGTCPlace> Places;

    /** Live occupancy per (kind, snapped-location) for crowd spreading. */
    TMap<uint32, int32> Occupancy;

    int32 NextHandle = 1;
    bool bSeeded = false;

    static uint32 OccupancyKey(FName Kind, const FVector& Location);
};
