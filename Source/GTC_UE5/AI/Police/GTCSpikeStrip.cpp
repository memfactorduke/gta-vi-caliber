// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCSpikeStrip.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "../Pursuit/SpikeStrip.h"

AGTCSpikeStrip::AGTCSpikeStrip()
{
    PrimaryActorTick.bCanEverTick = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // a flat hazard, not a wall
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Mesh->SetStaticMesh(Cube);
    }
    SetRootComponent(Mesh);
}

void AGTCSpikeStrip::Setup(const FVector& FleeDir, double RoadWidth, double LifeSeconds)
{
    FVector Flee = FleeDir.GetSafeNormal2D();
    if (Flee.IsNearlyZero())
    {
        Flee = GetActorForwardVector().GetSafeNormal2D();
    }
    LifeRemaining = LifeSeconds;

    // Strip runs perpendicular to the player's travel (across the road).
    const FVector RoadRight(-Flee.Y, Flee.X, 0.0);
    FSpikeStrip::Endpoints(GetActorLocation(), RoadRight, RoadWidth, StripA, StripB);

    // Flat wide slab laid along the strip line.
    const float Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(RoadRight.Y, RoadRight.X)));
    SetActorRotation(FRotator(0.0f, Yaw, 0.0f));
    Mesh->SetWorldScale3D(FVector(RoadWidth / 100.0, 0.6, 0.12)); // wide, shallow, flat
}

APawn* AGTCSpikeStrip::ResolvePlayer() const
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

void AGTCSpikeStrip::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    LifeRemaining -= static_cast<double>(DeltaSeconds);
    if (LifeRemaining <= 0.0)
    {
        Destroy();
        return;
    }

    APawn* Player = ResolvePlayer();
    if (Player == nullptr)
    {
        return;
    }
    const FVector CurrPos = Player->GetActorLocation();

    if (bHasPrev && !bTriggered && FSpikeStrip::Crosses(PrevPlayerPos, CurrPos, StripA, StripB))
    {
        bTriggered = true;

        // Chassis damage on the crossing; the deflation effect is OnSpiked's to wire.
        const FVector ShotDir = (CurrPos - PrevPlayerPos).GetSafeNormal();
        UGameplayStatics::ApplyPointDamage(
            Player, static_cast<float>(SpikeDamage), ShotDir, FHitResult(), nullptr, this, nullptr);
        OnSpiked(Player);

        // A spent strip lingers only briefly.
        LifeRemaining = FMath::Min(LifeRemaining, LingerAfterHitSeconds);
    }

    PrevPlayerPos = CurrPos;
    bHasPrev = true;
}
