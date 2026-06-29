// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCBoatComponent.h"

#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "../../World/Environment/GTCOceanSubsystem.h"

UGTCBoatComponent::UGTCBoatComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // the pawn drives Step()

    // Default speedboat hull: 4 corners + bow + stern (boat-local cm, X forward / Y right).
    HullSamplesCm = {
        FVector2D(250.0, 120.0), FVector2D(250.0, -120.0),
        FVector2D(-250.0, 120.0), FVector2D(-250.0, -120.0),
        FVector2D(350.0, 0.0), FVector2D(-350.0, 0.0),
    };
}

void UGTCBoatComponent::ApplyHandlingTuning()
{
    FBoatHandling::FParams P;
    P.EngineAccel = EngineAccel;
    P.ForwardDragPlowing = ForwardDragPlowing;
    P.ForwardDragPlaning = ForwardDragPlaning;
    P.LateralDrag = LateralDrag;
    P.PlaningSpeed = PlaningSpeed;
    P.RudderAuthority = RudderAuthority;
    P.RudderFlowSpeed = RudderFlowSpeed;
    Boat.Configure(P);
}

FWatercraftFloat::FParams UGTCBoatComponent::FloatParams() const
{
    FWatercraftFloat::FParams P;
    P.DraftCm = DraftCm;
    P.CapsizeAngleRad = CapsizeAngleRad;
    P.RoughnessRefCm = RoughnessRefCm;
    return P;
}

void UGTCBoatComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyHandlingTuning();
}

void UGTCBoatComponent::Step(double Dt)
{
    // Throttle (-1..1, reverse) + steer (the yaw axis); planing + anisotropic drag in the core.
    Boat.Update(Controls.Throttle, Controls.YawAxis, Dt);
}

void UGTCBoatComponent::IntegrateVertical(FVector& InOutLocCm, double Dt)
{
    UWorld* World = GetWorld();
    if (World == nullptr || HullSamplesCm.Num() == 0)
    {
        return; // keep the pawn's current Z
    }
    const UGTCOceanSubsystem* Ocean = World->GetSubsystem<UGTCOceanSubsystem>();
    if (Ocean == nullptr)
    {
        return;
    }

    // Place each hull point in the world from the new planar position + heading, sample the
    // surface there, and fit the resting pose.
    const double H = Boat.Heading();
    const double C = FMath::Cos(H);
    const double S = FMath::Sin(H);
    const int32 N = HullSamplesCm.Num();

    TArray<FWatercraftFloat::FHullSample> Samples;
    TArray<double> Heights;
    Samples.Reserve(N);
    Heights.Reserve(N);
    for (const FVector2D& Local : HullSamplesCm)
    {
        const double Wx = InOutLocCm.X + C * Local.X - S * Local.Y;
        const double Wy = InOutLocCm.Y + S * Local.X + C * Local.Y;
        Heights.Add(Ocean->HeightAtCm(Wx, Wy));
        Samples.Add(FWatercraftFloat::FHullSample{Local.X, Local.Y});
    }

    CachedPose = FWatercraftFloat::ResolvePose(Heights.GetData(), Samples.GetData(), N, FloatParams());
    InOutLocCm.Z = CachedPose.Zcm;
}

FRotator UGTCBoatComponent::DesiredAttitude() const
{
    return FRotator(
        FMath::RadiansToDegrees(CachedPose.PitchRad),
        FMath::RadiansToDegrees(Boat.Heading()),
        FMath::RadiansToDegrees(CachedPose.RollRad));
}

float UGTCBoatComponent::CameraSpeed() const
{
    return static_cast<float>(FMath::Abs(Boat.ForwardSpeed()));
}
