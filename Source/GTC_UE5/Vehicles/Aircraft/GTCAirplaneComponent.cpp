// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCAirplaneComponent.h"

#include "Math/UnrealMathUtility.h"
#include "../Drive/VehicleAxisState.h"
#include "../Attitude/VehicleAttitude.h"

UGTCAirplaneComponent::UGTCAirplaneComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // the pawn drives Step()
}

void UGTCAirplaneComponent::ApplyTuning()
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
    Flight.Configure(P);
}

void UGTCAirplaneComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyTuning();
}

void UGTCAirplaneComponent::Step(double Dt)
{
    // Throttle is a held lever nudged up/down; elevator/aileron are momentary.
    ThrottleSetting = FVehicleAxisState::Integrate(
        ThrottleSetting, Controls.Throttle, ThrottleRatePerSec, Dt, 0.0, 1.0);
    Flight.Update(ThrottleSetting, Controls.PitchAxis, Controls.RollAxis, Dt);
    // TODO(editor): clamp Flight's climb near the service ceiling via FAircraftAirspace
    // (the core ships + is tested; wiring needs the owner's live altitude + tuning).
}

FRotator UGTCAirplaneComponent::DesiredAttitude() const
{
    const double YawDeg = FMath::RadiansToDegrees(Flight.Heading());
    const double PitchRad = FVehicleAttitude::PitchFromVelocity(
        Flight.ClimbRate(), Flight.Airspeed(), MaxVisualPitchRad);
    const double BankDeg = Controls.RollAxis * MaxVisualBankDeg; // bank visibly into the roll input
    return FRotator(FMath::RadiansToDegrees(PitchRad), YawDeg, BankDeg);
}
