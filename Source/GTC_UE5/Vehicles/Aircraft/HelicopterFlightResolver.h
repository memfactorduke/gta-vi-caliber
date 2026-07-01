// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FHelicopterFlightResolver — the pure "integrate the flight model into a world position" adapter
 * half. FHelicopterFlight is a velocity/heading integrator and explicitly defers "integrating
 * Velocity into a world position, ground/obstacle collision" to its adapter. This is that step:
 * advance the position by the flight velocity, keep the chopper from sinking through the ground
 * under it, and derive the telemetry a HUD wants (altitude above ground, climb rate, ground speed).
 *
 * Ground is a single Z the shell feeds (a downward trace result, or a flat world floor); the shell
 * then moves the actor to the returned position (a sweep can add fine obstacle collision on top).
 * Pure, deterministic, no UObject. Units: positions/velocities in cm and cm/s (Z up), Dt in seconds.
 */
struct GTC_UE5_API FHelicopterFlightResolver
{
    struct FInput
    {
        FVector Position = FVector::ZeroVector; // current world position
        FVector Velocity = FVector::ZeroVector; // world velocity from FHelicopterFlight (cm/s, Z up)
        double GroundZ = 0.0;                   // ground height directly under the chopper (world Z)
        double Dt = 0.0;
    };

    struct FOutput
    {
        FVector Position = FVector::ZeroVector; // integrated position, clamped to sit on the ground
        bool bGrounded = false;                 // resting on / at the ground this frame
        double AltitudeAgl = 0.0;               // height above the ground (cm, >= 0)
        double ClimbRateCmS = 0.0;              // vertical speed (+up); 0 while grounded
        double GroundSpeedCmS = 0.0;            // horizontal speed magnitude
    };

    /** Advance one frame of position from the flight velocity, with a ground floor. Pure. */
    static FOutput Integrate(const FInput& In);
};
