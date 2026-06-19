// Copyright (c) 2026 GTC contributors

#include "WeaponStats.h"

FWeaponStats FWeaponStats::Pistol()
{
    FWeaponStats S;
    S.DisplayName = TEXT("Pistol");
    S.SoundKey = TEXT("pistol");
    S.bAutomatic = false;
    S.FireRate = 5.0;
    S.MagSize = 12;
    S.ReserveMax = 72;
    S.ReloadTime = 1.3;
    S.Damage = 22.0;
    S.Range = 90.0;
    S.BaseSpread = 0.010;
    S.SpreadPerShot = 0.022;
    S.MaxSpread = 0.12;
    S.SpreadRecovery = 0.30;
    S.RecoilKick = 0.020;
    return S;
}

FWeaponStats FWeaponStats::Smg()
{
    FWeaponStats S;
    S.DisplayName = TEXT("SMG");
    S.SoundKey = TEXT("smg");
    S.bAutomatic = true;
    S.FireRate = 13.0;
    S.MagSize = 30;
    S.ReserveMax = 180;
    S.ReloadTime = 1.8;
    S.Damage = 14.0;
    S.Range = 80.0;
    S.BaseSpread = 0.018;
    S.SpreadPerShot = 0.016;
    S.MaxSpread = 0.17;
    S.SpreadRecovery = 0.22;
    S.RecoilKick = 0.012;
    return S;
}

FWeaponStats FWeaponStats::Rifle()
{
    FWeaponStats S;
    S.DisplayName = TEXT("Rifle");
    S.SoundKey = TEXT("rifle");
    S.bAutomatic = true;
    S.FireRate = 9.0;
    S.MagSize = 30;
    S.ReserveMax = 150;
    S.ReloadTime = 2.1;
    S.Damage = 26.0;
    S.Range = 160.0;
    S.BaseSpread = 0.008;
    S.SpreadPerShot = 0.018;
    S.MaxSpread = 0.15;
    S.SpreadRecovery = 0.26;
    S.RecoilKick = 0.022;
    return S;
}

FWeaponStats FWeaponStats::Shotgun()
{
    FWeaponStats S;
    S.DisplayName = TEXT("Shotgun");
    S.SoundKey = TEXT("shotgun");
    S.bAutomatic = false;
    S.FireRate = 1.6;
    S.MagSize = 6;
    S.ReserveMax = 36;
    S.ReloadTime = 2.6;
    S.Damage = 9.0;
    S.Range = 45.0;
    S.DamageFalloffStart = 8.0;
    S.DamageFalloffEnd = 30.0;
    S.MinDamageFraction = 0.2;
    S.BaseSpread = 0.08;
    S.SpreadPerShot = 0.0;
    S.MaxSpread = 0.08;
    S.SpreadRecovery = 0.5;
    S.Pellets = 9;
    S.RecoilKick = 0.045;
    return S;
}
