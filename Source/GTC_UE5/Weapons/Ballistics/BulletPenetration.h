// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Bullet penetration ("wall-banging") — a high-powered round punching through a plywood door,
 * a car panel, or the body it just hit and still wounding what's behind, while a pistol round
 * stops dead in concrete. FWeaponBallistics resolves the shot's damage/falloff at a hit; this
 * decides whether the round CARRIES ON through the surface, with how much power left for the
 * next thing in line, and how much of its damage survives the trip.
 *
 * A surface costs `thickness x density` of penetration power (a 5 cm plank of density 0.5
 * costs 2.5; 5 cm of concrete at density 3 costs 15). A round penetrates while it has the
 * power to pay that cost; each surface bleeds power, so a round can chain through several thin
 * things or be stopped by one thick one.
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling, no RNG. The
 * weapon trace adapter, on a hit, asks CanPenetrate; if so it continues the trace past the
 * surface with RemainingPower and applies ExitDamageFactor to the next victim's damage. Unit-
 * tested headless (Weapons/Ballistics/Tests/BulletPenetrationTest.cpp, prefix
 * GTC.Weapons.Penetration).
 */
struct GTC_UE5_API FBulletPenetration
{
    /** A round that only just makes it through still does at least this fraction of damage. */
    static constexpr double MinExitFactor = 0.1;

    /** Penetration power a surface consumes: thickness (cm) x density (~0.5 wood .. 3 concrete). */
    static double SurfaceCost(double ThicknessCm, double Density);

    /** Does a round of PenetrationPower make it through this surface? */
    static bool CanPenetrate(double PenetrationPower, double ThicknessCm, double Density);

    /** Penetration power left after passing through (0 if it can't, or if power runs out). */
    static double RemainingPower(double PenetrationPower, double ThicknessCm, double Density);

    /**
     * Fraction of damage that survives passing through the surface: 0 if the round can't
     * penetrate, otherwise RemainingPower/PenetrationPower clamped to [MinExitFactor, 1] — a
     * round that barely punches through hits the far side weakly, one with power to spare hits
     * nearly full.
     */
    static double ExitDamageFactor(double PenetrationPower, double ThicknessCm, double Density);
};
