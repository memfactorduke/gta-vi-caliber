// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/Interactable.h"
#include "../../Vehicles/Ownership/VehicleModShop.h"
#include "GTCModShop.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * AGTCModShop — a press-E tuning garage: buy the next performance/armor tier of a
 * vehicle category (engine/brakes/armor/tires) against the player's wallet. The price
 * tiers + stat multipliers are the pure, tested VehicleModShop; this actor resolves the
 * wallet and applies the spend.
 *
 * The owned VehicleModShop tracks ONE vehicle's upgrade levels (so repeated interacts
 * climb its tiers). Pushing the resulting multipliers onto the player's CURRENT vehicle
 * handling is deferred to the vehicle-ownership wiring — the new multipliers are logged
 * so the effect is observable headlessly meanwhile.
 */
UCLASS()
class GTC_UE5_API AGTCModShop : public AActor, public IGTCInteractable
{
    GENERATED_BODY()

public:
    AGTCModShop();

    // --- IGTCInteractable ---------------------------------------------------
    virtual void Interact_Implementation(AActor* Instigator) override;
    virtual bool CanInteract_Implementation(const AActor* Instigator) const override;
    virtual FText GetInteractPrompt_Implementation() const override;

    /** Buy the next tier of `Category` for Instigator. Returns true on a paid upgrade. */
    bool UpgradeCategory(AActor* Instigator, const FString& Category);

    /** Category upgraded by a single press-E interact (engine/brakes/armor/tires). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    FString DefaultCategory = TEXT("engine");

private:
    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<UStaticMeshComponent> Mesh;

    /** Pure tuning model — tracks this garage's one worked-on vehicle. */
    VehicleModShop ModShop;
};
