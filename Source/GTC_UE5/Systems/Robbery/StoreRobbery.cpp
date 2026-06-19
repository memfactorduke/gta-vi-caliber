// Copyright Epic Games, Inc. All Rights Reserved.

#include "StoreRobbery.h"

#include "Math/UnrealMathUtility.h"

double FStoreRobbery::IntimidationFrom(bool bArmed, double GunTier01, bool bAimedAtClerk)
{
    if (!bArmed)
    {
        return 0.1; // a raised fist is barely a threat to a clerk behind glass
    }
    const double Base = bAimedAtClerk ? 0.7 : 0.3; // levelled vs merely holstered/visible
    const double Tier = FMath::Clamp(GunTier01, 0.0, 1.0);
    return FMath::Clamp(Base + Tier * 0.3, 0.0, 1.0);
}

void FStoreRobbery::Update(double Dt, bool bThreatening, double Intimidation)
{
    if (Dt <= 0.0)
    {
        return;
    }

    // Fear rises under a sustained threat, ebbs when it lets up.
    if (bThreatening)
    {
        const double Gain = FMath::Clamp(Intimidation, 0.0, 1.0) * FearGainPerSec * Dt;
        Fear = FMath::Clamp(Fear + Gain, 0.0, 1.0);
    }
    else
    {
        Fear = FMath::Clamp(Fear - FearDecayPerSec * Dt, 0.0, 1.0);
    }

    if (IsCooperating())
    {
        // Too scared to be brave — the alarm hand stays down and the till drains.
        AlarmCountdown = AlarmCountdownSec;
        if (RegisterTotal > 0.0 && !IsEmptied())
        {
            const double Rate = RegisterTotal / FMath::Max(EmptyDurationSec, 1e-6);
            CashTaken = FMath::Min(RegisterTotal, CashTaken + Rate * Dt);
        }
    }
    else if (!bAlarmRaised && !IsEmptied())
    {
        // A defiant clerk inches toward the silent alarm.
        AlarmCountdown -= Dt;
        if (AlarmCountdown <= 0.0)
        {
            AlarmCountdown = 0.0;
            bAlarmRaised = true;
        }
    }
}

double FStoreRobbery::Progress() const
{
    if (RegisterTotal <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(CashTaken / RegisterTotal, 0.0, 1.0);
}
