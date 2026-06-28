// Copyright Epic Games, Inc. All Rights Reserved.

#include "PartnerBond.h"

void FPartnerBond::Configure(const FParams& InParams)
{
    Params = InParams;
    BondValue = FMath::Clamp(Params.StartBond, 0.0, 1.0);
}

double FPartnerBond::Warm(double Strength)
{
    const double S = FMath::Max(0.0, Strength);
    // Diminishing returns: only a slice of the room still left is earned.
    const double Gain = S * FMath::Max(0.0, Params.WarmGain) * (1.0 - BondValue);
    const double Before = BondValue;
    BondValue = FMath::Clamp(BondValue + Gain, 0.0, 1.0);
    return BondValue - Before;
}

double FPartnerBond::Cool(double Strength)
{
    const double S = FMath::Max(0.0, Strength);
    // Flat loss: trust is cheap to lose no matter how close you were.
    const double Loss = S * FMath::Max(0.0, Params.CoolLoss);
    const double Before = BondValue;
    BondValue = FMath::Clamp(BondValue - Loss, 0.0, 1.0);
    return BondValue - Before;
}

void FPartnerBond::Advance(double Dt)
{
    if (Dt <= 0.0)
    {
        return; // time only moves forward
    }

    const double Baseline = FMath::Clamp(Params.Baseline, 0.0, 1.0);
    const double Closed = FMath::Clamp(FMath::Max(0.0, Params.DecayPerHour) * Dt, 0.0, 1.0);
    BondValue = FMath::Clamp(BondValue + (Baseline - BondValue) * Closed, 0.0, 1.0);
}

FPartnerBond::ETier FPartnerBond::Tier() const
{
    if (BondValue >= Params.RideOrDieAt)
    {
        return ETier::RideOrDie;
    }
    if (BondValue >= Params.SolidAt)
    {
        return ETier::Solid;
    }
    if (BondValue >= Params.WaryAt)
    {
        return ETier::Wary;
    }
    return ETier::Estranged;
}
