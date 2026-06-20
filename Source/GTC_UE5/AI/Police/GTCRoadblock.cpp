// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCRoadblock.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "../Pursuit/Roadblock.h"

AGTCRoadblock::AGTCRoadblock()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void AGTCRoadblock::Setup(const FVector& FleeDir, double RoadWidth, int32 UnitCount, double LifeSeconds)
{
    FleeDirection = FleeDir.GetSafeNormal2D();
    if (FleeDirection.IsNearlyZero())
    {
        FleeDirection = GetActorForwardVector().GetSafeNormal2D();
    }
    LifeRemaining = LifeSeconds;

    // The barrier line runs perpendicular to the player's travel, across the road.
    const FVector RoadRight(-FleeDirection.Y, FleeDirection.X, 0.0);
    const FVector Center = GetActorLocation();
    const TArray<FVector> Positions = FRoadblock::UnitPositions(Center, RoadRight, RoadWidth, UnitCount);
    const float Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(RoadRight.Y, RoadRight.X)));

    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (const FVector& Pos : Positions)
    {
        UStaticMeshComponent* Barrier = NewObject<UStaticMeshComponent>(this);
        Barrier->RegisterComponent();
        Barrier->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
        if (Cube != nullptr)
        {
            Barrier->SetStaticMesh(Cube);
        }
        Barrier->SetCollisionProfileName(TEXT("BlockAll")); // solid wall
        Barrier->SetWorldLocationAndRotation(Pos, FRotator(0.0f, Yaw, 0.0f));
        // Cube long axis (X) spans the road; deep enough to ram, tall enough to stop a car.
        Barrier->SetWorldScale3D(FVector(BarrierWidthCm / 100.0, 1.0, 1.4));
        Barriers.Add(Barrier);
    }
}

APawn* AGTCRoadblock::ResolvePlayer() const
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

void AGTCRoadblock::Tick(float DeltaSeconds)
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

    // Retire the block once the player has threaded or rammed past the line.
    if (const APawn* Player = ResolvePlayer())
    {
        if (FRoadblock::HasPassed(Player->GetActorLocation(), GetActorLocation(), FleeDirection))
        {
            Destroy();
        }
    }
}
