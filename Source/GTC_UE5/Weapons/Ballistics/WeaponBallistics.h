// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure gunplay math a weapon controller applies to each shot: distance falloff,
 * hit-zone multipliers, cone spread, and a stateful recoil/bloom accumulator.
 * Direct port of the Godot `WeaponBallistics` (RefCounted) at
 * `game/scripts/weapons/weapon_ballistics.gd`.
 *
 * The static helpers are scene-free (or take an explicit RNG), so they unit-test
 * deterministically via the parity oracle (Tests/WeaponBallisticsTest.cpp,
 * prefix GTC.Weapons.WeaponBallistics). This complements the lower-level
 * FBallistics (which takes a pre-drawn disk sample); here SpreadDirection draws
 * its own cone offset from a caller-supplied RNG, and the nested Bloom carries
 * per-shot spread growth/recovery across frames.
 *
 * Double precision throughout to match the GDScript float math. Distances are
 * metres, angles radians. Tolerance mirrors is_equal_approx (Eps = 1e-4).
 *
 * NOTE: no Godot->UE Z-up axis remap — the model stays in the Godot frame (the
 * Godot `Vector3.UP`/`Vector3.RIGHT` basis helpers are preserved verbatim so the
 * cone-containment tests match). Axis remap is a DEFERRED Wave-3 concern.
 *
 * RNG parity note: SpreadDirection uses FRandomStream, which is deterministic
 * and seed-reproducible WITHIN UE5 — it is NOT a Godot PCG32 reimplementation,
 * so the exact perturbed vectors are NOT byte-identical to Godot. This is safe:
 * the oracle never pins a seed to an exact spread vector; it only asserts the
 * statistical/range properties (result normalized, within the cone half-angle,
 * actually perturbed). Mapping: randf -> GetFraction(). (LootTable pattern.)
 *
 * PURE-CORE boundary: this is ONLY the spread/falloff/TTK/bloom math. The world
 * line traces that fire a perturbed direction and apply effective damage at a
 * hit are a DEFERRED Wave-3 adapter — NOT implemented here, NOT tested.
 */
struct GTC_UE5_API FWeaponBallistics
{
    /** Tolerance mirroring the Godot is_equal_approx / length guard. */
    static constexpr double Eps = 1e-4;

    /** Default hit-zone multipliers (head rewards precision, limbs punish sloppy aim). */
    static constexpr double HeadMultiplier = 2.0;
    static constexpr double TorsoMultiplier = 1.0;
    static constexpr double LimbMultiplier = 0.7;

    /**
     * Damage after distance falloff: full inside FalloffStart, lerps down to
     * BaseDamage * MinFactor by FalloffEnd, never below that floor. MinFactor is
     * clamped to [0, 1]; a degenerate band (End <= Start) snaps to the floor past
     * Start. Negative base/distance are guarded so damage can't go below zero.
     */
    static double DamageAtRange(
        double BaseDamage, double Distance, double FalloffStart, double FalloffEnd, double MinFactor);

    /**
     * Damage multiplier for the body part a shot landed on. Case-insensitive;
     * anything unrecognised (including "") falls back to 1.0.
     */
    static double HitMultiplier(const FString& BodyPart);

    /**
     * Perturb a normalised aim direction by up to SpreadRadians within a cone and
     * return a unit vector. Deterministic given Rng. Spread <= 0 (or a zero/
     * degenerate aim) returns the aim unchanged. The offset is drawn uniformly over
     * the cone's base disk via tan(spread), so dot(result, aim) >= cos(spread).
     */
    static FVector SpreadDirection(const FVector& AimDir, double SpreadRadians, FRandomStream& Rng);

    /**
     * Full per-shot damage: range falloff times the hit-zone multiplier. The one
     * call a weapon controller makes once it knows distance and where the ray landed.
     */
    static double EffectiveDamage(
        double BaseDamage,
        double Distance,
        const FString& BodyPart,
        double FalloffStart,
        double FalloffEnd,
        double MinFactor);

    /**
     * Seconds to down a target: shots needed (ceil) spread across the fire rate. The
     * first shot lands at t=0, so n shots take (n - 1) / fire_rate seconds. Returns
     * true +infinity when a shot can't hurt the target (no damage, or non-positive
     * fire rate). Tests assert the guard via !FMath::IsFinite.
     */
    static double TimeToKill(double EffectiveDamagePerShot, double FireRate, double TargetHealth);

    /**
     * Stateful recoil/bloom accumulator: the cone widens as fire is sustained and
     * tightens back toward its base while the trigger rests, so tapping stays
     * accurate and spraying walks off. An instance per live weapon. Nested type
     * mirroring the Godot inner class `WeaponBallistics.Bloom`.
     */
    struct GTC_UE5_API FBloom
    {
        double Spread = 0.0;
        double MinSpread = 0.0;
        double MaxSpread = 0.0;
        double PerShot = 0.0;
        double Recovery = 0.0;

        /**
         * Min/Max are the calm and fully-bloomed cone half-angles (rad); PerShot is
         * the bloom added each shot; Recovery is how fast the cone shrinks (rad/s).
         * All are clamped non-negative and ordered so Min <= Max.
         */
        explicit FBloom(
            double InMin = 0.01, double InMax = 0.16, double InPerShot = 0.02, double InRecovery = 0.22);

        /** The cone half-angle to fire the next shot with. */
        double CurrentSpread() const;

        /** Register a shot: the cone blooms by PerShot, capped at MaxSpread. */
        void AddShot();

        /**
         * Advance Delta seconds of trigger rest: the cone recovers toward MinSpread,
         * never dipping below it.
         */
        void Recover(double Delta);

        /** Snap back to the calm cone (e.g. on reload or holster). */
        void Reset();
    };
};
