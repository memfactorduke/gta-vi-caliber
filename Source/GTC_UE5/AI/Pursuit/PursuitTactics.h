// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure police-pursuit tactics: turns a cop car's situation versus a fleeing
 * target into a driving aim point and a discrete maneuver. Cops lead their
 * target, box it in with roadblocks, ram and PIT when the heat is high enough,
 * and back off when the wanted level drops or the target slips away.
 *
 * Direct port of the the reference `PursuitTactics` (RefCounted) at
 * `game/scripts/ai/pursuit_tactics.gd`. All static, FVector-in,
 * scalar/enum/FVector-out, no UObject — unit-tested headless via the parity
 * oracle (Tests/PursuitTacticsTest.cpp, prefix GTC.AI.Pursuit.PursuitTactics).
 *
 * Double precision throughout, to match the the reference implementation float math. Work happens in
 * the XZ plane (the reference Y-up); `Ground(V)` flattens to `FVector(V.X, 0, V.Z)`. No
 * Godot->UE Z-up axis remap is baked in — the model stays in the the reference XZ frame
 * so tests match the oracle bit-for-bit; the axis convention is a DEFERRED
 * Wave-3 concern. Defensive against zero-length inputs (no NaN, no div-by-zero).
 *
 * PURE-CORE boundary: this is the pure decision math only — it takes
 * caller-supplied positions/velocities/state as plain args. EQS + AI Perception
 * + Behavior Tree wiring is the deferred Wave-3 adapter, NOT implemented here
 * and NOT covered by the parity tests.
 */

/**
 * A discrete pursuit maneuver. Mirrors the the reference `Tactic` enum 1:1 (same order /
 * underlying int values).
 */
enum class EPursuitTactic : uint8
{
    Chase,   ///< drive to the lead-intercept point — the default pursuit
    Ram,     ///< close and aligned with authorisation → slam the target
    Block,   ///< set up ahead-and-to-the-side for a roadblock / boxing wall
    Pit,     ///< swing alongside to spin the target out
    BackOff, ///< disengage — wanted cleared or target gone
};

struct GTC_UE5_API FPursuitTactics
{
    /** Planar tolerance mirroring the Godot 0.0001 degenerate-input guard. */
    static constexpr double Eps = 0.0001;

    /** Stars at or above this authorise aggressive contact (ram / PIT). */
    static constexpr int32 AggressionStars = 3;

    /** Half-angle (radians) of the "lined up" cone for a ram. */
    static const double RamArcHalf;

    /** Distance past which a pursuer gives up regardless of stars. */
    static constexpr double GiveUpRange = 120.0;

    /** Drop the vertical component — cars steer on the ground plane. */
    static FVector Ground(const FVector& V);

    /** Planar unit direction from `A` to `B`, or ZERO if effectively coincident. */
    static FVector PlanarDir(const FVector& A, const FVector& B);

    /**
     * Lead-pursuit aim point: where to drive to cut the target off. Falls back to
     * the target's position when there is no useful solution (target stationary,
     * pursuer can't move, or the quadratic has no positive root).
     */
    static FVector InterceptPoint(
        const FVector& TargetPos, const FVector& TargetVel, const FVector& PursuerPos,
        double PursuerSpeed);

    /**
     * True only when a ram is justified: target within `RamRange`, roughly dead
     * ahead of the pursuer's heading, and `Stars >= AggressionStars`.
     */
    static bool ShouldRam(
        const FVector& PursuerPos, const FVector& PursuerHeading, const FVector& TargetPos,
        double RamRange, int32 Stars);

    /**
     * A point ahead of the target and offset to one `Side` (-1 left, +1 right of
     * the target's travel direction), at `Distance` out.
     */
    static FVector BlockOffset(
        const FVector& TargetPos, const FVector& TargetVel, double Side, double Distance);

    /** Which side (-1 / +1) the pursuer should swing to for a PIT maneuver. */
    static double PitSide(
        const FVector& PursuerPos, const FVector& TargetPos, const FVector& TargetVel);

    /** Target driving speed for closing a gap, ramping base..max across a band. */
    static double DesiredSpeed(double DistanceToTarget, double BaseSpeed, double MaxSpeed);

    /** Whether to disengage: 0 stars or target beyond give-up range. */
    static bool ShouldBackOff(int32 Stars, double Distance);

    /** Pick one tactic for this tick from the full picture (priority ladder). */
    static EPursuitTactic ChooseTactic(
        const FVector& PursuerPos, const FVector& PursuerHeading, const FVector& TargetPos,
        const FVector& TargetVel, int32 Stars, double RamRange);

private:
    /** Smallest strictly-positive of two roots, or -1 if neither qualifies. */
    static double SmallestPositive(double T1, double T2);

    /** True when the pursuer sits off to the target's side (good PIT geometry). */
    static bool IsAlongside(
        const FVector& PursuerPos, const FVector& TargetPos, const FVector& TargetVel);
};
