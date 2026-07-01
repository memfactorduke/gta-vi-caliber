// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTrainComponent.h"

#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

UGTCTrainComponent::UGTCTrainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; // inert until StartLine
}

AActor* UGTCTrainComponent::ResolveTrain() const
{
    if (Train)
    {
        return Train;
    }
    return GetOwner();
}

void UGTCTrainComponent::StartLine()
{
    FTrainMover::FParams P;
    P.LineSpeed = LineSpeed;
    P.Accel = Accel;
    P.Brake = Brake;
    P.DwellSeconds = DwellSeconds;

    // The rail geometry defines the loop length so the 1-DOF distance maps 1:1 onto the spline;
    // fall back to the tuning value only when there's no usable geometry.
    const double RailLen = FRailPathResolver::LoopLength(RailPoints);
    P.TrackLength = (RailLen > 0.0) ? RailLen : (double)TrackLength;

    // Sanitize authored stations into the loop's [0, TrackLength) domain: a mis-scaled or out-of-range
    // stop would otherwise leave Position() outside its contract and could strand the train (the
    // controller snaps Pos = station on arrival). Array order is preserved (the train serves in order).
    TArray<double> SafeStations;
    SafeStations.Reserve(Stations.Num());
    for (double S : Stations)
    {
        double Wrapped = FMath::Fmod(S, P.TrackLength);
        if (Wrapped < 0.0)
        {
            Wrapped += P.TrackLength;
        }
        SafeStations.Add(Wrapped);
    }

    Mover.Configure(P, SafeStations);
    bRunning = true;
    bOnRail = false;
    SetComponentTickEnabled(true);
}

void UGTCTrainComponent::StopLine()
{
    bRunning = false;
    SetComponentTickEnabled(false);
}

void UGTCTrainComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bRunning || DeltaTime <= 0.0f)
    {
        return;
    }

    // Advance the 1-DOF controller (chase the braking-distance cap, arrive + dwell, wrap the loop).
    Mover.Advance(DeltaTime);

    // Publish controller telemetry.
    DistanceAlongLine = (float)Mover.Position();
    Speed = (float)Mover.Speed();
    DistanceToNextStop = (float)Mover.DistanceToNextStop();
    TargetStation = Mover.TargetStation();
    bDwelling = Mover.IsDwelling();

    // Place the (kinematic) actor on the rail at the current distance, facing the direction of travel.
    bOnRail = false;
    AActor* Rail = ResolveTrain();
    if (Rail && RailPoints.Num() >= 2)
    {
        const FRailPathResolver::FSample S = FRailPathResolver::Sample(RailPoints, Mover.Position());
        if (S.bValid)
        {
            // One kinematic teleport onto the exact rail transform (no sweep — the train follows the
            // spline, it doesn't collide its way along; TeleportPhysics keeps the body's state clean).
            Rail->SetActorLocationAndRotation(S.Position, S.Forward.Rotation(),
                /*bSweep*/ false, /*OutHit*/ nullptr, ETeleportType::TeleportPhysics);
            bOnRail = true;
        }
    }
}
