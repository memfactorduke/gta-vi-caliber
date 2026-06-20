// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCModShop.h"

#include "GTCShopBridge.h"
#include "../../Player/Stats/PlayerStats.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGTCModShop::AGTCModShop()
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

void AGTCModShop::Interact_Implementation(AActor* Instigator)
{
    UpgradeCategory(Instigator, DefaultCategory);
}

bool AGTCModShop::CanInteract_Implementation(const AActor* /*Instigator*/) const
{
    return true;
}

FText AGTCModShop::GetInteractPrompt_Implementation() const
{
    return NSLOCTEXT("GTCShop", "Tune", "Press E to tune");
}

bool AGTCModShop::UpgradeCategory(AActor* Instigator, const FString& Category)
{
    UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(Instigator);
    if (Stats == nullptr)
    {
        UE_LOG(LogGtcShop, Warning, TEXT("ModShop: no player wallet found."));
        return false;
    }

    // VehicleModShop::Upgrade MUTATES the installed tier on success, so snapshot first
    // and roll back if the spend is rejected — the model and the wallet must move
    // together (an installed tier with no payment would be a free, persisted upgrade).
    // The rollback branch is unreachable today (same single-threaded wallet, same
    // balance fed to Upgrade) but keeps the two atomic under any future wallet change.
    const TArray<TPair<FString, int32>> Before = ModShop.Serialize();

    const VehicleModShop::FModUpgradeResult R = ModShop.Upgrade(Category, Stats->GetMoney());
    if (!R.bSuccess)
    {
        UE_LOG(LogGtcShop, Display, TEXT("ModShop: upgrade '%s' denied — %s"), *Category, *R.Reason);
        return false;
    }

    if (!Stats->SpendMoney(R.Cost))
    {
        ModShop.Restore(Before); // undo the tier so it isn't a free, persisted upgrade
        UE_LOG(LogGtcShop, Warning, TEXT("ModShop: wallet rejected spend of %d for '%s'; upgrade rolled back."),
            R.Cost, *Category);
        return false;
    }

    UE_LOG(LogGtcShop, Display,
        TEXT("ModShop: '%s' -> level %d for %d; speed x%.2f grip x%.2f armor x%.2f; balance %d."),
        *Category, R.NewLevel, R.Cost, ModShop.TopSpeedMultiplier(), ModShop.GripMultiplier(),
        ModShop.ArmorMultiplier(), Stats->GetMoney());
    return true;
}
