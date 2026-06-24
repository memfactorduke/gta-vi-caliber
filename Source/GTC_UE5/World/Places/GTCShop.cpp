// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCShop.h"

#include "GTCPlaceRegistrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// --- FShopFront (pure, headless-tested) -------------------------------------

FName FShopFront::PlaceKind()
{
    return FName(TEXT("shop"));
}

FShopFront::FShopFront() = default;

FShopFront::FShopFront(const TArray<FShopCatalogueItem>& Catalogue)
    : Model(Catalogue)
{
}

FText FShopFront::Prompt() const
{
    return NSLOCTEXT("GTCShop", "Prompt", "Press E to shop");
}

bool FShopFront::CanAfford(const FString& ItemId, int32 Balance) const
{
    return Model.CanAfford(ItemId, Balance);
}

FShopPurchase FShopFront::Buy(const FString& ItemId, int32 Balance) const
{
    return Model.Purchase(ItemId, Balance);
}

// --- AGTCShop (the UE adapter) ----------------------------------------------

AGTCShop::AGTCShop()
{
    PrimaryActorTick.bCanEverTick = false;
    Kind = FShopFront::PlaceKind();

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Root);
    // Collision on so the interaction component's object-query overlap finds us.
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // A basic engine cube gives a freshly placed shop a visible body out of the box
    // without committing any new binary art (the asset budget is frozen); designers
    // swap StorefrontMesh to a real CityBeachStrip storefront per instance.
    // /Engine/BasicShapes/Cube always resolves.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaceholderBody(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (PlaceholderBody.Succeeded())
    {
        Body->SetStaticMesh(PlaceholderBody.Object);
    }
}

void AGTCShop::BeginPlay()
{
    Super::BeginPlay();

    if (StorefrontMesh != nullptr && Body != nullptr)
    {
        Body->SetStaticMesh(StorefrontMesh);
    }

    if (UWorld* World = GetWorld())
    {
        if (UGTCPlaceRegistrySubsystem* Places = World->GetSubsystem<UGTCPlaceRegistrySubsystem>())
        {
            PlaceHandle = Places->RegisterPlace(Kind, GetActorLocation(), Capacity);
        }
    }
}

void AGTCShop::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (PlaceHandle != 0)
    {
        if (UWorld* World = GetWorld())
        {
            if (UGTCPlaceRegistrySubsystem* Places = World->GetSubsystem<UGTCPlaceRegistrySubsystem>())
            {
                Places->UnregisterPlace(PlaceHandle);
            }
        }
        PlaceHandle = 0;
    }
    Super::EndPlay(EndPlayReason);
}

void AGTCShop::Interact_Implementation(AActor* Instigator)
{
    const int32 Balance = ResolveWalletBalance(Instigator);
    const FShopPurchase Result = ShopFront.Buy(DefaultItemId, Balance);
    ApplyPurchase(Instigator, Result);
}

bool AGTCShop::CanInteract_Implementation(const AActor* /*Instigator*/) const
{
    return !bClosed;
}

FText AGTCShop::GetInteractPrompt_Implementation() const
{
    return ShopFront.Prompt();
}

int32 AGTCShop::ResolveWalletBalance(const AActor* /*Instigator*/) const
{
    // Live wiring reads the player's PlayerStats money; until that PIE-side adapter
    // lands, fall back to the designer-set preview balance so the actor is usable.
    return PreviewWalletBalance;
}

void AGTCShop::ApplyPurchase(AActor* /*Instigator*/, const FShopPurchase& Result)
{
    // Live wiring deducts Result.Cost from PlayerStats and refreshes the HUD; the
    // headless build only logs so the seam is observable in PIE.
    UE_LOG(LogTemp, Log, TEXT("[GTCShop] buy '%s': %s (cost %d, balance %d)"),
        *DefaultItemId,
        Result.bSuccess ? TEXT("ok") : *Result.Reason,
        Result.Cost, Result.NewBalance);
}
