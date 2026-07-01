// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCVehicleHandlingComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"

UGTCVehicleHandlingComponent::UGTCVehicleHandlingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

FRoadGrip::FParams UGTCVehicleHandlingComponent::RoadParams() const
{
    FRoadGrip::FParams P;
    P.WetGripLoss = WetGripLoss;
    P.AquaplaneSpeedKmh = AquaplaneSpeedKmh;
    P.AquaplaneSpeedFullKmh = AquaplaneSpeedFullKmh;
    P.AquaplaneFloor = AquaplaneFloor;
    return P;
}

bool UGTCVehicleHandlingComponent::IsBeingDriven() const
{
    // The car pawn is possessed by a controller only while occupied (the seat component
    // possesses it on entry, un-possesses on exit). Parked/ambient cars have no controller.
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    return OwnerPawn != nullptr && OwnerPawn->GetController() != nullptr;
}

void UGTCVehicleHandlingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const AActor* Owner = GetOwner();
    if (!Owner || !IsBeingDriven())
    {
        return; // parked / unoccupied — a single check, no per-frame handling cost
    }

    FVehicleGripResolver::FInput In;
    In.Velocity = Owner->GetVelocity(); // bare-actor API, no Chaos dependency
    In.Forward = Owner->GetActorForwardVector();
    In.Right = Owner->GetActorRightVector();
    In.Wetness = Wetness;
    In.TyreGrip = TyreGripFactor;
    In.bHandbrake = bHandbrakeHeld;

    const FVehicleGripResolver::FOutput Out = FVehicleGripResolver::Resolve(In, RoadParams());

    // Publish for the car BP / HUD / FX.
    RoadGrip = (float)Out.RoadGrip;
    GripFactor = (float)Out.GripFactor;
    SlipAngleDegrees = (float)FMath::RadiansToDegrees(Out.SlipAngleRad);
    DriftFactor = (float)Out.DriftFactor;
    LateralRetention = (float)Out.LateralRetention;
    AquaplaneAmount = (float)Out.AquaplaneAmount;
    bAquaplaning = Out.bAquaplaning;
    GrippedVelocity = Out.GrippedVelocity;

    // Advance the stunt scorer with this frame's drift, scored in cm/s to match the chassis'
    // native units (holds the run when not sliding; zero drift in reverse never scores).
    const double GroundSpeedCms = Out.GroundSpeedKmh / FVehicleGripResolver::CmsToKmh;
    Scorer.Update(Out.DriftFactor, GroundSpeedCms, DeltaTime);
}
