// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure parachute / skydive model — the freefall -> canopy -> landing arc a
 * player rides after leaping from a helicopter or rooftop.
 *
 * Direct port of the Godot `Parachute` (RefCounted) stateful instance at
 * `game/scripts/player/parachute.gd`. No nodes: it owns the jump's State and the
 * descent physics. The arc has three beats — FREEFALL plummets toward
 * TerminalVelocity with barely any steering; Deploy() pops the canopy (once, only
 * from freefall) to bleed speed toward CanopyDescentRate and steer-glide across
 * XZ; Land() ends the jump.
 *
 * Double precision throughout, to match the GDScript float math. Work happens in
 * the Godot Y-up frame: fall speed is a positive downward magnitude, horizontal
 * drift is on the XZ plane. NO Godot->UE Z-up axis remap is baked in — wiring
 * this onto a UE CharacterMovement rig (gravity/collision stay the engine's job)
 * is a DEFERRED Wave-3 adapter and is NOT parity-covered.
 *
 * NOTE: this system is marked UNUSED in the migration inventory, but it has a
 * live Godot test oracle (game/tests/unit/test_parachute.gd), so it is ported
 * with full parity coverage per the Wave-1 "port any that have an oracle" rule.
 *
 * `TimeToGround` returns true +infinity (std::numeric_limits<double>::infinity)
 * for a hovering descent rate, matching the Godot INF.
 *
 * Parity coverage: Tests/ParachuteTest.cpp, prefix GTC.Player.Motion.
 */
struct GTC_UE5_API FParachute
{
    enum class EState : uint8
    {
        Freefall,
        Deployed,
        Landed,
    };

    /** Gravity pulling a freefalling skydiver down (m/s^2). Positive = downward. */
    static constexpr double Gravity = 9.8;

    /** How fast the canopy bleeds excess speed once deployed (m/s^2 of deceleration). */
    static constexpr double CanopyDrag = 12.0;

    /**
     * TerminalVelocity caps the freefall plummet; CanopyDescentRate is the gentle
     * speed the open chute settles toward. Both are floored at 0 so the physics
     * never inverts. Defaults mirror the Godot _init (55.0, 6.0).
     */
    explicit FParachute(double TerminalVelocity = 55.0, double CanopyDescentRate = 6.0);

    /**
     * Pop the canopy. Only valid from FREEFALL — a second deploy (or deploying
     * after landing) is a no-op. Returns true iff the chute actually opened.
     */
    bool Deploy();

    /** True once the canopy is open (and not yet landed). */
    bool IsDeployed() const;

    /** The current jump State (Freefall / Deployed / Landed). */
    EState State() const;

    /**
     * Advance the vertical fall speed (positive m/s, downward) one frame. In
     * FREEFALL accelerate under gravity toward TerminalVelocity; under canopy
     * decelerate toward the slow CanopyDescentRate. Result is clamped so freefall
     * never exceeds terminal and the canopy never speeds back up past terminal.
     */
    double UpdateFallSpeed(double CurrentSpeed, double Delta) const;

    /**
     * Horizontal velocity (XZ) from a steer input. Under canopy you carve at
     * GlideSpeed; in freefall the same input yields a fraction (0.12) of the
     * drift. Vertical input is dropped and the result is clamped to GlideSpeed.
     */
    FVector HorizontalDrift(const FVector& SteerInput, bool bDeployed, double GlideSpeed) const;

    /**
     * Landing damage factor in [0, 1]: 0 for any touchdown at or below SafeSpeed,
     * ramping to a full 1 splat as impact climbs to twice SafeSpeed and beyond.
     * Guarded so a zero/negative SafeSpeed can't divide by zero.
     */
    double LandingImpact(double VerticalSpeed, double SafeSpeed) const;

    /** Whether a touchdown at VerticalSpeed is walked-off cleanly (no damage). */
    bool IsSafeLanding(double VerticalSpeed, double SafeSpeed) const;

    /**
     * Seconds until the ground at the current DescentRate — info/HUD only.
     * Negative altitude reads as already-landed (0); a zero/negative descent rate
     * (hovering) returns +infinity rather than dividing by zero.
     */
    double TimeToGround(double Altitude, double DescentRate) const;

    /** End the jump — feet on the ground. */
    void Land();

    /** Back to a fresh pre-jump FREEFALL, ready to skydive again. */
    void Reset();

    EState StateValue = EState::Freefall;
    double TerminalVelocityValue = 55.0;
    double CanopyDescentRateValue = 6.0;
};
