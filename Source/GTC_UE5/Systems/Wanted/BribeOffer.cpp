// Copyright Epic Games, Inc. All Rights Reserved.

#include "BribeOffer.h"

void FBribeOffer::Configure(const FParams& InParams)
{
    Params = InParams;
    BribesInWindow = 0;
    IdleHours = 0.0;
}

void FBribeOffer::Advance(double Dt)
{
    if (Dt <= 0.0)
    {
        return;
    }
    IdleHours += Dt;
    if (IdleHours >= FMath::Max(0.0, Params.CooldownHours))
    {
        BribesInWindow = 0; // gone straight long enough — the price resets
    }
}

double FBribeOffer::QuoteCost(int32 Stars) const
{
    // No price on a bribe that isn't on the table.
    if (Stars <= 0 || Stars > Params.MaxBribableStars)
    {
        return 0.0;
    }
    const double Base = FMath::Max(0.0, Params.CostPerStar) * Stars;
    const double Greed = 1.0 + FMath::Max(0.0, Params.GreedPerBribe) * BribesInWindow;
    return Base * Greed;
}

bool FBribeOffer::CanBribe(int32 Stars, double Cash) const
{
    if (Stars <= 0 || Stars > Params.MaxBribableStars)
    {
        return false; // no heat, or too hot to buy off
    }
    return Cash >= QuoteCost(Stars);
}

FBribeOffer::FResult FBribeOffer::Bribe(int32 Stars, double Cash)
{
    FResult Result;
    Result.Cost = QuoteCost(Stars);
    Result.StarsAfter = Stars;

    if (!CanBribe(Stars, Cash))
    {
        return Result; // declined: too hot, no heat, or can't afford
    }

    Result.bAccepted = true;
    Result.StarsAfter = FMath::Max(0, Stars - FMath::Max(0, Params.StarsReducedPerBribe));

    ++BribesInWindow; // officials get greedier
    IdleHours = 0.0;  // and the cooldown restarts
    return Result;
}
