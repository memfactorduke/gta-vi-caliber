// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCStorefront.h"

#include "GTCShopBridge.h"
#include "StorefrontTransaction.h"
#include "../../Player/Stats/PlayerStats.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGTCStorefront::AGTCStorefront()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);
    // Collision on so the interaction component's object-query overlap finds us.
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Engine basic shape as a content-free placeholder so a freshly placed vendor is
    // visible without project assets (designers swap the mesh per instance).
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }
}

void AGTCStorefront::BeginPlay()
{
    Super::BeginPlay();

    // Keep the catalogue alongside the model so a buy can recover an item's category
    // (grant kind) and display name by id — ShopModel only answers price/affordability.
    Catalogue = ShopModel::DefaultCatalogue();
    Shop = ShopModel(Catalogue);
}

void AGTCStorefront::Interact_Implementation(AActor* Instigator)
{
    BuyItem(Instigator, DefaultItemId);
}

bool AGTCStorefront::CanInteract_Implementation(const AActor* /*Instigator*/) const
{
    // Stay interactable even when broke: the buy reports affordability via its reason,
    // matching AGTCDoor which never hides its prompt.
    return true;
}

FText AGTCStorefront::GetInteractPrompt_Implementation() const
{
    return NSLOCTEXT("GTCShop", "Shop", "Press E to shop");
}

bool AGTCStorefront::BuyItem(AActor* Instigator, const FString& Id)
{
    UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(Instigator);
    if (Stats == nullptr)
    {
        UE_LOG(LogGtcShop, Warning, TEXT("Storefront: no player wallet found for buy '%s'."), *Id);
        return false;
    }

    const FShopCatalogueItem* Item =
        Catalogue.FindByPredicate([&Id](const FShopCatalogueItem& E) { return E.Id == Id; });
    if (Item == nullptr)
    {
        UE_LOG(LogGtcShop, Warning, TEXT("Storefront: unknown item '%s'."), *Id);
        return false;
    }

    const FStorefrontBuyDecision Decision = FStorefrontTransaction::ResolveBuy(Shop, *Item, Stats->GetMoney());
    if (!Decision.bSuccess)
    {
        UE_LOG(LogGtcShop, Display, TEXT("Storefront: buy '%s' denied — %s"), *Id, *Decision.Reason);
        return false;
    }

    // Apply the spend through the authoritative wallet, then grant. The balance fed to
    // ResolveBuy is GetMoney(), so this spend cannot under-run.
    if (!Stats->SpendMoney(Decision.Cost))
    {
        UE_LOG(LogGtcShop, Warning, TEXT("Storefront: wallet rejected spend of %d for '%s'."), Decision.Cost, *Id);
        return false;
    }

    GrantItem(Instigator, Decision.Grant, Decision.ItemName);
    UE_LOG(LogGtcShop, Display, TEXT("Storefront: bought '%s' (%s) for %d; balance %d."),
        *Decision.ItemName, FStorefrontTransaction::GrantKindName(Decision.Grant), Decision.Cost, Stats->GetMoney());
    return true;
}

void AGTCStorefront::GrantItem(AActor* Instigator, EStorefrontGrantKind Grant, const FString& ItemName)
{
    if (Grant == EStorefrontGrantKind::Armor)
    {
        if (UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(Instigator))
        {
            Stats->AddArmor(Stats->GetMaxArmor()); // body armor tops up the pool
            UE_LOG(LogGtcShop, Display, TEXT("Storefront: armor restored to %.0f."), Stats->GetArmor());
        }
        return;
    }

    // Weapon/ammo/vehicle inventories are owned by their own systems (the player weapon
    // component / vehicle ownership). Until those grant seams are connected the purchase
    // is recorded here — the money has already changed hands, so the loop stays honest.
    UE_LOG(LogGtcShop, Display, TEXT("Storefront: granted %s '%s' (inventory wiring pending)."),
        FStorefrontTransaction::GrantKindName(Grant), *ItemName);
}
