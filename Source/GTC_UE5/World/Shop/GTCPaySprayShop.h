// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/Interactable.h"
#include "../../Systems/PaySprayLoot/PaySpray.h"
#include "GTCPaySprayShop.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * AGTCPaySprayShop — the iconic pay-n-spray: a press-E shop that, for a star-scaled
 * fee, instantly clears the player's wanted level (IF no cop saw them duck in). The
 * money + eligibility decision is the pure, tested PaySpray model; this actor resolves
 * the wallet (UPlayerStatsComponent) and the heat (UWantedSubsystem), applies the spend,
 * and clears the heat.
 *
 * Police line-of-sight (PaySpray::IsSeenEntering) needs live officer positions, which
 * arrive with the police-pursuit system; until then no tail is modelled (bSeen=false) —
 * a documented seam, not a silent omission.
 */
UCLASS()
class GTC_UE5_API AGTCPaySprayShop : public AActor, public IGTCInteractable
{
    GENERATED_BODY()

public:
    AGTCPaySprayShop();

    // --- IGTCInteractable ---------------------------------------------------
    virtual void Interact_Implementation(AActor* Instigator) override;
    virtual bool CanInteract_Implementation(const AActor* Instigator) const override;
    virtual FText GetInteractPrompt_Implementation() const override;

    /** Resolve + apply a respray for Instigator. Returns true if the wanted level cleared. */
    bool Respray(AActor* Instigator);

    /** Flat respray fee added on top of the per-star surcharge. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    int32 BasePrice = 0;

    /** Per-star surcharge (higher heat costs more to wash off). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    int32 PerStarPrice = 100;

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<UStaticMeshComponent> Mesh;

    /** Pure respray model (eligibility + cost). */
    PaySpray Spray;
};
