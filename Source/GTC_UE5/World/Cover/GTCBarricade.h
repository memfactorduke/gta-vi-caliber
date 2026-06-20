// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../NPC/Vitals/NpcVitals.h"
#include "GTCBarricade.generated.h"

class UStaticMeshComponent;

/**
 * AGTCBarricade — destructible cover. A solid barrier that blocks both pawns and the
 * line-of-sight channel, so it reads as real cover for everyone: the player ducks
 * behind it, and the AI's `FCombatCover` cover-search (which traces against geometry
 * on `ECC_Visibility`) picks the spot behind it. But it's not permanent — it soaks a
 * health pool (`FNpcVitals`) and, once shot to bits, drops its collision so it no
 * longer shields anyone. That makes firefights dynamic: grind down the enemy's cover,
 * or watch yours crumble.
 *
 * Thin AActor; reuses the same `FNpcVitals` damage model as the pawns, so the player's
 * existing weapon hitscan chips it with zero new wiring.
 */
UCLASS()
class GTC_UE5_API AGTCBarricade : public AActor
{
    GENERATED_BODY()

public:
    AGTCBarricade();

    virtual void BeginPlay() override;

    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable, Category = "GTC|Cover")
    bool IsBroken() const { return bBroken; }

    UFUNCTION(BlueprintCallable, Category = "GTC|Cover")
    float GetHealthFraction() const;

    /** Fired once when the barrier is destroyed — seam for the shatter/debris FX. The
     *  C++ has already dropped its collision so it no longer provides cover. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Cover")
    void OnBroken();

protected:
    /** Punishment the barrier soaks before it gives way. */
    UPROPERTY(EditAnywhere, Category = "GTC|Cover")
    double MaxHealth = 180.0;

    /** Seconds the broken rubble lingers before despawning (0 = stay forever). */
    UPROPERTY(EditAnywhere, Category = "GTC|Cover")
    double RubbleLingerSeconds = 6.0;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Cover", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Mesh;

    FNpcVitals Vitals;
    bool bBroken = false;

    void EnterBroken();
};
