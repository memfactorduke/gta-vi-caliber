// Copyright Epic Games, Inc. All Rights Reserved.

#include "NavalPursuit.h"

#include "PursuitTactics.h"
#include "Math/UnrealMathUtility.h"

namespace GTCNaval
{
    // Swap Y<->Z to move between UE Z-up (water plane XY) and the legacy Godot XZ frame
    // FPursuitTactics works in (it flattens Y). The swap is its own inverse.
    static FVector SwapYZ(const FVector& V)
    {
        return FVector(V.X, V.Z, V.Y);
    }

    static constexpr double Eps = 1e-6;
}

FVector FNavalPursuit::InterceptPoint(const FVector& TargetPos, const FVector& TargetVel,
    const FVector& PursuerPos, double PursuerSpeed)
{
    const FVector R = GTCNaval::SwapYZ(FPursuitTactics::InterceptPoint(
        GTCNaval::SwapYZ(TargetPos), GTCNaval::SwapYZ(TargetVel), GTCNaval::SwapYZ(PursuerPos), PursuerSpeed));
    // FPursuitTactics flattens its vertical; the intercept is a water-plane maneuver, so
    // put it back on the target's water level.
    return FVector(R.X, R.Y, TargetPos.Z);
}

FVector FNavalPursuit::BroadsideOffset(const FVector& TargetPos, const FVector& TargetVel,
    double Side, double StandoffCm)
{
    const double S = FMath::Abs(Side) > GTCNaval::Eps ? FMath::Sign(Side) : 1.0;
    const FVector Flat(TargetVel.X, TargetVel.Y, 0.0); // water-plane travel
    if (Flat.Size() < GTCNaval::Eps)
    {
        // No heading: step out along world Y to the requested side, on the water level.
        return FVector(TargetPos.X, TargetPos.Y + S * StandoffCm, TargetPos.Z);
    }
    const FVector Dir = Flat.GetSafeNormal();
    // Right-hand perpendicular on the XY plane (Z up): forward x up = (dir.Y, -dir.X, 0).
    const FVector Right(Dir.Y, -Dir.X, 0.0);
    const FVector P = TargetPos + Right * (S * StandoffCm);
    return FVector(P.X, P.Y, TargetPos.Z);
}

FVector FNavalPursuit::ClampToWater(const FVector& Desired, double ShoreHeightAtDesiredCm, double SeaLevelZCm)
{
    return FVector(Desired.X, Desired.Y, FMath::Max(SeaLevelZCm, ShoreHeightAtDesiredCm));
}

bool FNavalPursuit::ShouldRamHull(const FVector& PursuerPos, const FVector& PursuerHeading,
    const FVector& TargetPos, double RamRangeCm, int32 Stars)
{
    return FPursuitTactics::ShouldRam(
        GTCNaval::SwapYZ(PursuerPos), GTCNaval::SwapYZ(PursuerHeading), GTCNaval::SwapYZ(TargetPos),
        RamRangeCm, Stars);
}
