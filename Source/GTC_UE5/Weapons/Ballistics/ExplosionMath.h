// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure radial blast-damage falloff. Direct port of the the reference `ExplosionMath`
 * (RefCounted) at `game/scripts/weapons/explosion_math.gd`.
 *
 * Full damage within InnerRadius, linear falloff to zero at OuterRadius, nothing
 * beyond. Scene-free so Grenade (and future rockets/car bombs) share one tested
 * curve. Unit-tested headless via the reference behavior (Tests/ExplosionMathTest.cpp,
 * prefix GTC.Weapons.ExplosionMath).
 *
 * Double precision throughout to match the the reference implementation float math. Tolerance
 * mirrors is_equal_approx (Eps = 1e-4).
 *
 * PURE-CORE boundary: this is ONLY the falloff curve. The world sphere overlap
 * that finds targets to feed it is a DEFERRED Wave-3 adapter — NOT here, NOT
 * tested. The function takes a precomputed distance as input.
 */
struct GTC_UE5_API FExplosionMath
{
    /** Tolerance mirroring the the reference is_equal_approx. */
    static constexpr double Eps = 1e-4;

    /**
     * Damage at Distance: MaxDamage within InnerRadius, linear to 0 at OuterRadius,
     * 0 beyond. A degenerate/inverted band (Outer <= Inner) describes no real blast
     * volume → no damage anywhere (checked first, wins over the full-damage case).
     */
    static double RadialDamage(double Distance, double InnerRadius, double OuterRadius, double MaxDamage);
};
