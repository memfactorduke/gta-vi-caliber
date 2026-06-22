// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/Interactable.h"      // IGTCInteractable contract this shop fulfils
#include "../../Systems/Economy/ShopModel.h"  // pure catalogue/purchase model the front wraps
#include "GTCShop.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;

/**
 * FShopFront — the pure, scene-free "storefront" logic for a walk-up shop.
 * Extracted as a plain non-UObject helper (the repo's "testable logic lives off
 * the actor" convention, mirroring FDoorSwing) so it unit-tests headless
 * (Tests/ShopFrontTest.cpp). AGTCShop is the thin UE adapter that registers the
 * front as a point-of-interest and drives a purchase from the player's wallet.
 *
 * The front owns the shop's identity — its place-registry Kind and prompt — and
 * composes the already-tested ShopModel for the catalogue + purchase resolution.
 * No UE world access: a buy resolves against a balance the caller passes in.
 */
class GTC_UE5_API FShopFront
{
public:
    /**
     * The canonical UGTCPlaceRegistrySubsystem kind every shop registers and is
     * queried as. Place "kinds" are free FName strings, so this constant is what
     * makes "shop" a first-class destination NPCs/crowds can FindNearest on,
     * alongside home/diner/bar/park/office/gym/street/restroom.
     */
    static FName PlaceKind();

    /** Default-stocked front (uses ShopModel's built-in catalogue). */
    FShopFront();

    /** Front over a specific catalogue (an empty list falls back to the default). */
    explicit FShopFront(const TArray<FShopCatalogueItem>& Catalogue);

    /** Context-sensitive interact prompt ("Press E to shop"). */
    FText Prompt() const;

    /** True when `Balance` covers `ItemId`'s price (unknown id is never affordable). */
    bool CanAfford(const FString& ItemId, int32 Balance) const;

    /** Resolve a purchase of `ItemId` against `Balance`; never mutates state. */
    FShopPurchase Buy(const FString& ItemId, int32 Balance) const;

    /** The underlying catalogue model (for queries / UI). */
    const ShopModel& Catalogue() const { return Model; }

private:
    ShopModel Model;
};

/**
 * AGTCShop — a press-E walk-up shop. Implements IGTCInteractable so the player's
 * UGTCInteractionComponent gathers it and, on IA_Interact, calls Interact() to buy.
 * On BeginPlay it registers itself as a "shop" point-of-interest with the world's
 * UGTCPlaceRegistrySubsystem (mirroring AGTCPlaceMarker) so citizens treat its spot
 * as a real shop destination; it unregisters cleanly on EndPlay.
 *
 * The buy decision is the pure FShopFront; reading the player's wallet and applying
 * the spend are the live adapter boundary (ResolveWalletBalance / ApplyPurchase),
 * PIE-deferred exactly like AGTCDoor's overlap/Interact wiring. GTC-prefixed per the
 * repo's interactable naming convention; the storefront body is a placeholder mesh
 * referenced by path (no new binary asset — the LFS budget is frozen).
 */
UCLASS()
class GTC_UE5_API AGTCShop : public AActor, public IGTCInteractable
{
    GENERATED_BODY()

public:
    AGTCShop();

    // --- IGTCInteractable ---------------------------------------------------
    /** Buy DefaultItemId from the interacting player's wallet via FShopFront. */
    virtual void Interact_Implementation(AActor* Instigator) override;
    /** Interactable unless the shop is closed. */
    virtual bool CanInteract_Implementation(const AActor* Instigator) const override;
    /** "Press E to shop". */
    virtual FText GetInteractPrompt_Implementation() const override;

    /** Place-registry kind this shop registers as (defaults to FShopFront::PlaceKind()). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    FName Kind;

    /** Optional occupancy cap passed to the registry (0 = unlimited). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    int32 Capacity = 0;

    /** Item bought on a simple interact (id into the ShopModel catalogue). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    FString DefaultItemId = TEXT("pistol");

    /** When true the shop is shut — CanInteract returns false. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    bool bClosed = false;

    /**
     * Stand-in wallet balance used by the default ResolveWalletBalance. The live
     * wiring (read PlayerStats money) overrides ResolveWalletBalance, so this only
     * matters until that PIE-side adapter lands. EditAnywhere for prototyping.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    int32 PreviewWalletBalance = 0;

    /** Optional storefront mesh override; a basic placeholder body is used if unset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Shop")
    TObjectPtr<UStaticMesh> StorefrontMesh;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /**
     * The interacting player's spendable balance. Default returns PreviewWalletBalance;
     * the live PlayerStats-backed wiring overrides this (PIE-deferred adapter seam).
     */
    virtual int32 ResolveWalletBalance(const AActor* Instigator) const;

    /**
     * Apply a resolved purchase (deduct money, refresh HUD). Default logs the outcome;
     * the live wiring overrides to spend from PlayerStats (PIE-deferred adapter seam).
     */
    virtual void ApplyPurchase(AActor* Instigator, const FShopPurchase& Result);

private:
    /** Pure storefront logic (prompt, affordability, purchase). */
    FShopFront ShopFront;

    /** Registry handle, so EndPlay removes exactly this shop. */
    int32 PlaceHandle = 0;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "GTC|Shop")
    TObjectPtr<UStaticMeshComponent> Body;
};
