// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcDuress.h"

#include "Math/UnrealMathUtility.h"

ENpcDuress FNpcDuress::Decide(double DistanceM, bool bArmed, double Bravery)
{
    if (!bArmed || DistanceM > FleeRangeM)
    {
        return ENpcDuress::None; // no weapon, or too far to be immediate duress
    }

    // Point-blank and short on nerve: freeze and comply — running would get you shot.
    if (DistanceM <= CowerRangeM && Bravery < CowerNerve)
    {
        return ENpcDuress::Cower;
    }

    // Otherwise run from the gun.
    return ENpcDuress::Flee;
}
