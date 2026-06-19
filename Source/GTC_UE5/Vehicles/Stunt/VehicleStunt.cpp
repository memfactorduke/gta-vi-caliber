// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleStunt.h"

#include "Math/UnrealMathUtility.h"

FStuntDetection FVehicleStuntDetector::Update(double Dt, double PitchRad, bool bAirborne, double HeightAboveGround)
{
    const double Step = FMath::Max(0.0, Dt);

    // Classify what the vehicle is doing this frame (airborne wins over pitch).
    EVehicleStunt Now;
    if (bAirborne)
    {
        Now = EVehicleStunt::Jump;
    }
    else if (PitchRad >= WheelieMinPitchRad)
    {
        Now = EVehicleStunt::Wheelie;
    }
    else if (PitchRad <= -StoppieMinPitchRad)
    {
        Now = EVehicleStunt::Stoppie;
    }
    else
    {
        Now = EVehicleStunt::None;
    }

    FStuntDetection Result;

    if (Now == Current)
    {
        // Same trick still going: extend it.
        Duration += Step;
        if (Current == EVehicleStunt::Jump)
        {
            PeakHeight = FMath::Max(PeakHeight, HeightAboveGround);
        }
        return Result;
    }

    // The trick changed — the previous one just ended. Bank it if it was real: a wheelie/
    // stoppie has to be HELD long enough; a jump has to get enough AIR (height, not airtime).
    if (Current != EVehicleStunt::None)
    {
        const bool bReal = (Current == EVehicleStunt::Jump)
            ? (PeakHeight >= MinJumpHeight)
            : (Duration >= MinTrickSeconds);
        if (bReal)
        {
            Result.bCompleted = true;
            Result.Kind = Current;
            Result.Magnitude = (Current == EVehicleStunt::Jump) ? PeakHeight : Duration;
        }
    }

    // Begin the new trick.
    Current = Now;
    Duration = Step;
    PeakHeight = (Now == EVehicleStunt::Jump) ? FMath::Max(0.0, HeightAboveGround) : 0.0;
    return Result;
}
