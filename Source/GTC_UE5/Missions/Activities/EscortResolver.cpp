// Copyright Epic Games, Inc. All Rights Reserved.

#include "EscortResolver.h"

#include "Math/UnrealMathUtility.h"

FEscortResolver::FOutput FEscortResolver::Cook(const FInput& In, const FParams& P)
{
    FOutput Out;

    // Route progress: how far to the destination the escortee has EVER gotten, 0..1, reaching 1
    // once inside the arrival radius. Monotonic high-water, so being shoved back or stalling under
    // fire never un-banks covered ground.
    const double Radius = FMath::Max(0.0, P.ArrivalRadius);
    const double Reach = In.RouteLength - Radius;
    const double Dist = (In.EscorteePos - In.Destination).Size();
    double CurFrac;
    if (Reach <= 0.0 || Dist <= Radius)
    {
        CurFrac = 1.0;
    }
    else
    {
        CurFrac = FMath::Clamp((In.RouteLength - Dist) / Reach, 0.0, 1.0);
    }
    const double HighWater = FMath::Clamp(In.PrevProgress, 0.0, 1.0);
    Out.Progress = FMath::Max(HighWater, CurFrac);
    Out.ProgressDelta = Out.Progress - HighWater;

    // Threat: sum the proximity-weighted heat of every attacker near the escortee (closer = hotter),
    // saturating to 1 at ThreatSaturation point-blank attackers.
    const double ThreatRadius = FMath::Max(1.0, P.ThreatRadius);
    double Heat = 0.0;
    for (const FVector& Hostile : In.HostilePositions)
    {
        const double D = (Hostile - In.EscorteePos).Size();
        if (D < ThreatRadius)
        {
            Heat += 1.0 - D / ThreatRadius;
        }
    }
    Out.ThreatLevel = FMath::Clamp(Heat / FMath::Max(1.e-4, P.ThreatSaturation), 0.0, 1.0);

    return Out;
}
