// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "GTCTurret.generated.h"

class UStaticMeshComponent;
class UGTCWeaponComponent;

/**
 * AGTCTurret — a fixed gun emplacement: a fortified, immobile enemy that guards a
 * spot. It is the stationary cousin of AGTCHostile — same reused UGTCWeaponComponent
 * fired through the AI-aim override and the same FNpcVitals damage model — but it
 * never moves; it swivels a head to track the player and opens up once it has the
 * shot. Tough enough that you have to commit to silencing it.
 *
 * Thin AActor; no decision pure-core needed beyond range/LOS gating. Drop one on a
 * rooftop or a chokepoint, or have a faction place them.
 */
UCLASS()
class GTC_UE5_API AGTCTurret : public AActor
{
    GENERATED_BODY()

public:
    AGTCTurret();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "GTC|Turret")
    bool IsDead() const { return bDead; }

    UFUNCTION(BlueprintCallable, Category = "GTC|Turret")
    float GetHealthFraction() const;

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Fired once when destroyed — seam for the explosion FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Turret")
    void OnDestroyed();

protected:
    /** Tough — a fortified emplacement. */
    UPROPERTY(EditAnywhere, Category = "GTC|Turret")
    double MaxHealth = 240.0;

    /** Engagement range (metres). */
    UPROPERTY(EditAnywhere, Category = "GTC|Turret")
    double RangeM = 35.0;

    /** Seconds between bursts. */
    UPROPERTY(EditAnywhere, Category = "GTC|Turret")
    double FireCooldownSec = 0.5;

    /** Head swivel rate (deg/sec). */
    UPROPERTY(EditAnywhere, Category = "GTC|Turret")
    double TurnRateDeg = 160.0;

    /** Seconds the wreck lingers before despawning. */
    UPROPERTY(EditAnywhere, Category = "GTC|Turret")
    double WreckLingerSeconds = 8.0;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Turret", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Base;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Turret", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Head;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Turret", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCWeaponComponent> Weapon;

    FNpcVitals Vitals;
    bool bDead = false;
    double FireTimer = 0.0;

    APawn* ResolveTarget() const;
    bool HasLineOfSight(const FVector& From, const APawn* Target) const;
    void EnterDeath();
};
