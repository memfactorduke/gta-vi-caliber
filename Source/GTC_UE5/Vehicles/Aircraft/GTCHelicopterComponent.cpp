// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCHelicopterComponent.h"

#include "Math/UnrealMathUtility.h"
#include "../Drive/VehicleAxisState.h"

UGTCHelicopterComponent::UGTCHelicopterComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // the pawn drives Step() from its own tick
}

void UGTCHelicopterComponent::ApplyTuning()
{
    FHelicopterFlight::FParams P;
    P.Gravity = GravityCmS2;
    P.MaxLiftAccel = MaxLiftAccel;
    P.TiltAccel = TiltAccel;
    P.YawRatePerSec = YawRatePerSec;
    P.LinearDrag = LinearDrag;
    Flight.Configure(P);
}

void UGTCHelicopterComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyTuning();
    // Start at the hover setting so a freshly-spawned chopper holds its altitude.
    CollectiveSetting = Flight.HoverCollective();
}

void UGTCHelicopterComponent::Step(double Dt)
{
    // Collective is a held lever nudged up/down (FVehicleAxisState), not a momentary axis.
    CollectiveSetting = FVehicleAxisState::Integrate(
        CollectiveSetting, Controls.CollectiveRaise, CollectiveRatePerSec, Dt, 0.0, 1.0);
    Flight.Update(CollectiveSetting, Controls.PitchAxis, Controls.RollAxis, Controls.YawAxis, Dt);
}

FRotator UGTCHelicopterComponent::DesiredAttitude() const
{
    // The body tilts INTO the cyclic: nose down when pushing forward, bank toward the roll.
    const double YawDeg = FMath::RadiansToDegrees(Flight.Heading());
    const double PitchDeg = -Controls.PitchAxis * MaxVisualTiltDeg; // stick fwd (neg) -> nose down
    const double RollDeg = Controls.RollAxis * MaxVisualTiltDeg;
    return FRotator(PitchDeg, YawDeg, RollDeg);
}
