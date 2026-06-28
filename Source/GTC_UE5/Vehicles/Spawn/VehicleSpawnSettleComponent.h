// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h" // TEnumAsByte<ECollisionChannel>
#include "VehicleSpawnSettle.h"  // the pure-core this component drives
#include "VehicleSpawnSettleComponent.generated.h"

/**
 * UVehicleSpawnSettleComponent — drop this on a physics-vehicle Blueprint (e.g.
 * BP_VehicleBase) to kill the "flies on spawn" launch in World-Partition maps such as
 * CityBeachStrip. It does NOT touch the car's input graph, its Chaos setup, or its
 * physics enable state, so it can't break driving — it only nudges the actor's
 * transform for the first instant of life.
 *
 * Each PRE-physics frame it traces straight down and feeds the hit to the tested core
 * FVehicleSpawnSettle: while the road cell has not streamed in it parks the car a fixed
 * clearance above its spawn (so it is above the road surface when the road appears),
 * and the frame ground is found it snaps the car to just above it, zeroes velocity, and
 * stops — the depenetration launch never gets a frame to fire. In a fully-loaded map
 * (e.g. CarTest) ground is found on the first frame, so the car settles instantly with
 * no visible hold and no behaviour change.
 *
 * No ChaosVehicles dependency: the owner is driven as a bare AActor with a
 * UPrimitiveComponent root, matching the Vehicles/Entry design (UVehicleSeatComponent).
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UVehicleSpawnSettleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVehicleSpawnSettleComponent();

    /** How far (cm) above the spawn point to park the car while waiting for ground to stream in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Spawn")
    float HoldClearanceCm = 300.0f;

    /** Gap (cm) left above the road surface when settling, so wheels drop a touch and rest. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Spawn")
    float SettleAboveGroundCm = 25.0f;

    /** Fail-safe: stop holding after this long if ground never appears (car spawned over a hole). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Spawn")
    float MaxWaitSeconds = 5.0f;

    /** Extra height (cm) above the car to begin the downward trace, to clear the chassis. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Spawn")
    float TraceStartAboveCm = 100.0f;

    /** How far (cm) below the car to look for ground. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Spawn")
    float TraceDistanceCm = 5000.0f;

    /**
     * OBJECT TYPE the road/ground is (this is a query against object types, NOT a trace
     * channel). The down-trace lands only on actors of this object type — WorldStatic for
     * the road/landscape — so it skips Pawn-typed NPCs (which here block both Visibility and
     * WorldStatic) and the Vehicle-typed car itself. Keep this an object-type channel
     * (WorldStatic / WorldDynamic / PhysicsBody / ...); a trace-only channel such as
     * Visibility or Camera matches no objects, so ground is never found and the car falls
     * back to the hold-then-timeout path.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle|Spawn")
    TEnumAsByte<ECollisionChannel> GroundChannel = ECC_WorldStatic;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** Pull the editable tuning into the core's FParams. */
    FVehicleSpawnSettle::FParams Params() const;

    /** Stop ticking once settled — the per-frame cost after that is zero. */
    void Finish();

    /** World Z the car spawned at, captured on BeginPlay (the hold reference). */
    double SpawnZ = 0.0;

    /** Seconds spent waiting for ground so far. */
    double Elapsed = 0.0;

    bool bSpawnZRecorded = false;
    bool bDone = false;
};
