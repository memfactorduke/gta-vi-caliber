// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelicopterFlight.h"

void FHelicopterFlight::Configure(const FParams& InParams)
{
    Params = InParams;
    Vel = FVector::ZeroVector;
    HeadingRad = 0.0;
}

double FHelicopterFlight::HoverCollective() const
{
    // Spell the type: KINDA_SMALL_NUMBER is a float and MaxLiftAccel a double, and FMath::Max has
    // no mixed-type overload (a bare call fails UBT template deduction).
    const double MaxLift = FMath::Max<double>(KINDA_SMALL_NUMBER, Params.MaxLiftAccel);
    return FMath::Clamp(FMath::Max(0.0, Params.Gravity) / MaxLift, 0.0, 1.0);
}

void FHelicopterFlight::Update(double Collective, double CyclicPitch, double CyclicRoll, double Pedal, double Dt)
{
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double Coll = FMath::Clamp(Collective, 0.0, 1.0);
    const double Pitch = FMath::Clamp(CyclicPitch, -1.0, 1.0);
    const double Roll = FMath::Clamp(CyclicRoll, -1.0, 1.0);
    const double Yaw = FMath::Clamp(Pedal, -1.0, 1.0);

    // Lift fights gravity: net vertical accel is zero at HoverCollective.
    const double VerticalAccel = Coll * FMath::Max(0.0, Params.MaxLiftAccel) - FMath::Max(0.0, Params.Gravity);

    // Cyclic tilts the thrust into the body frame: stick forward (negative pitch)
    // accelerates forward; bank right (positive roll) accelerates right.
    const double ForwardAccel = -Pitch * Params.TiltAccel;
    const double RightAccel = Roll * Params.TiltAccel;

    const double CosH = FMath::Cos(HeadingRad);
    const double SinH = FMath::Sin(HeadingRad);
    const FVector Forward(CosH, SinH, 0.0);
    const FVector Right(SinH, -CosH, 0.0); // 90 deg clockwise from forward in the XY plane

    FVector Accel = Forward * ForwardAccel + Right * RightAccel;
    Accel.Z += VerticalAccel;

    // Drag sets the terminal speeds (and bleeds momentum when you neutralize the sticks).
    Accel -= Vel * FMath::Max(0.0, Params.LinearDrag);

    Vel += Accel * Step;
    HeadingRad += Yaw * Params.YawRatePerSec * Step;
}
