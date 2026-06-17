// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure radial-blast model — damage, knockback, and chain reactions. Direct port
 * of the the reference `ExplosionModel` (RefCounted) at
 * `game/scripts/weapons/explosion_model.gd`.
 *
 * Full 3D (verticality matters: a grenade under a balcony still lifts bodies),
 * scene-free, all static, FVector-in / FVector-out. Grenades, car explosions,
 * and barrel chains feed it positions; it returns numbers a node applies.
 * Unit-tested headless via the reference behavior (Tests/ExplosionModelTest.cpp,
 * prefix GTC.Weapons.ExplosionModel).
 *
 * FExplosionMath already owns a simple inner/outer-radius damage curve for the
 * grenade; this is the broader model (knockback + chaining + batch queries).
 *
 * Falloff curve: LINEAR. value = 1 at the centre, ramps straight down to 0 at
 * Radius, 0 beyond.
 *
 * Double precision throughout to match the the reference implementation float math. Tolerance
 * mirrors is_equal_approx (Eps = 1e-4).
 *
 * NOTE: no Godot->UE Z-up axis remap — the model stays in the the reference frame, and
 * the upward knockback bias is along the reference +Y (FVector::UpVector here is the
 * literal the reference `Vector3.UP` = (0,1,0)). Axis remap is a DEFERRED Wave-3 concern.
 *
 * PURE-CORE boundary: this is ONLY the blast math. The world sphere overlaps
 * that gather targets/explosives and the physics that apply impulses are a
 * DEFERRED Wave-3 adapter — NOT implemented here, NOT tested. Functions take
 * caller-supplied positions; ApplyToMany takes a precomputed target array.
 */

/** One blast hit from ApplyToMany: the target's index and the damage it takes. */
struct GTC_UE5_API FExplosionHit
{
    int32 Index = 0;
    double Damage = 0.0;
};

struct GTC_UE5_API FExplosionModel
{
    /** Tolerance mirroring the the reference is_equal_approx. */
    static constexpr double Eps = 1e-4;

    /**
     * Fraction of the remaining (distance-scaled) impulse added straight up, so
     * bodies pop into the air instead of only sliding along the ground.
     */
    static constexpr double UpwardBias = 0.35;

    /**
     * The shared 0..1 falloff: 1 at the centre, linearly to 0 at Radius, clamped
     * both ends. A non-positive radius is a degenerate blast → always 0.
     */
    static double Falloff(double Distance, double Radius);

    /**
     * True while TargetPos sits strictly inside the blast sphere. The boundary
     * (distance == radius) is out — falloff is already 0 there.
     */
    static bool IsInBlast(const FVector& Center, const FVector& TargetPos, double Radius);

    /**
     * Damage dealt to a target: MaxDamage at the centre, linearly to 0 at Radius,
     * 0 beyond. Never negative (MaxDamage is floored at 0).
     */
    static double DamageAt(const FVector& Center, const FVector& TargetPos, double MaxDamage, double Radius);

    /**
     * Outward knockback impulse: away from Center, magnitude MaxImpulse at the
     * centre falling to 0 at Radius, plus an upward bias so bodies are lifted.
     * Returns ZeroVector when the target is exactly at the centre (no direction →
     * no NaN, but the vertical bias still throws the body up) or at/beyond Radius.
     */
    static FVector Knockback(const FVector& Center, const FVector& TargetPos, double MaxImpulse, double Radius);

    /**
     * Whether this blast detonates a nearby explosive (car, barrel) sitting at
     * OtherExplosivePos, driving chain reactions. True within TriggerRadius
     * (inclusive — a touching gas tank should go), false beyond.
     */
    static bool ShouldChain(const FVector& Center, const FVector& OtherExplosivePos, double TriggerRadius);

    /**
     * Batch damage query: every target inside the blast, as {Index, Damage}.
     * Targets at/beyond Radius are omitted (they take nothing), so callers iterate
     * only on real hits.
     *
     * (the reference defensive `not (entry is Vector3)` skip is unrepresentable with a
     * typed TArray<FVector> — every entry is already a valid FVector — so it has
     * no analogue here, mirroring the LootTable malformed-entry note.)
     */
    static TArray<FExplosionHit> ApplyToMany(
        const FVector& Center, const TArray<FVector>& Targets, double MaxDamage, double Radius);
};
