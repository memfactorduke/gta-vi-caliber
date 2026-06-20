// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "GTCPoliceDirector.generated.h"

class AGTCPoliceOfficer;
class AGTCPoliceHelicopter;
class AGTCPoliceCar;
class AGTCRoadblock;
class AGTCSpikeStrip;
class AGTCSwatVan;
class AGTCK9;
class APawn;

/**
 * AGTCPoliceDirector — the Wave-3 actor that EMBODIES FPoliceDispatch: it turns the
 * live wanted level into a live police presence around the player by spawning and
 * retiring AGTCPoliceOfficer pawns. It is to the police what UGTCCrowdSubsystem is
 * to pedestrians (a ring streamer), but built as an AActor rather than a subsystem
 * — drop ONE into the persistent level, the same way AGTCWeatherController owns the
 * sky — because the repo forbids new subsystems without maintainer sign-off.
 *
 * Each streaming pass it:
 *   1. reads UWantedSubsystem::Stars();
 *   2. recalls officers per FPoliceDispatch::ShouldDespawn (stars 0 -> all stand
 *      down; anyone too far behind is culled);
 *   3. tops the field back up from FPoliceSpawnPlan (the deficit toward
 *      FPoliceDispatch::DesiredUnits, dealt as a fanned ring of typed units).
 *
 * It also drives the two clocks nothing else owns yet: UWantedSubsystem::TickFrame
 * (so heat decays and witness reports land) and UArrestSubsystem::Tick (fed the
 * nearest officer's distance, closing the bust loop). All pure decisions live in
 * the tested cores; this actor only spawns, places (NavMesh-projected), and recalls.
 *
 * FRAME/UNIT REMAP: FPoliceSpawnPlan offsets are the core frame (metres, planar XZ);
 * this director converts to Unreal world space (X->X, Z->Y, metres*100 -> cm) and
 * projects onto the NavMesh, exactly as the officer remaps for its combat math.
 */
UCLASS()
class GTC_UE5_API AGTCPoliceDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTCPoliceDirector();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Officer pawn to spawn. Defaults to AGTCPoliceOfficer; a BP subclass (with a
     *  mesh/anim) may override it without touching this director. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    TSubclassOf<AGTCPoliceOfficer> OfficerClass;

    /** Air-support pawn. Defaults to AGTCPoliceHelicopter; spawned as a single unit
     *  at high heat (FHelicopterPursuit::ShouldDeploy). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    TSubclassOf<AGTCPoliceHelicopter> HelicopterClass;

    /** Pursuit cruiser. Defaults to AGTCPoliceCar; a small pool joins at 2+ stars. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    TSubclassOf<AGTCPoliceCar> CarClass;

    /** Most pursuit cruisers live at once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    int32 MaxCars = 2;

    /** Police dog. Defaults to AGTCK9; a small pack joins at low heat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    TSubclassOf<AGTCK9> K9Class;

    /** Most police dogs live at once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    int32 MaxK9 = 2;

    /** Barrier wall thrown across the road ahead at high heat (FRoadblock). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    TSubclassOf<AGTCRoadblock> RoadblockClass;

    /** Spike strip laid on the approach to a roadblock (FSpikeStrip). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    TSubclassOf<AGTCSpikeStrip> SpikeStripClass;

    /** Heavy ground response: a van that rolls in and unloads a SWAT squad. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    TSubclassOf<AGTCSwatVan> SwatVanClass;

    /** Hard ceiling on live officers regardless of heat (the scene budget). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    int32 MaxAlive = 8;

    /** Most officers added per streaming pass, so pressure trickles in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    int32 MaxPerWave = 3;

    /** Seconds between streaming passes (spawning need not run every frame). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    double StreamIntervalSec = 0.6;

    /** Metres past which an officer that has fallen behind the player is recalled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    double DespawnRadiusMeters = 160.0;

    /** Drive UWantedSubsystem::TickFrame from here (nothing else does). Turn off if a
     *  future owner takes over the wanted clock. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    bool bDriveWantedClock = true;

    /** Feed UArrestSubsystem::Tick the nearest-officer distance so busts can land. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    bool bDriveArrest = true;

    /** Base seed for the deterministic spawn ring (angles/radii). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Police")
    int32 SpawnSeed = 1337;

private:
    /** The player pawn the response orbits, or null. */
    APawn* GetPlayerPawn() const;

    /** Refresh CachedStars + nearest-LIVING-officer distance every frame, so the
     *  arrest loop (driven per frame) never reads a stale or dead-cop snapshot. */
    void RefreshCombatSnapshot();

    /** One streaming pass: recall, then top up the field around the player. */
    void StreamPolice();

    /** Spawn one officer of `UnitType` at a NavMesh-projected world point. */
    void SpawnOfficer(int32 UnitTypeRaw, const FVector& WorldPoint);

    /** Spawn/recall the single air-support helicopter by heat (and prune a wreck). */
    void StreamHelicopter(const FVector& PlayerPos, int32 Stars);

    /** Spawn/recall the pursuit cruiser pool by heat (and prune wrecks). */
    void StreamCars(const FVector& PlayerPos, int32 Stars);

    /** Maybe throw a roadblock across the road ahead of a fleeing player. */
    void StreamRoadblock(const FVector& PlayerPos, int32 Stars);

    /** Spawn/recall the single SWAT van by heat (and prune a wreck). */
    void StreamSwatVan(const FVector& PlayerPos, int32 Stars);

    /** Spawn/recall the K9 pack by heat (and prune the dead). */
    void StreamK9(const FVector& PlayerPos, int32 Stars);

    /** Live officers (weak: a destroyed officer just drops out next prune). */
    UPROPERTY()
    TArray<TWeakObjectPtr<AGTCPoliceOfficer>> Officers;

    /** The single live air-support unit, or invalid when none is deployed. */
    TWeakObjectPtr<AGTCPoliceHelicopter> Helicopter;

    /** Live pursuit cruisers (weak: a destroyed car drops out next prune). */
    TArray<TWeakObjectPtr<AGTCPoliceCar>> Cars;

    /** Live police dogs (weak: a dead dog drops out next prune). */
    TArray<TWeakObjectPtr<AGTCK9>> K9s;

    /** The active roadblock, or invalid when none is up. */
    TWeakObjectPtr<AGTCRoadblock> Roadblock;

    /** The single live SWAT van, or invalid when none is deployed. */
    TWeakObjectPtr<AGTCSwatVan> SwatVan;

    /** Cooldown (s) so blocks aren't thrown up every pass. */
    double RoadblockCooldown = 0.0;

    FRandomStream Rng;
    int32 SpawnCounter = 0;
    double StreamAccum = 0.0;

    /** Last-known wanted stars + nearest officer distance (metres), refreshed each
     *  streaming pass and fed to the arrest loop every frame. */
    int32 CachedStars = 0;
    double CachedNearestCopDistM = 1.0e6;
};
