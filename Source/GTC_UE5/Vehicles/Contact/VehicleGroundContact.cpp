// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleGroundContact.h"

#include "Math/UnrealMathUtility.h"

double FVehicleGroundContact::AltitudeAGL(double BodyZCm, double GroundZCm)
{
    return BodyZCm - GroundZCm;
}

double FVehicleGroundContact::RestZCm(double GroundZCm, const FParams& Params)
{
    return GroundZCm + FMath::Max(0.0, Params.GearHeightCm);
}

double FVehicleGroundContact::ImpactHardness01(double DescentSpeedCm, const FParams& Params)
{
    const double Soft = FMath::Max(0.0, Params.SoftLandingSpeedCm);
    // Crash threshold is at least the soft threshold, so the band [Soft, Crash] is never inverted.
    const double Crash = FMath::Max(Soft, Params.CrashLandingSpeedCm);
    const double Speed = FMath::Max(0.0, DescentSpeedCm);

    if (Speed <= Soft)
    {
        return 0.0;
    }
    if (Crash <= Soft)
    {
        // Degenerate band (Soft == Crash): anything past Soft is a full-hardness crash.
        return 1.0;
    }
    return FMath::Clamp((Speed - Soft) / (Crash - Soft), 0.0, 1.0);
}

FVehicleGroundContact::ETouchdown FVehicleGroundContact::ClassifyTouchdown(
    double DescentSpeedCm, const FParams& Params)
{
    const double Soft = FMath::Max(0.0, Params.SoftLandingSpeedCm);
    const double Crash = FMath::Max(Soft, Params.CrashLandingSpeedCm);
    const double Speed = FMath::Max(0.0, DescentSpeedCm);

    if (Speed <= Soft)
    {
        return ETouchdown::Soft;
    }
    if (Speed >= Crash)
    {
        return ETouchdown::Crash;
    }
    return ETouchdown::Hard;
}

FVehicleGroundContact::FResult FVehicleGroundContact::Evaluate(
    double BodyZCm, bool bGroundHit, double GroundZCm,
    double VerticalSpeedCm, EContact Prev, const FParams& Params)
{
    FResult Result;

    // No floor traced below us — definitively airborne, no clamp.
    if (!bGroundHit)
    {
        Result.Contact = EContact::Airborne;
        return Result;
    }

    const double RestZ = RestZCm(GroundZCm, Params);
    const double LiftMargin = FMath::Max(0.0, Params.LiftoffMarginCm);

    if (Prev == EContact::Grounded)
    {
        // Stay grounded until we climb clear of the resting height by the liftoff margin.
        if (BodyZCm > RestZ + LiftMargin)
        {
            Result.Contact = EContact::Airborne;
            return Result;
        }
        // Still settled: hold the body on the floor (no fresh touchdown — already down).
        Result.Contact = EContact::Grounded;
        Result.bClampToGround = true;
        Result.ClampZCm = RestZ;
        return Result;
    }

    // Was airborne: a touchdown happens the moment the body reaches/penetrates the
    // resting height. (Reaching the ground is the trigger; the descent speed only sets
    // how hard. A body that spawned below ground gets lifted to the rest height as a
    // zero-speed Soft set-down.)
    if (BodyZCm <= RestZ)
    {
        const double Descent = FMath::Max(0.0, -VerticalSpeedCm);
        Result.Contact = EContact::Grounded;
        Result.Touchdown = ClassifyTouchdown(Descent, Params);
        Result.ImpactHardness01 = ImpactHardness01(Descent, Params);
        Result.bClampToGround = true;
        Result.ClampZCm = RestZ;
        return Result;
    }

    // Above the resting height with a floor below — still flying.
    Result.Contact = EContact::Airborne;
    return Result;
}
