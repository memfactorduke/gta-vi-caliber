// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "../Core/Weapon.h"

/**
 * Pure trigger/fire-cadence model for one held weapon. Wraps an FWeapon (the
 * ammo/cooldown/bloom state machine) and adds the GTA-style trigger semantics
 * that decide WHEN a shot is allowed to go out:
 *
 *   - Semi-automatic (bAutomatic == false): at most one shot per trigger pull.
 *     Holding the trigger does NOT keep firing; the trigger must be released and
 *     pulled again for the next round. This is the pistol / shotgun feel.
 *   - Automatic   (bAutomatic == true): keeps firing at the weapon's fire rate
 *     for as long as the trigger is held (gated only by the FWeapon cooldown,
 *     ammo, and reloads). This is the SMG / rifle feel.
 *
 * Scene-free and RNG-free: the owning UGTCWeaponComponent presses/releases the
 * trigger from input, ticks this each frame, and calls TryFire() to learn
 * whether a round left the barrel this frame (the component then does the world
 * line trace using FWeaponBallistics). Keeping the cadence here makes the
 * semi-vs-auto latch and cooldown gating unit-testable headless
 * (Tests/WeaponFireControllerTest.cpp, prefix GTC.Weapons.Firing.FireController).
 */
struct GTC_UE5_API FWeaponFireController
{
    /** The underlying ammo/cooldown/bloom/reload state machine. */
    FWeapon Weapon;

    /**
     * Construct around a weapon archetype. StartReserve < 0 fills to ReserveMax
     * (see FWeapon). The trigger starts released.
     */
    explicit FWeaponFireController(const FWeaponStats& WeaponStats, int32 StartReserve = -1);

    /** True for full-auto weapons (hold to keep firing). */
    bool IsAutomatic() const;

    /** Begin holding the trigger (input "pressed"). */
    void PressTrigger();

    /** Release the trigger — also re-arms a semi-automatic for its next shot. */
    void ReleaseTrigger();

    /** Whether the trigger is currently held. */
    bool IsTriggerHeld() const;

    /** Advance cooldown, reload, and bloom recovery by Delta seconds. */
    void Tick(double Delta);

    /**
     * Decide and consume a shot for this frame. Returns true exactly when a round
     * leaves the barrel — the owner should then line-trace. Honors the trigger
     * state: a released trigger never fires; a semi-automatic fires only once per
     * pull; an automatic fires whenever it is off cooldown with ammo loaded.
     */
    bool TryFire();

    /** Begin a reload from reserve (delegates to FWeapon::StartReload). */
    bool Reload();

    bool IsReloading() const;

    /** Current cone half-angle (radians) the next shot would use. */
    double CurrentSpread() const;

    int32 AmmoInMag() const;
    int32 Reserve() const;

private:
    /** Trigger physically held down. */
    bool bTriggerHeld = false;

    /**
     * Semi-auto latch: set when a semi weapon fires, cleared on trigger release.
     * Blocks a held semi-auto from auto-repeating.
     */
    bool bAwaitingRelease = false;
};
