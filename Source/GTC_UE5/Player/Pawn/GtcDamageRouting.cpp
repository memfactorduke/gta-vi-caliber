// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcDamageRouting.h"

#include "../Stats/PlayerStats.h"
#include "../Health/PlayerHealthModel.h"

double GtcDamage::ApplyToPlayer(FPlayerStats& Stats, FPlayerHealthModel& Health, double Amount)
{
    // (1) Stats armor (the SOLE armor pool) soaks first; returns the overflow.
    const double Overflow = Stats.SoakDamage(Amount);
    // (2) Overflow hits the health model, which is health-only (ArmorMax == 0,
    //     so its armor pool soaks nothing — no second soak).
    Health.Apply(Overflow);
    return Overflow;
}
