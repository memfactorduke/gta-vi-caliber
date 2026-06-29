// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Arcade watercraft handling — how a boat or jet ski actually drives, for a game on
 * the Miami coast. The Ocean stack floats things (OceanBuoyancy is vertical bob and
 * righting) but nothing turns throttle + steer into a boat's motion. FBoatHandling is
 * that: it produces the planing velocity and heading the actor adapter rides on top
 * of the buoyancy.
 *
 * A boat doesn't drive like a car, and two details carry the feel:
 *   - ANISOTROPIC DRAG. Water resists sideways motion far more than forward, so the
 *     hull slides in a hard turn and then bites — drift, then grip. Velocity is split
 *     into surge (forward) and sway (sideways); sway is bled off fast.
 *   - THE PLANING HUMP. Forward drag is high while plowing through the water, then
 *     drops once you pass PlaningSpeed and the hull climbs onto plane — so the boat
 *     lurches up to speed and its top end lives above that threshold.
 * And the rudder needs FLOW: steering authority scales with speed, so a dead-stop boat
 * won't turn until the throttle pushes water past the rudder.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject — a
 * velocity/heading integrator; the adapter owns position, buoyancy and wakes.
 * Unit-tested headless (Tests/BoatHandlingTest.cpp, prefix GTC.Vehicles.Watercraft).
 *
 * PURE-CORE boundary: reading the sticks, integrating Velocity into a world position,
 * layering on OceanBuoyancy, and the wake/spray VFX is the DEFERRED adapter and is NOT
 * covered by the tests. Units: accelerations cm/s^2, Velocity cm/s, Heading radians,
 * Dt seconds. Throttle -1..1 (reverse), Steer -1..1.
 */
struct FBoatHandling
{
    /** Per-hull tuning. Defaults give a responsive speedboat that planes. */
    struct FParams
    {
        /** Forward acceleration at full throttle (cm/s^2). */
        double EngineAccel = 800.0;
        /** Forward drag (per second) while plowing — below the planing speed. */
        double ForwardDragPlowing = 0.9;
        /** Forward drag (per second) while on plane — above the planing speed (lower => faster). */
        double ForwardDragPlaning = 0.3;
        /** Sideways drag (per second) — high, so sway bleeds off and the hull grips after a slide. */
        double LateralDrag = 3.0;
        /** Forward speed (cm/s) at which the hull climbs onto plane. */
        double PlaningSpeed = 600.0;
        /** Yaw rate at full steer and full rudder flow (rad/s). */
        double RudderAuthority = 1.2;
        /** Forward speed (cm/s) at which the rudder reaches full authority; below, it scales down. */
        double RudderFlowSpeed = 300.0;
    };

    /** Install the tuning and reset to rest at heading 0. */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick. `Throttle` (clamped -1..1; negative = reverse) drives surge;
     * `Steer` (clamped -1..1) yaws the heading with authority that grows with speed
     * (the rudder needs flow). Drag is anisotropic and the forward drag drops on plane.
     * `Dt` clamped >= 0; a non-positive Dt is a no-op.
     */
    void Update(double Throttle, double Steer, double Dt);

    /** World velocity (cm/s), Z is 0 (handling is planar; buoyancy owns vertical). */
    FVector Velocity() const { return Vel; }

    /** Heading (radians); 0 faces +X. */
    double Heading() const { return HeadingRad; }

    /** Speed along the heading (cm/s); negative when reversing. */
    double ForwardSpeed() const;

    /** Sideways slip speed (cm/s) — nonzero mid-drift, bled off by lateral drag. */
    double LateralSpeed() const;

    /** True when the hull is on plane (forward speed past PlaningSpeed). */
    bool IsPlaning() const;

private:
    FParams Params;
    FVector Vel = FVector::ZeroVector;
    double HeadingRad = 0.0;
};
