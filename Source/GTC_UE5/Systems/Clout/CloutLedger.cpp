// Copyright Epic Games, Inc. All Rights Reserved.

#include "CloutLedger.h"

void FCloutLedger::Configure(const FParams& InParams)
{
    Params = InParams;
    FollowerCount = 0.0;
    // Start well-rested so the very first post earns full reach.
    HoursRested = FMath::Max(0.0, Params.FatigueWindow);
}

double FCloutLedger::CurrentReach() const
{
    // BaseReach grown by the audience you already have — the network effect.
    return FMath::Max(0.0, Params.BaseReach) + FMath::Max(0.0, Params.AudienceFactor) * FollowerCount;
}

double FCloutLedger::Post(double Appeal)
{
    const double A = FMath::Clamp(Appeal, 0.0, 1.0);
    const double Reach = CurrentReach();

    double Delta;
    if (A < Params.FlopAppeal)
    {
        // A flop: the worse the post (toward 0), the more of FlopLoss*reach it costs.
        const double Flop = Params.FlopAppeal > 0.0 ? (Params.FlopAppeal - A) / Params.FlopAppeal : 1.0;
        Delta = -FMath::Max(0.0, Params.FlopLoss) * Reach * Flop;
    }
    else
    {
        // Fatigue ramps from FatigueFloor (just posted) to 1 (rested past the window).
        const double Window = FMath::Max(0.0, Params.FatigueWindow);
        const double RestFrac = Window > 0.0 ? FMath::Clamp(HoursRested / Window, 0.0, 1.0) : 1.0;
        const double Fatigue = FMath::Lerp(FMath::Clamp(Params.FatigueFloor, 0.0, 1.0), 1.0, RestFrac);

        // Quality above the flop line, 0..1 — a post just above flop earns ~nothing.
        const double Denom = 1.0 - Params.FlopAppeal;
        const double Quality = Denom > 0.0 ? FMath::Clamp((A - Params.FlopAppeal) / Denom, 0.0, 1.0) : 1.0;

        double Viral = 1.0;
        if (A >= Params.ViralAppeal)
        {
            // Interpolate 1x at ViralAppeal up to ViralMultiplier at Appeal 1 — no jump at the line.
            const double ViralDenom = 1.0 - Params.ViralAppeal;
            const double ViralFrac = ViralDenom > 0.0 ? FMath::Clamp((A - Params.ViralAppeal) / ViralDenom, 0.0, 1.0) : 1.0;
            Viral = FMath::Lerp(1.0, FMath::Max(1.0, Params.ViralMultiplier), ViralFrac);
        }

        Delta = Reach * Quality * Fatigue * Viral;
    }

    FollowerCount = FMath::Max(0.0, FollowerCount + Delta);
    HoursRested = 0.0; // the audience was just posted to
    return Delta;
}

void FCloutLedger::Advance(double Dt)
{
    if (Dt <= 0.0)
    {
        return; // time only moves forward
    }

    HoursRested += Dt;

    // The audience cools while you're offline: shed up to DecayPerHour of the
    // following each idle hour (capped so a long Dt can't drive it negative).
    const double Shed = FMath::Clamp(FMath::Max(0.0, Params.DecayPerHour) * Dt, 0.0, 1.0);
    FollowerCount = FMath::Max(0.0, FollowerCount - FollowerCount * Shed);
}

FCloutLedger::ETier FCloutLedger::Tier() const
{
    if (FollowerCount >= Params.CelebrityAt)
    {
        return ETier::Celebrity;
    }
    if (FollowerCount >= Params.MacroAt)
    {
        return ETier::Macro;
    }
    if (FollowerCount >= Params.MicroAt)
    {
        return ETier::Micro;
    }
    if (FollowerCount >= Params.NanoAt)
    {
        return ETier::Nano;
    }
    return ETier::Unknown;
}
