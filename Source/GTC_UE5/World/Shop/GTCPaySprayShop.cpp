// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPaySprayShop.h"

#include "GTCShopBridge.h"
#include "../../Player/Stats/PlayerStats.h"
#include "../../Systems/Wanted/WantedSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGTCPaySprayShop::AGTCPaySprayShop()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }
}

void AGTCPaySprayShop::Interact_Implementation(AActor* Instigator)
{
    Respray(Instigator);
}

bool AGTCPaySprayShop::CanInteract_Implementation(const AActor* /*Instigator*/) const
{
    return true;
}

FText AGTCPaySprayShop::GetInteractPrompt_Implementation() const
{
    return NSLOCTEXT("GTCShop", "Respray", "Press E to respray");
}

bool AGTCPaySprayShop::Respray(AActor* Instigator)
{
    UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(Instigator);
    if (Stats == nullptr)
    {
        UE_LOG(LogGtcShop, Warning, TEXT("PaySpray: no player wallet found."));
        return false;
    }

    UWantedSubsystem* Wanted = GTCShop::FindWanted(GetWorld());
    const int32 Stars = Wanted ? Wanted->Stars() : 0;

    // bSeen needs live police positions (IsSeenEntering) — that source arrives with the
    // police-pursuit system; until then a respray is never traced.
    const bool bSeen = false;

    const PaySpray::FResolveResult R = Spray.Resolve(Stars, Stats->GetMoney(), bSeen, BasePrice, PerStarPrice);
    if (!R.bAllowed)
    {
        UE_LOG(LogGtcShop, Display, TEXT("PaySpray: denied — %s"), *R.Reason);
        return false;
    }

    if (R.Cost > 0 && !Stats->SpendMoney(R.Cost))
    {
        UE_LOG(LogGtcShop, Warning, TEXT("PaySpray: wallet rejected spend of %d."), R.Cost);
        return false;
    }

    if (Wanted != nullptr)
    {
        Wanted->Clear();
    }
    UE_LOG(LogGtcShop, Display, TEXT("PaySpray: resprayed (was %d star) for %d; wanted cleared; balance %d."),
        Stars, R.Cost, Stats->GetMoney());
    return true;
}
