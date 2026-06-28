// Copyright Epic Games, Inc. All Rights Reserved.

#include "LockpickComponent.h"

ULockpickComponent::ULockpickComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULockpickComponent::StartPick()
{
    FLockpick::FParams Params;
    Params.SweetSpot = static_cast<double>(SweetSpot);
    Params.Tolerance = static_cast<double>(Tolerance);
    Params.TurnRatePerSec = static_cast<double>(TurnRatePerSec);
    Params.StressRatePerSec = static_cast<double>(StressRatePerSec);
    Model.Configure(Params);
}

EGtcLockpickOutcome ULockpickComponent::GetOutcome() const
{
    switch (Model.Outcome())
    {
    case FLockpick::EOutcome::Unlocked: return EGtcLockpickOutcome::Unlocked;
    case FLockpick::EOutcome::Broken:   return EGtcLockpickOutcome::Broken;
    default:                            return EGtcLockpickOutcome::Picking;
    }
}

void ULockpickComponent::UpdatePick(float PickAngle, float Tension, float DeltaSeconds)
{
    const bool bWasPicking = Model.IsPicking();
    Model.Update(static_cast<double>(PickAngle), static_cast<double>(Tension), static_cast<double>(DeltaSeconds));
    if (bWasPicking && !Model.IsPicking())
    {
        if (Model.IsUnlocked())
        {
            OnLockUnlocked.Broadcast();
        }
        else if (Model.IsBroken())
        {
            OnPickBroken.Broadcast();
        }
    }
}
