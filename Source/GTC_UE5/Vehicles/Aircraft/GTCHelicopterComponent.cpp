// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCHelicopterComponent.h"

#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

UGTCHelicopterComponent::UGTCHelicopterComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; // inert until StartFlight
}

void UGTCHelicopterComponent::SetControls(float Collective, float CyclicPitch, float CyclicRoll, float Pedal)
{
    RawCollective = Collective;
    RawPitch = CyclicPitch;
    RawRoll = CyclicRoll;
    RawPedal = Pedal;
}

FHelicopterFlight::FParams UGTCHelicopterComponent::FlightParams() const
{
    FHelicopterFlight::FParams P;
    P.Gravity = Gravity;
    P.MaxLiftAccel = MaxLiftAccel;
    P.TiltAccel = TiltAccel;
    P.YawRatePerSec = YawRatePerSec;
    P.LinearDrag = LinearDrag;
    return P;
}

AActor* UGTCHelicopterComponent::ResolveAircraft() const
{
    if (Aircraft)
    {
        return Aircraft;
    }
    return GetOwner();
}

void UGTCHelicopterComponent::StartFlight()
{
    Flight.Configure(FlightParams());
    GroundZ = WorldFloorZ;
    RawCollective = RawPitch = RawRoll = RawPedal = 0.0f;
    bFlying = true;
    bGrounded = false;
    SetComponentTickEnabled(true);
}

void UGTCHelicopterComponent::StopFlight()
{
    bFlying = false;
    SetComponentTickEnabled(false);
}

void UGTCHelicopterComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bFlying || DeltaTime <= 0.0f)
    {
        return;
    }

    AActor* Chopper = ResolveAircraft();
    if (!Chopper)
    {
        return;
    }

    // Rotor model: the four controls -> a new world velocity + heading. Roll is NEGATED: the model's
    // Right basis is Godot-handed (the negation of UE's right vector), so feeding -roll makes "bank
    // right" actually slide the actor to its right.
    Flight.Update(RawCollective, RawPitch, -RawRoll, RawPedal, DeltaTime);

    // Integrate that velocity into a world position with a ground floor.
    FHelicopterFlightResolver::FInput In;
    In.Position = Chopper->GetActorLocation();
    In.Velocity = Flight.Velocity();
    In.GroundZ = GroundZ;
    In.Dt = DeltaTime;
    const FHelicopterFlightResolver::FOutput Out = FHelicopterFlightResolver::Integrate(In);

    // On the deck, stop the model from winding up downward velocity against the clamp, so raising
    // the collective lifts off cleanly instead of after a delay.
    if (Out.bGrounded)
    {
        Flight.HaltDescent();
    }

    // Fly the (kinematic) actor there, yawing to the flight heading but leaving pitch/roll to the
    // BP/anim (so authored bank/tilt isn't clobbered); the sweep handles obstacles.
    const double HeadingRad = Flight.Heading();
    const FRotator Current = Chopper->GetActorRotation();
    Chopper->SetActorLocation(Out.Position, /*bSweep*/ true);
    Chopper->SetActorRotation(FRotator(Current.Pitch, (float)FMath::RadiansToDegrees(HeadingRad), Current.Roll));

    // Publish telemetry for the HUD (heading normalized to [0,360)).
    double HeadingDeg = FMath::Fmod(FMath::RadiansToDegrees(HeadingRad), 360.0);
    if (HeadingDeg < 0.0)
    {
        HeadingDeg += 360.0;
    }
    AltitudeAgl = (float)Out.AltitudeAgl;
    ClimbRate = (float)Out.ClimbRateCmS;
    GroundSpeed = (float)Out.GroundSpeedCmS;
    HeadingDegrees = (float)HeadingDeg;
    bGrounded = Out.bGrounded;
}
