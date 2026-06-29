// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCCoastGuardDirector.generated.h"

class AGTCCoastGuardBoat;
class APawn;

/**
 * AGTCCoastGuardDirector — streams the Coast Guard onto a player fleeing by sea, the naval
 * counterpart to AGTCPoliceDirector. Drop ONE in the persistent level. Each pass it reads
 * the wanted stars + the player's medium (GTCVehicleMedium) and asks FAirSeaEscalation how
 * many interceptors should be out: boats spawn offshore and converge, and stand down when
 * the player leaves the water or the heat clears.
 *
 * Gate-deferred UObject. The escalation + pursuit decisions are shim-verified + GTC.* tested.
 */
UCLASS()
class GTC_UE5_API AGTCCoastGuardDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTCCoastGuardDirector();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Interceptor to spawn. Defaults to AGTCCoastGuardBoat; a BP subclass may override it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|CoastGuard")
    TSubclassOf<AGTCCoastGuardBoat> BoatClass;

    /** How far offshore (cm) interceptors spawn from the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|CoastGuard")
    double SpawnRadiusCm = 9000.0;

    /** Seconds between streaming passes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|CoastGuard")
    double StreamIntervalSec = 1.0;

private:
    APawn* GetPlayerPawn() const;
    int32 ReadStars() const;
    void StreamCoastGuard();
    void Prune();

    UPROPERTY()
    TArray<TWeakObjectPtr<AGTCCoastGuardBoat>> Boats;

    double StreamAccum = 0.0;
    int32 SpawnCounter = 0;
};
