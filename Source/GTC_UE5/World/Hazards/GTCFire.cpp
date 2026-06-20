// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCFire.h"

#include "Components/PointLightComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    constexpr double MetresToCm = 100.0;
}

AGTCFire::AGTCFire()
{
    PrimaryActorTick.bCanEverTick = true;

    // The light is the root + the visible flame glow.
    Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
    Glow->SetLightColor(FLinearColor(1.0f, 0.45f, 0.1f));
    Glow->SetAttenuationRadius(500.0f);
    Glow->SetIntensity(4000.0f);
    SetRootComponent(Glow);
}

void AGTCFire::Ignite(double StartIntensity, double FuelLoad, AController* InIgniter, int32 InGeneration)
{
    Cell.Ignite(StartIntensity, FuelLoad);
    Igniter = InIgniter;
    Generation = InGeneration;
    Rng.Initialize(static_cast<int32>(GetUniqueID()));
    SpreadTimer = SpreadIntervalSec;
}

void AGTCFire::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    Cell.Tick(static_cast<double>(DeltaSeconds));
    if (Glow != nullptr)
    {
        Glow->SetIntensity(static_cast<float>(4000.0 * Cell.Intensity));
    }

    BurnOverlapping(DeltaSeconds);
    TrySpread(DeltaSeconds);

    if (!Cell.IsBurning())
    {
        Destroy();
    }
}

void AGTCFire::BurnOverlapping(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    const double Dps = FFirePropagation::DamagePerSecond(Cell.Intensity);
    if (World == nullptr || Dps <= 0.0)
    {
        return;
    }

    const FVector Center = GetActorLocation();
    TArray<FOverlapResult> Overlaps;
    World->OverlapMultiByChannel(
        Overlaps, Center, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(static_cast<float>(DamageRadiusCm)));

    const float Damage = static_cast<float>(Dps * DeltaSeconds);
    TSet<AActor*> Burned;
    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Actor = O.GetActor();
        if (Actor == nullptr || Actor == this || Burned.Contains(Actor))
        {
            continue;
        }
        Burned.Add(Actor);
        UGameplayStatics::ApplyPointDamage(
            Actor, Damage, FVector::UpVector, FHitResult(), Igniter.Get(), this, nullptr);
    }
}

void AGTCFire::TrySpread(float DeltaSeconds)
{
    SpreadTimer -= static_cast<double>(DeltaSeconds);
    if (SpreadTimer > 0.0 || Generation >= MaxGeneration || !Cell.CanSpread())
    {
        return;
    }
    SpreadTimer = SpreadIntervalSec;

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // Try to jump to one flammable spot within the spread radius.
    const double DistM = FMath::Lerp(1.0, FFirePropagation::SpreadRadiusM, Rng.FRand());
    if (!FFirePropagation::CanIgnite(Cell.Intensity, DistM, GroundFlammability))
    {
        return;
    }
    const double Angle = Rng.FRand() * UE_DOUBLE_TWO_PI;
    const FVector Point = GetActorLocation()
        + FVector(FMath::Cos(Angle) * DistM * MetresToCm, FMath::Sin(Angle) * DistM * MetresToCm, 0.0);

    const double StartIntensity = FFirePropagation::SpreadIntensity(Cell.Intensity, DistM);
    TSubclassOf<AGTCFire> Class = SpreadClass;
    if (!Class)
    {
        Class = GetClass();
    }
    SpawnFire(World, Point, StartIntensity, SpreadFuelLoad, Igniter.Get(), Generation + 1, Class);
}

AGTCFire* AGTCFire::SpawnFire(
    UWorld* World, const FVector& Location, double StartIntensity, double FuelLoad,
    AController* InIgniter, int32 InGeneration, TSubclassOf<AGTCFire> Class)
{
    if (World == nullptr || StartIntensity <= 0.0)
    {
        return nullptr;
    }
    TSubclassOf<AGTCFire> SpawnClass = Class;
    if (!SpawnClass)
    {
        SpawnClass = AGTCFire::StaticClass();
    }
    const FTransform Xform(FRotator::ZeroRotator, Location);
    AGTCFire* Fire = World->SpawnActor<AGTCFire>(SpawnClass, Xform);
    if (Fire != nullptr)
    {
        Fire->Ignite(StartIntensity, FuelLoad, InIgniter, InGeneration);
    }
    return Fire;
}
