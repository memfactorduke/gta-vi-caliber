// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCSpikeStrip.generated.h"

class UStaticMeshComponent;

/**
 * AGTCSpikeStrip — the stinger the police fling across the road, EMBODYING
 * FSpikeStrip. Where AGTCRoadblock is a wall to thread or ram, this is a hazard to
 * swerve around: cross it and your tyres shred. The actor lays the strip across the
 * carriageway, watches the player's path each frame, and on a crossing
 * (`FSpikeStrip::Crosses`) deals chassis damage and fires `OnSpiked` — the hook the
 * vehicle layer binds to apply the real grip/top-speed deflation
 * (`FSpikeStrip::DeflatedGripMult/TopSpeedMult` feed `FVehicleHandling`, which lives
 * in a sibling system). Detection + signal are fully here; the deflation effect is
 * the vehicle owner's to wire.
 *
 * Thin AActor over the tested core; `FSpikeStrip` is plain UE cm on the X/Y ground
 * plane (no frame remap). Deployed ahead of a fleeing player by AGTCPoliceDirector.
 */
UCLASS()
class GTC_UE5_API AGTCSpikeStrip : public AActor
{
    GENERATED_BODY()

public:
    AGTCSpikeStrip();

    virtual void Tick(float DeltaSeconds) override;

    /**
     * Lay the strip across the road: the actor's location is the centre, `FleeDir`
     * is the player's (planar) travel direction (the strip runs perpendicular to
     * it), spanning `RoadWidth` cm. `LifeSeconds` is a fallback despawn.
     */
    void Setup(const FVector& FleeDir, double RoadWidth, double LifeSeconds);

    /** Fired once when the player crosses the strip — the seam the vehicle layer
     *  binds to shred tyres (grip/top-speed deflation). Chassis damage is already
     *  applied; `Victim` is the pawn that crossed. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|SpikeStrip")
    void OnSpiked(APawn* Victim);

protected:
    /** Chassis/undercarriage damage dealt on a crossing. */
    UPROPERTY(EditAnywhere, Category = "GTC|SpikeStrip")
    double SpikeDamage = 22.0;

    /** Seconds a spent strip lingers after it's been hit. */
    UPROPERTY(EditAnywhere, Category = "GTC|SpikeStrip")
    double LingerAfterHitSeconds = 3.0;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|SpikeStrip", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    FVector StripA = FVector::ZeroVector;
    FVector StripB = FVector::ZeroVector;
    FVector PrevPlayerPos = FVector::ZeroVector;
    bool bHasPrev = false;
    bool bTriggered = false;
    double LifeRemaining = 20.0;

    APawn* ResolvePlayer() const;
};
