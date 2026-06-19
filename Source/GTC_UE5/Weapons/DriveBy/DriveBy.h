// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Drive-by shooting — leaning out of a moving car to spray the street. The constraints that
 * make it feel right and not like a turret: you can't fire forward THROUGH your own vehicle
 * (only out the sides and back), and your aim is shakier the faster you're going. This is
 * the thin rule layer between the player's aim and the existing FWeaponBallistics shot
 * resolution; it gates whether a shot is allowed and how much extra spread the speed adds.
 *
 * Composes with FAimAssist (soft-lock still helps pick the side target) and FWeaponBallistics
 * (feed EffectiveSpread into its cone). Used for the player and for drive-by AI pursuers.
 *
 * PURE-CORE: scene-free, deterministic, double precision, FVector-in, no engine coupling, no
 * RNG. The vehicle-combat adapter supplies the car's forward + right (ground-plane unit
 * vectors), the aim direction, and the current speed fraction (speed / top speed), then gates
 * the trigger on CanFire and widens the ballistics cone by EffectiveSpread. Angles are
 * radians. Unit-tested headless (Weapons/DriveBy/Tests/DriveByTest.cpp, prefix
 * GTC.Weapons.DriveBy).
 */
struct GTC_UE5_API FDriveBy
{
    /** Half-angle (radians) of the forward cone you CAN'T fire into — your own bonnet/windscreen (~45deg). */
    static constexpr double ForwardBlockHalfAngleRad = 0.7854;
    /** Extra spread (radians) added at full speed; lerps from 0 at a standstill (~7deg). */
    static constexpr double MaxSpeedSpreadRad = 0.12;

    /** Angle (radians, 0..pi) between the car's forward and the aim direction. 0 dead ahead,
     *  pi straight back. Returns 0 for a degenerate forward/aim (so it reads as "ahead" and
     *  CanFire blocks it). */
    static double AimAngleFromForward(const FVector& VehicleForward, const FVector& AimDir);

    /** Can the shot be taken without firing through your own car: the aim is outside the
     *  forward block cone (and the inputs are valid). */
    static bool CanFire(const FVector& VehicleForward, const FVector& AimDir,
        double ForwardBlockHalfAngle = ForwardBlockHalfAngleRad);

    /** Which side the aim points to: +1 right, -1 left, 0 straight fore/aft. */
    static double FiringSide(const FVector& VehicleRight, const FVector& AimDir);

    /** Extra aim spread (radians) from driving at SpeedFraction (0..1 of top speed). */
    static double SpeedSpread(double SpeedFraction);

    /** Spread to feed FWeaponBallistics: the weapon's base spread plus the speed penalty. */
    static double EffectiveSpread(double BaseSpreadRad, double SpeedFraction);
};
