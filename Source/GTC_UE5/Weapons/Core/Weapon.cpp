// Copyright (c) 2026 GTC contributors

#include "Weapon.h"

#include "Math/UnrealMathUtility.h"

FWeapon::FWeapon(const FWeaponStats& WeaponStats, int32 StartReserve)
{
    Stats = WeaponStats;
    Ammo = Stats.MagSize;
    Reserve = StartReserve < 0 ? Stats.ReserveMax : StartReserve;
    Spread = Stats.BaseSpread;
}

bool FWeapon::IsReloading() const
{
    return ReloadRemaining > 0.0;
}

bool FWeapon::CanFire() const
{
    return Ammo > 0 && Cooldown <= 0.0 && !IsReloading();
}

bool FWeapon::Fire()
{
    if (!CanFire())
    {
        return false;
    }
    Ammo -= 1;
    Cooldown = 1.0 / Stats.FireRate;
    Spread = FMath::Min(Spread + Stats.SpreadPerShot, Stats.MaxSpread);
    return true;
}

bool FWeapon::StartReload()
{
    if (IsReloading() || Ammo >= Stats.MagSize || Reserve <= 0)
    {
        return false;
    }
    ReloadRemaining = Stats.ReloadTime;
    return true;
}

void FWeapon::Tick(double Delta)
{
    if (Cooldown > 0.0)
    {
        Cooldown = FMath::Max(Cooldown - Delta, 0.0);
    }
    if (IsReloading())
    {
        ReloadRemaining -= Delta;
        if (ReloadRemaining <= 0.0)
        {
            ReloadRemaining = 0.0;
            CompleteReload();
        }
    }
    else
    {
        Spread = FMath::Max(Spread - Stats.SpreadRecovery * Delta, Stats.BaseSpread);
    }
}

void FWeapon::CompleteReload()
{
    const int32 Needed = Stats.MagSize - Ammo;
    const int32 Take = FMath::Min(Needed, Reserve);
    Ammo += Take;
    Reserve -= Take;
}
