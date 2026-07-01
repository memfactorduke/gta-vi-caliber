// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCFixedWingComponent.h"

#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

UGTCFixedWingComponent::UGTCFixedWingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; // inert until StartFlight
}

void UGTCFixedWingComponent::SetControls(float Throttle, float Elevator, float Aileron)
{
    RawThrottle = Throttle;
    RawElevator = Elevator;
    RawAileron = Aileron;
}

FFixedWingFlight::FParams UGTCFixedWingComponent::FlightParams() const
{
    FFixedWingFlight::FParams P;
    P.MaxThrustAccel = MaxThrustAccel;
    P.Drag = Drag;
    P.StallSpeed = StallSpeed;
    P.StallSinkRate = StallSinkRate;
    P.CruiseSpeed = CruiseSpeed;
    P.ClimbAuthority = ClimbAuthority;
    P.ClimbAirspeedCost = ClimbAirspeedCost;
    P.TurnAuthority = TurnAuthority;
    return P;
}

AActor* UGTCFixedWingComponent::ResolveAircraft() const
{
    if (Aircraft)
    {
        return Aircraft;
    }
    return GetOwner();
}

void UGTCFixedWingComponent::StartFlight()
{
    Flight.Configure(FlightParams());
    GroundZ = WorldFloorZ;
    StallSpeedActive = StallSpeed;
    RawThrottle = RawElevator = RawAileron = 0.0f;
    bFlying = true;
    bGrounded = false;
    SetComponentTickEnabled(true);
}

void UGTCFixedWingComponent::StopFlight()
{
    bFlying = false;
    SetComponentTickEnabled(false);
}

void UGTCFixedWingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bFlying || DeltaTime <= 0.0f)
    {
        return;
    }

    AActor* Plane = ResolveAircraft();
    if (!Plane)
    {
        return;
    }

    // Flight model: throttle/elevator/aileron -> a new world velocity + heading.
    Flight.Update(RawThrottle, RawElevator, RawAileron, DeltaTime);

    // Integrate that velocity into a world position with a ground floor, plus stall telemetry.
    FFixedWingFlightResolver::FInput In;
    In.Position = Plane->GetActorLocation();
    In.Velocity = Flight.Velocity();
    In.GroundZ = GroundZ;
    In.Dt = DeltaTime;
    In.Airspeed = Flight.Airspeed();
    In.StallSpeed = StallSpeedActive; // snapshot — matches the model's own stall speed
    const FFixedWingFlightResolver::FOutput Out = FFixedWingFlightResolver::Resolve(In);

    // Fly the (kinematic) actor there, yawing to the flight heading but leaving pitch/roll to the
    // BP/anim (so authored bank/pitch isn't clobbered); the sweep handles obstacles.
    const double HeadingRad = Flight.Heading();
    const FRotator Current = Plane->GetActorRotation();
    Plane->SetActorLocation(Out.Position, /*bSweep*/ true);
    Plane->SetActorRotation(FRotator(Current.Pitch, (float)FMath::RadiansToDegrees(HeadingRad), Current.Roll));

    // Publish telemetry for the cockpit HUD (heading normalized to [0,360)).
    double HeadingDeg = FMath::Fmod(FMath::RadiansToDegrees(HeadingRad), 360.0);
    if (HeadingDeg < 0.0)
    {
        HeadingDeg += 360.0;
    }
    Airspeed = (float)Flight.Airspeed();
    AltitudeAgl = (float)Out.AltitudeAgl;
    ClimbRate = (float)Out.ClimbRateCmS;
    GroundSpeed = (float)Out.GroundSpeedCmS;
    HeadingDegrees = (float)HeadingDeg;
    StallMargin = (float)Out.StallMargin;
    bStalled = Out.bStalled;
    bGrounded = Out.bGrounded;
}
