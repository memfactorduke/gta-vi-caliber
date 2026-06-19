// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Tunable data for one weapon archetype. Direct port of the Godot `WeaponStats`
 * (a Resource) at `game/scripts/weapons/weapon_stats.gd`.
 *
 * In Godot this is a Resource so designers can author `.tres` variants; here it
 * is a plain value struct (the intra-entry dependency of FWeapon — there is no
 * dedicated test oracle, WeaponStats is exercised through FWeapon). The firing
 * logic that consumes it (FWeapon, FWeaponBallistics) stays pure and unit-tested.
 *
 * Distances are metres, angles radians, times seconds, rates per second. The
 * preset factories are the source of truth for the four launch archetypes until
 * balance .tres files exist. Literal values are ported verbatim as double.
 */
struct GTC_UE5_API FWeaponStats
{
    /** Display name shown in the HUD / weapon wheel. */
    FString DisplayName = TEXT("Weapon");

    /** Audio class key -> assets/audio/weapons/<SoundKey>.wav. Decoupled from DisplayName. */
    FString SoundKey = TEXT("pistol");

    /** Held down to keep firing (true) vs one shot per trigger pull (false). */
    bool bAutomatic = false;

    /** Rounds per second; the inverse is the per-shot cooldown. */
    double FireRate = 6.0;

    int32 MagSize = 12;

    /** Maximum spare rounds carried outside the magazine. */
    int32 ReserveMax = 60;

    double ReloadTime = 1.6;

    /** Damage at or inside DamageFalloffStart. */
    double Damage = 18.0;

    /** Hitscan reach; beyond this a shot hits nothing. */
    double Range = 120.0;

    /** Damage stays full up to _Start, then lerps to Damage * MinDamageFraction by _End. */
    double DamageFalloffStart = 25.0;
    double DamageFalloffEnd = 90.0;
    double MinDamageFraction = 0.45;

    /** Cone half-angle (rad) when calm/aiming — the tightest the weapon ever is. */
    double BaseSpread = 0.012;

    /** Added to the cone on each shot (bloom), clamped to MaxSpread. */
    double SpreadPerShot = 0.02;
    double MaxSpread = 0.16;

    /** Bloom recovery toward BaseSpread (rad/s) while not firing. */
    double SpreadRecovery = 0.22;

    /** Projectiles per trigger pull (>1 = shotgun); each gets its own spread sample. */
    int32 Pellets = 1;

    /** Upward camera kick (rad) applied per shot. */
    double RecoilKick = 0.018;

    static FWeaponStats Pistol();
    static FWeaponStats Smg();
    static FWeaponStats Rifle();
    static FWeaponStats Shotgun();
};
