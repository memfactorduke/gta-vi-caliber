// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCExplosive.generated.h"

class UStaticMeshComponent;
class AGTCFire;

/**
 * AGTCExplosive — a shootable explosive world object (fuel barrel, gas tank) that
 * EMBODIES FExplosionModel: take enough damage and it detonates, dealing radial
 * damage + knockback to everything nearby and CHAIN-REACTING into neighbouring
 * explosives (FExplosionModel::ShouldChain) for the cascading barrel runs that
 * make a firefight feel destructible.
 *
 * Thin AActor over the tested blast core (FExplosionModel — frame-agnostic radial
 * damage/falloff; knockback re-biased to UE +Z here, as in AGTCThrowable). The
 * actor only holds hull health, runs the world sphere overlap, applies the
 * numbers, and triggers neighbours; the bDetonated guard makes chains terminate
 * (A triggers B, B's chain back to A is a no-op).
 *
 * The detonation carries the original instigator through the chain, so a barrel
 * the player shoots — and everything it sets off — credits the player (heat /
 * kills route through the standard TakeDamage path).
 */
UCLASS()
class GTC_UE5_API AGTCExplosive : public AActor
{
    GENERATED_BODY()

public:
    AGTCExplosive();

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Set it off now — from damage, a nearby blast, or a fire. Safe to call
     *  repeatedly (the first call wins); `ByInstigator` is credited for the kills.
     *  (Named ByInstigator, not Instigator, which UHT reserves on AActor.) */
    UFUNCTION(BlueprintCallable, Category = "GTC|Explosive")
    void Detonate(AController* ByInstigator = nullptr);

    /** Fired once at detonation — the seam for the fireball/smoke FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Explosive")
    void OnExplode(FVector Center, float RadiusCm);

protected:
    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    double BlastRadiusCm = 550.0;

    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    double MaxDamage = 140.0;

    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    double MaxImpulse = 110000.0;

    /** Hull health — a few rounds, or one nearby blast, sets it off. */
    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    double Health = 35.0;

    /** Reach (cm) within which this blast detonates other explosives. */
    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    double ChainTriggerRadiusCm = 650.0;

    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    bool bDrawDebugBlast = true;

    /** Leave a spreading fire where the explosive went off (the burning aftermath). */
    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    bool bLeavesFire = true;

    /** Fire patch class spawned as the aftermath (defaults to AGTCFire). */
    UPROPERTY(EditAnywhere, Category = "GTC|Explosive")
    TSubclassOf<AGTCFire> FireClass;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Explosive", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    bool bDetonated = false;
};
