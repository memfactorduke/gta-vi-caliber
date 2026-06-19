// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Throwables — grenades, molotovs, sticky bombs: the lob arc, the landing-point preview,
 * and the cook-the-fuse tension that makes them a skill rather than a button. FExplosion
 * model already resolves the BLAST (radial damage / knockback / chain) once something
 * goes off; this is the missing front half — WHERE a thrown explosive ends up and WHEN it
 * detonates — whose output (a detonation point) feeds straight into FExplosionModel.
 *
 * Three things it answers:
 *   - The ARC: ballistic projectile motion from a throw, so the adapter can draw the aim
 *     trajectory and predict the landing spot before the player commits.
 *   - The FUSE / COOK: a grenade's fuse can be started while still in hand ("cooking") to
 *     rob the target of reaction time — hold too long and it goes off in your palm.
 *   - The DETONATION: a fuse grenade bursts at its fuse time wherever it is (air-burst), or
 *     rests where it landed if it touched down first.
 *
 * PURE-CORE: scene-free, deterministic, double precision, FVector-in / FVector-out, no
 * engine coupling, no RNG. The throwable actor/adapter holds the live cook timer, spawns
 * the mesh, sweeps the real collision for an early bounce, and on detonation calls
 * FExplosionModel at DetonationPoint(...). Units are the adapter's own (UE cm / cm-per-s /
 * cm-per-s^2 by default); times are seconds; +Z is up. Unit-tested headless
 * (Weapons/Throwables/Tests/ThrowableTest.cpp, prefix GTC.Weapons.Throwable).
 */
struct GTC_UE5_API FThrowable
{
    /** Default grenade fuse (seconds from pin-pull to detonation). */
    static constexpr double DefaultFuseSeconds = 3.5;
    /** Default gravity magnitude (UE cm/s^2). Adapter may override per-throwable. */
    static constexpr double DefaultGravity = 980.0;

    // --- Fuse / cook ---

    /**
     * Fuse time left after cooking for `CookSeconds`. Clamped to >= 0. Reaching 0 means it
     * cooked off — see CookedOff. A negative FuseSeconds is treated as 0.
     */
    static double FuseAfterCook(double FuseSeconds, double CookSeconds);

    /** True if cooking this long detonates it before it can be thrown (in your hand). */
    static bool CookedOff(double FuseSeconds, double CookSeconds);

    // --- Trajectory ---

    /** Launch velocity from a (need-not-be-normalised) aim direction and a throw speed. */
    static FVector LaunchVelocity(const FVector& AimDir, double ThrowSpeed);

    /**
     * Ballistic position at time `T` >= 0 under downward gravity:
     * Origin + LaunchVel*T - 0.5*Gravity*T^2 on Z. T < 0 returns Origin.
     */
    static FVector PositionAtTime(const FVector& Origin, const FVector& LaunchVel, double Gravity, double T);

    /**
     * Seconds until the projectile falls to height `GroundZ` (the later, descending root of
     * the ballistic arc). Returns -1 if it never reaches that height (e.g. no gravity and
     * thrown level or upward, or ground above the apex). Requires Gravity > 0 for the arc.
     */
    static double TimeToGround(const FVector& Origin, const FVector& LaunchVel, double Gravity, double GroundZ);

    /** Predicted landing point on the plane Z = GroundZ. Falls back to Origin if it never
     *  descends to the ground. */
    static FVector PredictLanding(const FVector& Origin, const FVector& LaunchVel, double Gravity, double GroundZ);

    // --- Detonation ---

    /**
     * Where a fuse grenade detonates given `FuseRemaining` seconds left when it leaves the
     * hand: at its airborne position if the fuse expires before it lands, otherwise at the
     * point where it touched down (it rests there until the fuse runs out). The detonation
     * TIME is FuseRemaining; feed the returned point into FExplosionModel.
     */
    static FVector DetonationPoint(const FVector& Origin, const FVector& LaunchVel, double Gravity,
        double GroundZ, double FuseRemaining);
};
