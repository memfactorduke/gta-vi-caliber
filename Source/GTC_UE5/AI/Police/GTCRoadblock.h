// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCRoadblock.generated.h"

class UStaticMeshComponent;

/**
 * AGTCRoadblock — the wall of barriers the police string across the road ahead of
 * a fleeing player at high heat, EMBODYING FRoadblock. The director rolls whether
 * to set one up (FPoliceEscalation::RoadblockChance) and where (FRoadblock places
 * it far enough ahead to deploy, scaled by player speed); this actor lays the
 * barrier line across the carriageway and retires itself once the player threads
 * or rams through (FRoadblock::HasPassed).
 *
 * Thin AActor over the tested placement core (FRoadblock — plain UE cm, ground
 * plane X/Y, +Z up; no remap). Below a saturating unit count the line leaves a
 * squeeze-through gap, so it's an obstacle to thread or ram, not an instant wall.
 * Self-contained — solid barriers block whatever drives into them.
 */
UCLASS()
class GTC_UE5_API AGTCRoadblock : public AActor
{
    GENERATED_BODY()

public:
    AGTCRoadblock();

    virtual void Tick(float DeltaSeconds) override;

    /**
     * Lay the barrier line across the road. The actor's location is the blockade
     * centre; `FleeDir` is the player's (normalised, planar) travel direction, the
     * line runs perpendicular to it. `UnitCount` barriers span `RoadWidth` (cm);
     * `LifeSeconds` is a fallback despawn if the player never passes.
     */
    void Setup(const FVector& FleeDir, double RoadWidth, int32 UnitCount, double LifeSeconds);

private:
    /** Span a single barrier plugs (cm) — matches FRoadblock::UnitWidth. */
    static constexpr double BarrierWidthCm = 280.0;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Roadblock", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> Root;

    /** The barrier segments, created in Setup. */
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Barriers;

    FVector FleeDirection = FVector::ForwardVector;
    double LifeRemaining = 20.0;

    APawn* ResolvePlayer() const;
};
