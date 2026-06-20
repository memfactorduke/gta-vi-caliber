// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/Interactable.h" // IGTCInteractable contract this vendor fulfils
#include "../../Systems/Economy/ShopModel.h"
#include "GTCStorefront.generated.h"

class USceneComponent;
class UStaticMeshComponent;
enum class EStorefrontGrantKind : uint8;

/**
 * AGTCStorefront — a press-E weapons/armor/vehicle vendor. Implements IGTCInteractable,
 * so the player's UGTCInteractionComponent gathers it (via its QueryAndPhysics mesh
 * overlap), shows its prompt, and on IA_Interact calls Interact() to buy DefaultItemId
 * against the player's wallet.
 *
 * The money decision + grant classification live in the pure, tested
 * FStorefrontTransaction over the existing ShopModel; this actor is the thin adapter
 * that resolves the player's UPlayerStatsComponent, applies the spend, and performs the
 * grant. Pre-UI scope: a single press-E buys one configured item; the multi-item Slate
 * buy-menu is the follow-up (it will call BuyItem per selection). Granting a
 * weapon/ammo/vehicle is logged where the player inventory isn't wired yet; an armor
 * purchase tops up the armor pool today, so the loop (money down, armor up) is real.
 */
UCLASS()
class GTC_UE5_API AGTCStorefront : public AActor, public IGTCInteractable
{
    GENERATED_BODY()

public:
    AGTCStorefront();

    // --- IGTCInteractable ---------------------------------------------------
    virtual void Interact_Implementation(AActor* Instigator) override;
    virtual bool CanInteract_Implementation(const AActor* Instigator) const override;
    virtual FText GetInteractPrompt_Implementation() const override;

    /**
     * Resolve and apply a purchase of `Id` for `Instigator`. Returns true on a
     * successful, paid-for buy. Public so the buy-menu UI follow-up (and the
     * GTC.Shop console) can drive the same path.
     */
    bool BuyItem(AActor* Instigator, const FString& Id);

    /** Catalogue item bought by a single press-E interact (pre buy-menu). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    FString DefaultItemId = TEXT("pistol");

protected:
    virtual void BeginPlay() override;

private:
    /** Hand the bought item to the player; armor applies now, others log until wired. */
    void GrantItem(AActor* Instigator, EStorefrontGrantKind Grant, const FString& ItemName);

    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<USceneComponent> Root;

    /** QueryAndPhysics mesh so the interaction overlap broad-phase finds this vendor. */
    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<UStaticMeshComponent> Mesh;

    /** The stock this vendor sells (default catalogue). */
    ShopModel Shop;

    /** Effective catalogue, for item Category/Name lookup by id. */
    TArray<FShopCatalogueItem> Catalogue;
};
