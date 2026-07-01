// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeliveryRun.h"

void FDeliveryRun::Configure(const FParams& InParams)
{
    Params = InParams;
    CurrentState = EState::InProgress;
    TimeLeftValue = FMath::Max(0.0, Params.TimeLimitSeconds);
    ProgressValue = 0.0;
    CargoValue = 1.0;
}

void FDeliveryRun::Update(double ProgressDelta, double Damage, double Dt)
{
    if (CurrentState != EState::InProgress)
    {
        return;
    }
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    // Cargo first: wreck it and the run's over regardless of time or distance.
    CargoValue = FMath::Clamp(CargoValue - FMath::Max(0.0, Damage), 0.0, 1.0);
    if (CargoValue <= 0.0)
    {
        CurrentState = EState::Wrecked;
        return;
    }

    ProgressValue = FMath::Clamp(ProgressValue + FMath::Max(0.0, ProgressDelta), 0.0, 1.0);
    TimeLeftValue = FMath::Max(0.0, TimeLeftValue - Step);

    // Arrival is checked before the timeout, so reaching the drop on the buzzer counts.
    if (ProgressValue >= 1.0)
    {
        CurrentState = EState::Delivered;
        return;
    }
    if (TimeLeftValue <= 0.0)
    {
        CurrentState = EState::TooSlow;
    }
}

double FDeliveryRun::PayoutFactor() const
{
    if (CurrentState != EState::Delivered)
    {
        return 0.0;
    }
    const double Limit = FMath::Max<double>(KINDA_SMALL_NUMBER, Params.TimeLimitSeconds);
    const double TimeSpare = FMath::Clamp(TimeLeftValue / Limit, 0.0, 1.0);
    return FMath::Clamp((0.5 + 0.5 * TimeSpare) * CargoValue, 0.0, 1.0);
}
