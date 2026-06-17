// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure hitscan math shared by every weapon — the low-level twin of
 * FWeaponBallistics. Direct port of the the reference `Ballistics` (RefCounted) at
 * `game/scripts/weapons/ballistics.gd`.
 *
 * Static functions only — no scene access, no RNG. The caller passes a random
 * disk sample in (so results are deterministic and testable). All FVector-in /
 * FVector-out, scalar-in / scalar-out. Unit-tested headless via the parity
 * oracle (Tests/BallisticsTest.cpp, prefix GTC.Weapons.Ballistics).
 *
 * Double precision throughout, to match the the reference implementation float math. Tolerance
 * mirrors the oracle's is_equal_approx (Eps = 1e-4).
 *
 * NOTE: no Godot->UE Z-up axis remap is baked in here — the model stays in the
 * the reference frame (forward = -Z, up = +Y) so the ported unit tests match the oracle.
 * The axis convention port to UE's Z-up space is a DEFERRED Wave-3 concern.
 *
 * PURE-CORE boundary: this is ONLY the spread/falloff/sampling math. The actual
 * world line traces that shoot a perturbed direction and apply damage at a hit
 * are a DEFERRED Wave-3 adapter — NOT implemented here, NOT covered by the
 * parity tests. The model takes positions/directions/params as inputs.
 */
struct GTC_UE5_API FBallistics
{
    /** Planar tolerance mirroring the the reference is_zero_approx / is_equal_approx. */
    static constexpr double Eps = 1e-4;

    /**
     * Perturb a forward aim direction within a cone. Sample is a point in the unit
     * disk (each component in [-1, 1], length <= 1) supplied by the caller's RNG;
     * Right/Up are the camera basis. Spread is the cone half-angle in radians: 0
     * (or a zero sample) returns Forward unchanged. Result is unit length.
     */
    static FVector SpreadDirection(
        const FVector& Forward, const FVector& Right, const FVector& Up, const FVector2D& Sample, double Spread);

    /**
     * Damage after distance falloff: full inside FalloffStart, lerps down to
     * BaseDamage * MinFraction by FalloffEnd, flat beyond. MinFraction is clamped
     * to [0, 1]. A degenerate (End <= Start) band returns the near (floor) value.
     */
    static double DamageAtRange(
        double BaseDamage, double Distance, double FalloffStart, double FalloffEnd, double MinFraction);

    /**
     * Damage multiplier for where a shot landed on a target: a hit at or above
     * HeadHeight (measured from the target's origin) gets HeadMult, otherwise 1.
     */
    static double ZoneMultiplier(double LocalHeight, double HeadHeight, double HeadMult);

    /**
     * A point uniformly distributed in the unit disk from two independent [0, 1)
     * samples (rejection-free, area-correct). Feeds SpreadDirection without
     * clumping shots toward the centre. Caller supplies the randoms.
     */
    static FVector2D DiskSample(double URadius, double UAngle);
};
