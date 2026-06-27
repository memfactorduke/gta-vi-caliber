// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleSpawnSettleComponent.h"

#include "../GTCTrafficSubsystem.h"

#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UVehicleSpawnSettleComponent::UVehicleSpawnSettleComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Reposition BEFORE the physics sub-step each frame, so the car is parked clear of
    // any just-streamed road before Chaos gets a chance to depenetrate-launch it. We
    // disable the tick the instant we settle, so the steady-state cost is zero.
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UVehicleSpawnSettleComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        SpawnZ = Owner->GetActorLocation().Z;
        bSpawnZRecorded = true;

        // Tell the traffic director this car occupies a lane, so the ambient streamer
        // never spawns a car on top of us. The settle logic above keeps the streaming
        // ROAD from launching the chassis; this keeps streaming TRAFFIC from doing the
        // same. Both are "don't let this car get ejected the instant something appears".
        if (const UWorld* World = Owner->GetWorld())
        {
            if (UGTCTrafficSubsystem* Traffic = World->GetSubsystem<UGTCTrafficSubsystem>())
            {
                Traffic->RegisterExternalVehicle(Owner);
            }
        }
    }
}

void UVehicleSpawnSettleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AActor* Owner = GetOwner())
    {
        if (const UWorld* World = Owner->GetWorld())
        {
            if (UGTCTrafficSubsystem* Traffic = World->GetSubsystem<UGTCTrafficSubsystem>())
            {
                Traffic->UnregisterExternalVehicle(Owner);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

FVehicleSpawnSettle::FParams UVehicleSpawnSettleComponent::Params() const
{
    FVehicleSpawnSettle::FParams P;
    P.HoldClearanceCm = HoldClearanceCm;
    P.SettleAboveGroundCm = SettleAboveGroundCm;
    P.MaxWaitSeconds = MaxWaitSeconds;
    return P;
}

void UVehicleSpawnSettleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bDone)
    {
        Finish();
        return;
    }

    AActor* Owner = GetOwner();
    UWorld* World = Owner ? Owner->GetWorld() : nullptr;
    if (!Owner || !World)
    {
        return;
    }

    if (!bSpawnZRecorded)
    {
        SpawnZ = Owner->GetActorLocation().Z;
        bSpawnZRecorded = true;
    }
    Elapsed += DeltaTime;

    // Trace straight down from a little above the chassis to well below it.
    const FVector Loc = Owner->GetActorLocation();
    const FVector Start = Loc + FVector(0.0, 0.0, TraceStartAboveCm);
    const FVector End = Loc - FVector(0.0, 0.0, TraceDistanceCm);

    FCollisionQueryParams QueryParams(TEXT("VehicleSpawnSettle"), /*bTraceComplex*/ false, Owner);
    FHitResult Hit;

    // Trace by OBJECT TYPE, not by trace channel. We only want to land on static world
    // geometry (the road / landscape, object type WorldStatic). A channel trace would stop
    // on anything that BLOCKS that channel — and in this project NPC capsules re-enable
    // ECC_Visibility so they are shootable (see AGTCCitizen) and block ECC_WorldStatic via
    // the default Pawn profile, so a channel trace could "find ground" on a pedestrian and
    // settle the car onto them. NPCs are object type Pawn (and the car itself is object type
    // Vehicle), so an object-type query for WorldStatic skips both and hits only the road.
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(GroundChannel.GetValue());
    const bool bGroundHit = World->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, QueryParams);

    const FVehicleSpawnSettle::FResult Result =
        FVehicleSpawnSettle::Evaluate(bGroundHit, Hit.ImpactPoint.Z, SpawnZ, Elapsed, Params());

    // Hold and Settle reposition the car; GiveUp just lets physics have it where it is.
    if (Result.Action != FVehicleSpawnSettle::EAction::GiveUp)
    {
        Owner->SetActorLocation(FVector(Loc.X, Loc.Y, Result.TargetZ),
            /*bSweep*/ false, /*OutSweepHitResult*/ nullptr, ETeleportType::TeleportPhysics);

        if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
        {
            if (Root->IsSimulatingPhysics())
            {
                // Kill any velocity the hold accrued, so the hand-off to physics is a
                // clean settle rather than a flung release.
                Root->SetPhysicsLinearVelocity(FVector::ZeroVector);
                Root->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
            }
        }
    }

    if (Result.bDone)
    {
        bDone = true;
        Finish();
    }
}

void UVehicleSpawnSettleComponent::Finish()
{
    SetComponentTickEnabled(false);
}
