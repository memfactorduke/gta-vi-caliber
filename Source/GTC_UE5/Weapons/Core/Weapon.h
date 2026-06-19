// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "WeaponStats.h"

/**
 * Runtime firing state for one carried weapon: ammo, cooldown, reload, bloom.
 * Direct port of the Godot `Weapon` (RefCounted) at
 * `game/scripts/weapons/weapon.gd`.
 *
 * Holds mutable state but no scene access and no RNG — the owner ticks it, asks
 * CanFire(), calls Fire(), and does the raycast itself using the exposed Spread.
 * This keeps the whole fire/reload/bloom state machine unit-testable
 * (Tests/WeaponTest.cpp, prefix GTC.Weapons.Core.Weapon) against a plain
 * FWeaponStats. Double precision matches the GDScript float math.
 *
 * PURE-CORE boundary: this is ONLY the per-weapon ammo/cooldown/bloom state
 * machine. The WeaponController that ticks many of these, drives GAS abilities,
 * and fires line traces is a DEFERRED Wave-3 entry — NOT ported here.
 */
struct GTC_UE5_API FWeapon
{
    FWeaponStats Stats;

    /** Rounds currently in the magazine. */
    int32 Ammo = 0;

    /** Spare rounds carried outside the magazine. */
    int32 Reserve = 0;

    /** Current cone half-angle (radians); grows with each shot, recovers in Tick(). */
    double Spread = 0.0;

    /**
     * Construct from stats. StartReserve < 0 (the default) means "fill to
     * ReserveMax"; any non-negative value overrides the starting spare ammo.
     * The magazine starts full and the cone starts at BaseSpread.
     */
    explicit FWeapon(const FWeaponStats& WeaponStats, int32 StartReserve = -1);

    bool IsReloading() const;

    /** True when a shot is allowed: rounds loaded, off cooldown, not mid-reload. */
    bool CanFire() const;

    /**
     * Consume one round: decrement ammo, start the cooldown, bloom the cone.
     * Returns true if a shot actually went off (so the owner only raycasts then).
     */
    bool Fire();

    /**
     * Begin a reload if it would do anything (not already reloading, mag not full,
     * spare rounds available). Returns whether the reload started.
     */
    bool StartReload();

    /**
     * Advance cooldown, reload timer (refilling the mag on completion), and bloom
     * recovery for one frame.
     */
    void Tick(double Delta);

private:
    double Cooldown = 0.0;
    double ReloadRemaining = 0.0;

    void CompleteReload();
};
