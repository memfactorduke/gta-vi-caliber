// Copyright Epic Games, Inc. All Rights Reserved.

#include "FixedWingFlightResolver.h"

#include "Math/UnrealMathUtility.h"

FFixedWingFlightResolver::FOutput FFixedWingFlightResolver::Resolve(const FInput& In)
{
    FOutput Out;

    // Advance the position by the flight velocity this frame.
    FVector NewPos = In.Position + In.Velocity * FMath::Max(0.0, In.Dt);

    // Ground floor: the plane can't sink through the runway/terrain under it.
    if (NewPos.Z <= In.GroundZ)
    {
        NewPos.Z = In.GroundZ;
        Out.bGrounded = true;
    }
    Out.Position = NewPos;

    Out.AltitudeAgl = FMath::Max(0.0, NewPos.Z - In.GroundZ);
    Out.ClimbRateCmS = Out.bGrounded ? FMath::Max(0.0, In.Velocity.Z) : In.Velocity.Z;
    Out.GroundSpeedCmS = FVector(In.Velocity.X, In.Velocity.Y, 0.0).Size();

    // Stall telemetry: the plane's whole skill is keeping airspeed above stall.
    Out.bStalled = In.Airspeed < In.StallSpeed;
    const double Denom = FMath::Max(1.0, In.StallSpeed);
    Out.StallMargin = FMath::Clamp((In.Airspeed - In.StallSpeed) / Denom, 0.0, 1.0);

    return Out;
}
