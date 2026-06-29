// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Impact response for kinematic vehicles — the two numbers a swept move needs when the
 * pawn hits something: how much DAMAGE the collision deals, and how much of the
 * into-surface velocity to KEEP (so a glancing scrape slides along a wall while a head-on
 * slam dead-stops). Pairs with FVehicleGroundContact (which handles the floor) for the
 * walls/props/other-hulls an aircraft or boat can run into. The damage feeds
 * FVehicleHealth::ApplyDamage; the restitution shapes the velocity the pawn keeps.
 *
 * Damage is zero up to a safe speed, then grows quadratically — light taps are free,
 * but speed hurts fast, the way a real prang does.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * stateless static helpers. Units: speeds cm/s, damage in FVehicleHealth points.
 * Unit-tested headless (Vehicles/Contact/Tests/VehicleImpactTest.cpp, prefix
 * GTC.Vehicles.Impact).
 */
struct GTC_UE5_API FVehicleImpact
{
    /**
     * Damage from an impact at `ImpactSpeedCm`: 0 at/below `SafeSpeedCm`, then
     * `Scale * (speed - safe)^2` above it. `SafeSpeedCm`/`Scale` clamp to >= 0; a
     * non-positive scale yields no damage.
     */
    static double DamageForSpeed(double ImpactSpeedCm, double SafeSpeedCm, double Scale);

    /**
     * Fraction (0..1) of the into-surface velocity to keep after a contact. `GrazeDot`
     * is how head-on the hit is (0 = a perpendicular glance along the surface, 1 = a
     * dead-on hit into the surface normal): a glance keeps ~all of it (slide), a head-on
     * hit keeps ~none (stop). Returns 0 when `IntoSurfaceSpeedCm` is non-positive (the
     * body is already moving away — nothing to retain).
     */
    static double Restitution(double IntoSurfaceSpeedCm, double GrazeDot);
};
