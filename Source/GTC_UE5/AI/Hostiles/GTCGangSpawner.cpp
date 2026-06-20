// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCGangSpawner.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"

#include "GTCHostile.h"
#include "GTCTurret.h"
#include "../../World/Cover/GTCBarricade.h"

AGTCGangSpawner::AGTCGangSpawner()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f; // streaming need not run every frame
}

void AGTCGangSpawner::BeginPlay()
{
    Super::BeginPlay();
    Rng.Initialize(Seed);
}

APawn* AGTCGangSpawner::GetPlayer() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }
    return nullptr;
}

bool AGTCGangSpawner::PlayerInRange() const
{
    const APawn* Player = GetPlayer();
    if (Player == nullptr)
    {
        return false;
    }
    return FVector::DistSquared(Player->GetActorLocation(), GetActorLocation())
        <= (ActivationRadius * ActivationRadius);
}

void AGTCGangSpawner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    // Prune the dead / destroyed.
    Members.RemoveAll([](const TWeakObjectPtr<AGTCHostile>& M)
    {
        const AGTCHostile* H = M.Get();
        return H == nullptr || H->IsActorBeingDestroyed() || H->IsDead();
    });

    RespawnTimer -= static_cast<double>(DeltaSeconds);
    if (RespawnTimer > 0.0)
    {
        return;
    }
    RespawnTimer = RespawnInterval;

    if (PlayerInRange() && Members.Num() < FMath::Max(1, GangSize))
    {
        TopUp();
    }

    // Keep the turf's turret standing while the player is near.
    if (bDeployTurret && PlayerInRange())
    {
        AGTCTurret* T = Turret.Get();
        const bool bAlive = T != nullptr && !T->IsActorBeingDestroyed() && !T->IsDead();
        if (!bAlive)
        {
            if (UWorld* World = GetWorld())
            {
                TSubclassOf<AGTCTurret> TurretCls = TurretClass;
                if (!TurretCls)
                {
                    TurretCls = AGTCTurret::StaticClass();
                }
                FVector Point = GetActorLocation();
                Point.Z += 40.0;
                Turret = World->SpawnActor<AGTCTurret>(TurretCls, FTransform(FRotator::ZeroRotator, Point));
            }
        }
    }

    // Fortify the turf with destructible barricades — once, when the player first nears.
    if (bDeployBarricades && !bSpawnedBarricades && PlayerInRange())
    {
        bSpawnedBarricades = true;
        if (UWorld* World = GetWorld())
        {
            TSubclassOf<AGTCBarricade> BarCls = BarricadeClass;
            if (!BarCls)
            {
                BarCls = AGTCBarricade::StaticClass();
            }
            const int32 Count = FMath::Max(0, BarricadeCount);
            for (int32 b = 0; b < Count; ++b)
            {
                const double Angle = Rng.FRandRange(0.0, 2.0 * PI);
                const double Radius = Rng.FRandRange(0.4, 0.9) * SpawnRadius;
                FVector Point =
                    GetActorLocation() + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
                if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
                {
                    FNavLocation Projected;
                    if (Nav->ProjectPointToNavigation(Point, Projected, FVector(500.0, 500.0, 600.0)))
                    {
                        Point = Projected.Location;
                    }
                }
                Point.Z += 60.0;
                const float Yaw = static_cast<float>(FMath::RadiansToDegrees(Angle));
                World->SpawnActor<AGTCBarricade>(BarCls, FTransform(FRotator(0.0f, Yaw, 0.0f), Point));
            }
        }
    }
}

void AGTCGangSpawner::TopUp()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    TSubclassOf<AGTCHostile> SpawnClass = HostileClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCHostile::StaticClass();
    }

    // One member per pass so the band trickles in rather than popping all at once.
    const double Angle = Rng.FRandRange(0.0, 2.0 * PI);
    const double Radius = Rng.FRandRange(0.3, 1.0) * SpawnRadius;
    FVector Point = GetActorLocation() + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);

    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(Point, Projected, FVector(500.0, 500.0, 600.0)))
        {
            Point = Projected.Location;
        }
    }
    Point.Z += 90.0; // capsule half-height

    const FTransform Xform(FRotator::ZeroRotator, Point);
    AGTCHostile* Thug = World->SpawnActorDeferred<AGTCHostile>(
        SpawnClass, Xform, this, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (Thug != nullptr)
    {
        const bool bMelee = Rng.FRand() < MeleeFraction;
        Thug->InitializeHostile(SpawnCounter++, bMelee);
        Thug->FinishSpawning(Xform);
        Members.Add(Thug);
    }
}
