// Copyright (c) 2026 GTC contributors

#include "NpcReaction.h"

#include "Math/UnrealMathUtility.h"

double FNpcReaction::ThreatFrom(double PlayerSpeed, bool bArmed)
{
    double T = 0.0;
    if (bArmed)
    {
        T += 0.6;
    }
    T += FMath::Clamp(PlayerSpeed / 10.0, 0.0, 0.4); // full tilt adds up to 0.4
    return FMath::Clamp(T, 0.0, 1.0);
}

FString FNpcReaction::Decide(double Distance, double Threat, double Bravery, double Curiosity)
{
    if (Distance > NoticeRadius)
    {
        return TEXT("ignore");
    }

    // Fear wins first. A high threat lets fear reach out further than personal
    // space; a brave NPC shrugs off more of it.
    const double Fear = Threat - Bravery;
    const double FleeRange = PersonalRadius + Threat * 8.0;
    if (Fear > 0.0 && Distance <= FleeRange)
    {
        return TEXT("flee");
    }

    // Not scared — but is it interesting enough to stop and stare?
    if (Curiosity > 0.5 && Distance <= NoticeRadius)
    {
        return TEXT("gawk");
    }

    return TEXT("ignore");
}

bool FNpcReaction::CatchesPanic(double Distance, double Bravery)
{
    return Distance <= PanicRadius && Bravery < PanicNerve;
}
