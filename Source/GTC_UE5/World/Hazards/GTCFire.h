// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../Weapons/Fire/FirePropagation.h"
#include "GTCFire.generated.h"

class UPointLightComponent;

/**
 * AGTCFire — one patch of spreading fire that EMBODIES FFirePropagation: a molotov
 * splash or a barrel's burning aftermath. It owns an FFireCell that grows on fuel
 * and dies out, deals damage-over-time to anyone standing in it, and jumps to new
 * patches nearby (CanIgnite) until the fuel runs low — the slow creeping aftermath
 * to FExplosionModel's instant blast.
 *
 * Thin AActor over the tested fire core (FFireCell tick + the static spread math).
 * Distances are metres in the core (-> *100 for cm here). Spread is generation- and
 * fuel-capped so a single splash can't burn the whole map. The igniter is carried
 * through so a fire the player started credits the player on its kills.
 */
UCLASS()
class GTC_UE5_API AGTCFire : public AActor
{
    GENERATED_BODY()

public:
    AGTCFire();

    virtual void Tick(float DeltaSeconds) override;

    /** Light this patch: starting intensity, fuel load, the controller credited for
     *  its damage, and how many spread-generations deep it already is. */
    void Ignite(double StartIntensity, double FuelLoad, AController* InIgniter, int32 InGeneration);

    /** Spawn + ignite a fire patch in one call. */
    static AGTCFire* SpawnFire(
        UWorld* World, const FVector& Location, double StartIntensity, double FuelLoad,
        AController* InIgniter, int32 InGeneration, TSubclassOf<AGTCFire> Class);

    /** Current fire intensity (0..1), for FX/debug. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Fire")
    float GetIntensity() const { return static_cast<float>(Cell.Intensity); }

protected:
    /** Radius (cm) within which the fire burns actors each tick. */
    UPROPERTY(EditAnywhere, Category = "GTC|Fire")
    double DamageRadiusCm = 170.0;

    /** Flammability (0..1) of the ground the fire tries to spread onto. */
    UPROPERTY(EditAnywhere, Category = "GTC|Fire")
    double GroundFlammability = 0.6;

    /** Deepest spread chain from the original splash (caps runaway fire). */
    UPROPERTY(EditAnywhere, Category = "GTC|Fire")
    int32 MaxGeneration = 3;

    /** Seconds between spread attempts. */
    UPROPERTY(EditAnywhere, Category = "GTC|Fire")
    double SpreadIntervalSec = 1.0;

    /** Fuel a freshly-lit neighbour patch gets. */
    UPROPERTY(EditAnywhere, Category = "GTC|Fire")
    double SpreadFuelLoad = 3.0;

    /** The class spread spawns (defaults to this class). */
    UPROPERTY(EditAnywhere, Category = "GTC|Fire")
    TSubclassOf<AGTCFire> SpreadClass;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Fire", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UPointLightComponent> Glow;

    FFirePropagation::FFireCell Cell;
    TWeakObjectPtr<AController> Igniter;
    int32 Generation = 0;
    double SpreadTimer = 0.0;
    FRandomStream Rng;

    void BurnOverlapping(float DeltaSeconds);
    void TrySpread(float DeltaSeconds);
};
