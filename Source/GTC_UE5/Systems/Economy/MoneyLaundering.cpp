// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoneyLaundering.h"

double FMoneyLaundering::SpaceRemaining() const
{
    return FMath::Max(0.0, FMath::Max(0.0, Params.Capacity) - Dirty);
}

double FMoneyLaundering::Deposit(double DirtyAmount)
{
    if (DirtyAmount <= 0.0)
    {
        return 0.0;
    }
    // Take only what fits under the capacity ceiling.
    const double Accepted = FMath::Min(DirtyAmount, SpaceRemaining());
    Dirty += Accepted;
    return Accepted;
}

double FMoneyLaundering::Advance(double Dt)
{
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0 || Dirty <= 0.0)
    {
        return 0.0;
    }

    // Launder at most the throughput allows, and never more than is in the pool.
    const double Capacity = FMath::Max(0.0, Params.ThroughputPerHour) * Step;
    const double Laundered = FMath::Min(Dirty, Capacity);

    const double Fee = Laundered * FMath::Clamp(Params.Cut, 0.0, 1.0);
    const double Clean = Laundered - Fee;

    Dirty -= Laundered;
    FeesTaken += Fee;
    return Clean;
}
