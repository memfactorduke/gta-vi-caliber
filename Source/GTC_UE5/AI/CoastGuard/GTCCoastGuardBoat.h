// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "GTCCoastGuardBoat.generated.h"

class UGTCBoatComponent;
class UGTCWeaponComponent;
class UStaticMeshComponent;

/**
 * AGTCCoastGuardBoat — the naval pursuer that EMBODIES FNavalPursuit, the waterborne sibling
 * of AGTCPoliceHelicopter. It REUSES the player's UGTCBoatComponent for motion + buoyancy
 * (the concrete payoff of the locomotion-strategy split), steers it toward the
 * FNavalPursuit lead-intercept, fires a mounted gun when it has a clear shot, and reports a
 * kill to the wanted system if it brings the player down. Spawned/recalled by
 * AGTCCoastGuardDirector when the player flees by sea at high heat.
 *
 * Kinematic, no Chaos: it integrates the boat component's velocity into SetActorLocation
 * itself, exactly like the police chopper. Gate-deferred UObject; FNavalPursuit + the boat
 * cores are shim-verified + GTC.* tested.
 */
UCLASS()
class GTC_UE5_API AGTCCoastGuardBoat : public AActor
{
    GENERATED_BODY()

public:
    AGTCCoastGuardBoat();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "GTC|CoastGuard") bool IsDead() const { return bDead; }
    UFUNCTION(BlueprintCallable, Category = "GTC|CoastGuard") float GetHealthFraction() const;

    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Fired once when sunk — the seam for the explosion/smoke/sink FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|CoastGuard")
    void OnSunk();

protected:
    UPROPERTY(EditAnywhere, Category = "GTC|CoastGuard") double FireRangeCm = 5000.0;
    UPROPERTY(EditAnywhere, Category = "GTC|CoastGuard") double RamRangeCm = 600.0;
    UPROPERTY(EditAnywhere, Category = "GTC|CoastGuard") double MaxHealth = 500.0;
    UPROPERTY(EditAnywhere, Category = "GTC|CoastGuard") double WreckLingerSeconds = 8.0;
    /** How hard it steers toward the intercept heading (rad error -> steer, clamped). */
    UPROPERTY(EditAnywhere, Category = "GTC|CoastGuard") double SteerGain = 1.5;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|CoastGuard", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, Category = "GTC|CoastGuard", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCBoatComponent> Boat;

    UPROPERTY(VisibleAnywhere, Category = "GTC|CoastGuard", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCWeaponComponent> Weapon;

    FNpcVitals Vitals;
    bool bDead = false;
    double FireTimer = 0.0;
    int32 CachedStars = 0;

    APawn* ResolveTarget() const;
    int32 ReadStars() const;
    void EnterDeath(bool bByPlayer);
    void ReportCrimeToWanted(bool bKilled) const;
};
