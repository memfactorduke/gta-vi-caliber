// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lockpick.h"

void FLockpick::Configure(const FParams& InParams)
{
    Params = InParams;
    Result = EOutcome::Picking;
    TurnProgress = 0.0;
    PickStress = 0.0;
}

double FLockpick::AlignmentAt(double Angle) const
{
    const double A = FMath::Clamp(Angle, 0.0, 1.0);
    const double Tol = FMath::Max(KINDA_SMALL_NUMBER, Params.Tolerance);
    return FMath::Clamp(1.0 - FMath::Abs(A - Params.SweetSpot) / Tol, 0.0, 1.0);
}

void FLockpick::Update(double PickAngle, double Tension, double Dt)
{
    if (Result != EOutcome::Picking)
    {
        return; // already opened or snapped
    }
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double T = FMath::Clamp(Tension, 0.0, 1.0);
    const double Quality = AlignmentAt(PickAngle);

    if (T > 0.0)
    {
        // Tension turns the lock by how aligned you are, and stresses the pick by how
        // far off you are.
        TurnProgress += T * Quality * FMath::Max(0.0, Params.TurnRatePerSec) * Step;
        PickStress += T * (1.0 - Quality) * FMath::Max(0.0, Params.StressRatePerSec) * Step;

        if (PickStress > FMath::Max(0.0, Params.PickStrength))
        {
            PickStress = FMath::Max(0.0, Params.PickStrength); // pin at the break point
            Result = EOutcome::Broken;
            return;
        }
        if (TurnProgress >= 1.0)
        {
            TurnProgress = 1.0;
            Result = EOutcome::Unlocked;
        }
    }
    else
    {
        // Ease off the tension and the pick un-stresses — reposition without snapping.
        PickStress = FMath::Max(0.0, PickStress - FMath::Max(0.0, Params.RelaxRatePerSec) * Step);
    }
}
