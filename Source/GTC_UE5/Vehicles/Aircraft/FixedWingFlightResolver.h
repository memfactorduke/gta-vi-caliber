// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FFixedWingFlightResolver — the pure "integrate the flight model into a world position" adapter
 * half for the plane. FFixedWingFlight is a velocity/heading integrator and defers "integrating
 * Velocity into a world position, gear/terrain collision" to its adapter. This advances the
 * position by the flight velocity with a ground floor (a plane sits on the runway until it has the
 * airspeed to climb), and derives the telemetry a cockpit HUD wants — including the STALL, which is
 * the plane's whole skill: a stall margin (how much airspeed cushion is left) and a stall flag.
 *
 * Ground is a single Z the shell feeds (a downward trace, or a flat runway). Pure, deterministic,
 * no UObject. Units: positions/velocities cm and cm/s (Z up), airspeeds cm/s, Dt seconds.
 */
struct GTC_UE5_API FFixedWingFlightResolver
{
    struct FInput
    {
        FVector Position = FVector::ZeroVector; // current world position
        FVector Velocity = FVector::ZeroVector; // world velocity from FFixedWingFlight (cm/s, Z up)
        double GroundZ = 0.0;                   // ground height directly under the plane (world Z)
        double Dt = 0.0;
        double Airspeed = 0.0;                  // forward airspeed (cm/s) from the flight model
        double StallSpeed = 0.0;                // the airframe's stall airspeed (cm/s)
    };

    struct FOutput
    {
        FVector Position = FVector::ZeroVector; // integrated position, clamped to sit on the ground
        bool bGrounded = false;                 // resting on / at the ground this frame
        double AltitudeAgl = 0.0;               // height above the ground (cm, >= 0)
        double ClimbRateCmS = 0.0;              // vertical speed (+up); 0 while grounded
        double GroundSpeedCmS = 0.0;            // horizontal speed magnitude
        bool bStalled = false;                  // airspeed below stall — wings not flying
        double StallMargin = 0.0;               // airspeed cushion over stall, 0..1 (0 = at/below stall)
    };

    /** Advance one frame of position from the flight velocity + derive stall/altitude telemetry. Pure. */
    static FOutput Resolve(const FInput& In);
};
