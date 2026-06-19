// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponFireController.h"

FWeaponFireController::FWeaponFireController(const FWeaponStats& WeaponStats, int32 StartReserve)
    : Weapon(WeaponStats, StartReserve)
{
}

bool FWeaponFireController::IsAutomatic() const
{
    return Weapon.Stats.bAutomatic;
}

void FWeaponFireController::PressTrigger()
{
    bTriggerHeld = true;
}

void FWeaponFireController::ReleaseTrigger()
{
    bTriggerHeld = false;
    // Re-arm the semi-automatic latch so the next pull can fire.
    bAwaitingRelease = false;
}

bool FWeaponFireController::IsTriggerHeld() const
{
    return bTriggerHeld;
}

void FWeaponFireController::Tick(double Delta)
{
    Weapon.Tick(Delta);
}

bool FWeaponFireController::TryFire()
{
    if (!bTriggerHeld)
    {
        return false;
    }
    // A semi-automatic that already fired this pull waits for the trigger release.
    if (!IsAutomatic() && bAwaitingRelease)
    {
        return false;
    }
    if (!Weapon.CanFire())
    {
        return false;
    }

    const bool bFired = Weapon.Fire();
    if (bFired && !IsAutomatic())
    {
        bAwaitingRelease = true;
    }
    return bFired;
}

bool FWeaponFireController::Reload()
{
    return Weapon.StartReload();
}

bool FWeaponFireController::IsReloading() const
{
    return Weapon.IsReloading();
}

double FWeaponFireController::CurrentSpread() const
{
    return Weapon.Spread;
}

int32 FWeaponFireController::AmmoInMag() const
{
    return Weapon.Ammo;
}

int32 FWeaponFireController::Reserve() const
{
    return Weapon.Reserve;
}
