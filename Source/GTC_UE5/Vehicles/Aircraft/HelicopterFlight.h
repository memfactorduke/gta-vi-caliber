// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Arcade rotary-wing flight — the missing half of the police chopper (and every
 * flyable helicopter). The ground vehicle stack is Chaos-based and the police
 * response can already call in a helicopter at 3 stars (World/Police), but nothing
 * actually FLIES one — there's no rotor model. FHelicopterFlight is that model: it
 * turns the four helicopter controls into a velocity and heading the actor adapter
 * integrates into a position.
 *
 * The feel of a helicopter is that "up" isn't free. COLLECTIVE makes lift that has
 * to fight gravity; CYCLIC tilts that thrust vector so some of the lift becomes
 * horizontal motion (stick forward drops the nose and you accelerate forward, stick
 * over banks and you slide sideways); the PEDAL spins the heading. So the per-tick
 * acceleration is just tilted-thrust minus gravity minus drag, resolved in a
 * yaw-rotated body frame, and the consequences fall out: hover is the exact
 * collective where lift cancels gravity (HoverCollective), holding full collective
 * climbs toward a drag-limited terminal speed, and easing off lets you sink.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * a velocity/heading integrator, so the adapter owns position and ground contact.
 * Unit-tested headless (Tests/HelicopterFlightTest.cpp, prefix GTC.Vehicles.Helicopter).
 *
 * PURE-CORE boundary: reading the sticks, integrating Velocity into a world position,
 * ground/obstacle collision, and the rotor wash/audio is the DEFERRED adapter and is
 * NOT covered by the tests. Units: accelerations are cm/s^2, Velocity cm/s, Heading
 * radians, Dt seconds. Controls: Collective 0..1, CyclicPitch/CyclicRoll/Pedal -1..1.
 */
struct FHelicopterFlight
{
    /** Per-airframe tuning. Defaults give a stable, responsive arcade chopper. */
    struct FParams
    {
        /** Downward gravity (cm/s^2). */
        double Gravity = 980.0;
        /** Upward acceleration at full collective (cm/s^2) — must exceed Gravity to climb. */
        double MaxLiftAccel = 2000.0;
        /** Horizontal acceleration at full cyclic deflection (cm/s^2). */
        double TiltAccel = 1200.0;
        /** Yaw rate at full pedal (rad/s). */
        double YawRatePerSec = 1.5;
        /** Linear drag coefficient (per second) applied to velocity — sets terminal speeds. */
        double LinearDrag = 0.5;
    };

    /** Install the tuning and reset to rest at heading 0. */
    void Configure(const FParams& InParams);

    /**
     * Advance one tick. `Collective` (clamped 0..1) lifts against gravity; `CyclicPitch`
     * (clamped -1..1; negative = nose down = accelerate forward) and `CyclicRoll`
     * (clamped -1..1; positive = bank right) tilt the thrust into horizontal motion;
     * `Pedal` (clamped -1..1) turns the heading. `Dt` clamped >= 0; a non-positive Dt is
     * a no-op.
     */
    void Update(double Collective, double CyclicPitch, double CyclicRoll, double Pedal, double Dt);

    /** World-space velocity (cm/s). Z is vertical. */
    FVector Velocity() const { return Vel; }

    /** Heading (radians); 0 faces +X. */
    double Heading() const { return HeadingRad; }

    /** The collective that exactly cancels gravity — the hands-off hover setting, 0..1. */
    double HoverCollective() const;

    /** Kill any descent (called by the adapter on ground contact) so a chopper sitting on the deck
     *  doesn't wind up downward velocity against the ground clamp and then lift off sluggishly. */
    void HaltDescent()
    {
        if (Vel.Z < 0.0)
        {
            Vel.Z = 0.0;
        }
    }

private:
    FParams Params;
    FVector Vel = FVector::ZeroVector;
    double HeadingRad = 0.0;
};
