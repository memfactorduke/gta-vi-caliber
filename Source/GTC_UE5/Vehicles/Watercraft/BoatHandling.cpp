// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoatHandling.h"

namespace
{
    // Body-frame basis for a heading: forward and (90 deg clockwise) right, in the XY plane.
    void BodyAxes(double HeadingRad, FVector& OutForward, FVector& OutRight)
    {
        const double CosH = FMath::Cos(HeadingRad);
        const double SinH = FMath::Sin(HeadingRad);
        OutForward = FVector(CosH, SinH, 0.0);
        OutRight = FVector(SinH, -CosH, 0.0);
    }

    double Dot2D(const FVector& A, const FVector& B)
    {
        return A.X * B.X + A.Y * B.Y;
    }
}

void FBoatHandling::Configure(const FParams& InParams)
{
    Params = InParams;
    Vel = FVector::ZeroVector;
    HeadingRad = 0.0;
}

double FBoatHandling::ForwardSpeed() const
{
    FVector Forward, Right;
    BodyAxes(HeadingRad, Forward, Right);
    return Dot2D(Vel, Forward);
}

double FBoatHandling::LateralSpeed() const
{
    FVector Forward, Right;
    BodyAxes(HeadingRad, Forward, Right);
    return Dot2D(Vel, Right);
}

bool FBoatHandling::IsPlaning() const
{
    return ForwardSpeed() > Params.PlaningSpeed;
}

void FBoatHandling::Update(double Throttle, double Steer, double Dt)
{
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double Thr = FMath::Clamp(Throttle, -1.0, 1.0);
    const double Str = FMath::Clamp(Steer, -1.0, 1.0);

    FVector Forward, Right;
    BodyAxes(HeadingRad, Forward, Right);

    const double SurgeV = Dot2D(Vel, Forward);   // forward speed
    const double SwayV = Dot2D(Vel, Right);      // sideways slip

    // The hull climbs onto plane past the planing speed -> forward drag drops.
    const double FwdDrag = (SurgeV > Params.PlaningSpeed)
        ? FMath::Max(0.0, Params.ForwardDragPlaning)
        : FMath::Max(0.0, Params.ForwardDragPlowing);

    // Surge: thrust minus forward drag. Sway: only drag (water grips sideways).
    const double SurgeAccel = Thr * Params.EngineAccel - SurgeV * FwdDrag;
    const double SwayAccel = -SwayV * FMath::Max(0.0, Params.LateralDrag);

    Vel += Forward * SurgeAccel * Step + Right * SwayAccel * Step;

    // The rudder needs flow over it: authority scales with speed.
    const double Flow = FMath::Clamp(FMath::Abs(SurgeV) / FMath::Max(KINDA_SMALL_NUMBER, Params.RudderFlowSpeed), 0.0, 1.0);
    HeadingRad += Str * Params.RudderAuthority * Flow * Step;
}
