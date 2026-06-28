// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Arcade fixed-wing flight — the plane to the helicopter's chopper. FHelicopterFlight
 * can hover; a plane is its inverse and the difference IS the gameplay: a plane's lift
 * comes from AIRSPEED, so the defining mechanic is the STALL. Fly too slow and the
 * wings stop flying — the nose drops and you sink no matter what the stick says, so
 * "keep your speed up" is the whole skill. FFixedWingFlight turns throttle + elevator
 * + aileron into the velocity and heading the actor adapter integrates.
 *
 * Two couplings make it feel like flight rather than a hovercraft:
 *   - CLIMBING COSTS AIRSPEED. Pulling the nose up trades speed for altitude; pull too
 *     hard and you bleed into a stall and drop. Diving trades altitude back for speed.
 *   - TURN RATE SCALES WITH AIRSPEED. You bank to turn, but a near-stalled plane barely
 *     comes around — you need flow over the surfaces.
 * Throttle sets airspeed against drag (a terminal cruise), and below StallSpeed the
 * plane sinks at StallSinkRate with the controls washed out until it picks speed back
 * up in the dive.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject — a
 * velocity/heading integrator; the adapter owns position and ground/terrain.
 * Unit-tested headless (Tests/FixedWingFlightTest.cpp, prefix GTC.Vehicles.FixedWing).
 *
 * PURE-CORE boundary: reading the sticks, integrating Velocity into a world position,
 * gear/terrain collision, and the engine/wind audio is the DEFERRED adapter and is NOT
 * covered by the tests. Units: accelerations cm/s^2, speeds cm/s, Heading radians, Dt
 * seconds. Throttle 0..1, Elevator/Aileron -1..1.
 */
struct FFixedWingFlight
{
    /** Per-airframe tuning. Defaults give a nimble light aircraft. */
    struct FParams
    {
        /** Forward acceleration at full throttle (cm/s^2). */
        double MaxThrustAccel = 500.0;
        /** Airspeed drag (per second) — sets the terminal cruise (thrust/drag). */
        double Drag = 0.2;
        /** Airspeed (cm/s) below which the wings stall (no lift). */
        double StallSpeed = 800.0;
        /** Vertical sink rate (cm/s) while stalled, controls washed out. */
        double StallSinkRate = 600.0;
        /** Reference airspeed at which control authority is full. */
        double CruiseSpeed = 1500.0;
        /** Climb rate (cm/s) at full up-elevator and full authority. */
        double ClimbAuthority = 700.0;
        /** Airspeed bled per second at full up-elevator (climbing costs speed; diving adds it). */
        double ClimbAirspeedCost = 300.0;
        /** Yaw/turn rate (rad/s) at full aileron and full authority. */
        double TurnAuthority = 0.8;
    };

    /** Install the tuning and reset to rest (stalled, at heading 0). */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick. `Throttle` (clamped 0..1) drives airspeed against drag;
     * `Elevator` (clamped -1..1; positive = nose up = climb, costing airspeed) sets
     * climb; `Aileron` (clamped -1..1) banks to turn, with authority scaling with
     * airspeed. Below StallSpeed the plane sinks and the controls wash out. `Dt`
     * clamped >= 0; a non-positive Dt is a no-op.
     */
    void Update(double Throttle, double Elevator, double Aileron, double Dt);

    /** World velocity (cm/s): horizontal along heading at airspeed, Z = climb rate. */
    FVector Velocity() const;

    /** Forward airspeed (cm/s). */
    double Airspeed() const { return AirspeedValue; }

    /** Vertical climb rate this tick (cm/s); negative is descending. */
    double ClimbRate() const { return ClimbRateValue; }

    /** Heading (radians); 0 faces +X. */
    double Heading() const { return HeadingRad; }

    /** True when airspeed is below StallSpeed (wings not flying). */
    bool IsStalled() const { return AirspeedValue < Params.StallSpeed; }

private:
    FParams Params;
    double AirspeedValue = 0.0;
    double ClimbRateValue = 0.0;
    double HeadingRad = 0.0;
};
