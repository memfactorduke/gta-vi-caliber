// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeliveryRunResolver.h"

#include "Math/UnrealMathUtility.h"

FDeliveryRunResolver::FOutput FDeliveryRunResolver::Cook(const FInput& In, const FParams& P)
{
    FOutput Out;

    // Current absolute route progress: how far toward the drop you are now, 0..1, reaching 1.0
    // once inside the arrival radius (you never sit EXACTLY on an authored point). The distance
    // that must be closed to arrive is the route minus that radius; a route already within the
    // radius (or degenerate) is simply "there".
    const double Radius = FMath::Max(0.0, P.ArrivalRadius);
    const double Reach = In.RouteLength - Radius;
    double CurFrac;
    if (Reach <= 0.0 || In.DistanceToDropoff <= Radius)
    {
        CurFrac = 1.0;
    }
    else
    {
        CurFrac = FMath::Clamp((In.RouteLength - In.DistanceToDropoff) / Reach, 0.0, 1.0);
    }

    // Monotonic high-water mark: progress never falls, and only ground you have not covered before
    // counts — so driving away from the drop and back does not re-award the same stretch of route.
    const double HighWater = FMath::Clamp(In.PrevProgress, 0.0, 1.0);
    Out.Progress = FMath::Max(HighWater, CurFrac);
    Out.ProgressDelta = Out.Progress - HighWater;

    // Crashes into cargo damage: this tick's accumulated impact impulse scaled into 0..1 cargo loss.
    Out.Damage = FMath::Max(0.0, In.ImpactImpulse) * FMath::Max(0.0, P.DamagePerImpulse);

    return Out;
}
