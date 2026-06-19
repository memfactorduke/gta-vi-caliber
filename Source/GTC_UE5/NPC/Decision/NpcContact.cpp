// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcContact.h"

#include "Math/UnrealMathUtility.h"

double FNpcContact::Severity(double ImpactSpeed, bool bStrike)
{
    const double Approach = FMath::Max(0.0, ImpactSpeed); // m/s, never negative

    if (bStrike)
    {
        // A deliberate blow is serious even thrown from a standstill; the swing
        // speed escalates it toward a flooring hit. Reaches 1.0 at StrikeSwingSpeed.
        const double Swing = FMath::Clamp(Approach / StrikeSwingSpeed, 0.0, 1.0);
        return FMath::Clamp(StrikeBase + (1.0 - StrikeBase) * Swing, 0.0, 1.0);
    }

    // A body bump/shove scales with how hard you barged in — but you can't floor
    // someone just by walking into them, so it caps below the knockdown band.
    const double Raw = FMath::Clamp(Approach / SlamSpeed, 0.0, 1.0);
    return FMath::Min(Raw, BodyMax);
}

ENpcContactReaction FNpcContact::Decide(double Severity, double Bravery, double Alarm)
{
    const double S = FMath::Clamp(Severity, 0.0, 1.0);
    const bool bRattled = FMath::Clamp(Alarm, 0.0, 1.0) >= RattledThreshold;

    // A floor-force blow takes anyone off their feet, brave or not.
    if (S >= FloorMin)
    {
        return ENpcContactReaction::Knockdown;
    }

    // Light contact: a calm citizen is merely inconvenienced; a jumpy one flinches.
    if (S <= GrazeMax)
    {
        return bRattled ? ENpcContactReaction::Flinch : ENpcContactReaction::Excuse;
    }
    if (S <= BumpMax)
    {
        return bRattled ? ENpcContactReaction::Flinch : ENpcContactReaction::Annoyed;
    }

    // A real shove or hit (BumpMax..FloorMin): nerve decides fight or flight.
    if (Bravery >= ConfrontNerve)
    {
        return ENpcContactReaction::Confront;
    }
    // The timid recoil from a moderate shove but break and run from a hard one.
    return (S <= HardMax) ? ENpcContactReaction::Flinch : ENpcContactReaction::Flee;
}

double FNpcContact::Duration(ENpcContactReaction R)
{
    switch (R)
    {
        case ENpcContactReaction::Excuse:    return 1.0;
        case ENpcContactReaction::Annoyed:   return 1.6;
        case ENpcContactReaction::Flinch:    return 1.0;
        case ENpcContactReaction::Confront:  return 3.0;
        case ENpcContactReaction::Flee:      return 2.5;
        case ENpcContactReaction::Knockdown: return 2.2;
        default:                             return 0.0;
    }
}

double FNpcContact::Knockback(ENpcContactReaction R, double Severity)
{
    const double S = FMath::Clamp(Severity, 0.0, 1.0);
    switch (R)
    {
        case ENpcContactReaction::Knockdown: return 320.0 + 480.0 * S; // a real launch
        case ENpcContactReaction::Flinch:    return 140.0 * S;         // a recoil step
        default:                             return 0.0;
    }
}

double FNpcContact::MemoryBump(ENpcContactReaction R, double Severity)
{
    const double S = FMath::Clamp(Severity, 0.0, 1.0);
    switch (R)
    {
        case ENpcContactReaction::Knockdown: return FMath::Clamp(0.7 + 0.3 * S, 0.0, 1.0);
        case ENpcContactReaction::Flee:      return 0.6;
        case ENpcContactReaction::Confront:  return 0.5;
        case ENpcContactReaction::Flinch:    return 0.25;
        case ENpcContactReaction::Annoyed:   return 0.1;
        default:                             return 0.0; // Excuse / Ignore leave no mark
    }
}
