// Copyright Epic Games, Inc. All Rights Reserved.

#include "FixedWingFlight.h"

void FFixedWingFlight::Configure(const FParams& InParams)
{
    Params = InParams;
    AirspeedValue = 0.0;
    ClimbRateValue = 0.0;
    HeadingRad = 0.0;
}

void FFixedWingFlight::Update(double Throttle, double Elevator, double Aileron, double Dt)
{
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double Thr = FMath::Clamp(Throttle, 0.0, 1.0);
    const double Elev = FMath::Clamp(Elevator, -1.0, 1.0);
    const double Ail = FMath::Clamp(Aileron, -1.0, 1.0);

    // Airspeed: thrust against drag, plus the elevator trade (climb bleeds speed, dive adds it).
    AirspeedValue += (Thr * FMath::Max(0.0, Params.MaxThrustAccel) - FMath::Max(0.0, Params.Drag) * AirspeedValue) * Step;
    AirspeedValue -= Elev * FMath::Max(0.0, Params.ClimbAirspeedCost) * Step;
    AirspeedValue = FMath::Max(0.0, AirspeedValue);

    // Control authority needs flow over the surfaces.
    const double Cruise = FMath::Max<double>(KINDA_SMALL_NUMBER, Params.CruiseSpeed);
    const double Authority = FMath::Clamp(AirspeedValue / Cruise, 0.0, 1.0);

    if (AirspeedValue < Params.StallSpeed)
    {
        // Stalled: wings aren't flying — drop, controls washed out.
        ClimbRateValue = -FMath::Max(0.0, Params.StallSinkRate);
    }
    else
    {
        ClimbRateValue = Elev * FMath::Max(0.0, Params.ClimbAuthority) * Authority;
        HeadingRad += Ail * Params.TurnAuthority * Authority * Step;
    }
}

FVector FFixedWingFlight::Velocity() const
{
    return FVector(FMath::Cos(HeadingRad) * AirspeedValue,
                   FMath::Sin(HeadingRad) * AirspeedValue,
                   ClimbRateValue);
}
