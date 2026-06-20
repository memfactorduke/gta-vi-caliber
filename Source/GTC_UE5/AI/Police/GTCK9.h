// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "GTCK9.generated.h"

/**
 * AGTCK9 — a police dog. Fast, fragile, and relentless: it sprints down the player
 * and bites on a cooldown, so you can't simply outrun it on foot the way you can
 * shake a slower officer — you have to put it down or break contact. A lean
 * ACharacter over the shared FNpcVitals damage model (the player's hitscan drops it
 * with no new wiring); no gun, no decision pure-core needed beyond range gating.
 *
 * Spawned alongside officers as a low/mid-heat unit, or placed directly.
 */
UCLASS()
class GTC_UE5_API AGTCK9 : public ACharacter
{
    GENERATED_BODY()

public:
    AGTCK9();

    virtual void Tick(float DeltaSeconds) override;

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /** Stamp toughness before BeginPlay. */
    void InitializeK9(int32 Seed);

    UFUNCTION(BlueprintCallable, Category = "GTC|K9")
    bool IsDead() const { return bDead; }

    /** Fired on a bite — seam for the snarl/bite FX. Damage already applied. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|K9")
    void OnBite(APawn* Victim);

    /** Fired once when killed — body already ragdolling. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|K9")
    void OnKilled(const FVector& FromDirection);

protected:
    UPROPERTY(EditAnywhere, Category = "GTC|K9")
    double MaxHealth = 55.0;

    /** Sprint speed — faster than a person, so it runs you down. */
    UPROPERTY(EditAnywhere, Category = "GTC|K9")
    double RunSpeed = 720.0;

    UPROPERTY(EditAnywhere, Category = "GTC|K9")
    double BiteRangeM = 1.8;

    UPROPERTY(EditAnywhere, Category = "GTC|K9")
    double BiteDamage = 14.0;

    UPROPERTY(EditAnywhere, Category = "GTC|K9")
    double BiteCooldownSec = 1.0;

    UPROPERTY(EditAnywhere, Category = "GTC|K9")
    double CorpseLingerSeconds = 10.0;

private:
    FNpcVitals Vitals;
    bool bDead = false;
    double BiteTimer = 0.0;

    APawn* ResolveTarget() const;
    void FaceTarget(const FVector& TargetPos, float DeltaSeconds);
    void EnterDeath(const FVector& Travel);
};
