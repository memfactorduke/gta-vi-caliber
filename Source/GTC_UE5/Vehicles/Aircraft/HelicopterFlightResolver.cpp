// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelicopterFlightResolver.h"

#include "Math/UnrealMathUtility.h"

FHelicopterFlightResolver::FOutput FHelicopterFlightResolver::Integrate(const FInput& In)
{
    FOutput Out;

    // Advance the position by the flight velocity this frame.
    FVector NewPos = In.Position + In.Velocity * FMath::Max(0.0, In.Dt);

    // Ground floor: the chopper can't sink through the ground directly under it.
    if (NewPos.Z <= In.GroundZ)
    {
        NewPos.Z = In.GroundZ;
        Out.bGrounded = true;
    }
    Out.Position = NewPos;

    Out.AltitudeAgl = FMath::Max(0.0, NewPos.Z - In.GroundZ);
    // While grounded, report no descent (the position is pinned even though the flight model's
    // velocity may still point down until the collective is raised past hover).
    Out.ClimbRateCmS = Out.bGrounded ? FMath::Max(0.0, In.Velocity.Z) : In.Velocity.Z;
    Out.GroundSpeedCmS = FVector(In.Velocity.X, In.Velocity.Y, 0.0).Size();

    return Out;
}
