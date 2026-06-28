// Copyright Epic Games, Inc. All Rights Reserved.

#include "FishingFightComponent.h"

UFishingFightComponent::UFishingFightComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UFishingFightComponent::StartFight()
{
    FFishingFight::FParams Params;
    Params.LineStrength = static_cast<double>(LineStrength);
    Params.ReelRate = static_cast<double>(ReelRate);
    Params.PullBack = static_cast<double>(PullBack);
    Model.Configure(Params);
}

EGtcFishingOutcome UFishingFightComponent::GetOutcome() const
{
    switch (Model.Outcome())
    {
    case FFishingFight::EOutcome::Landed: return EGtcFishingOutcome::Landed;
    case FFishingFight::EOutcome::Lost:   return EGtcFishingOutcome::Lost;
    default:                              return EGtcFishingOutcome::Fighting;
    }
}

void UFishingFightComponent::UpdateFight(float ReelInput, float DeltaSeconds)
{
    const bool bWasFighting = Model.IsFighting();
    Model.Update(static_cast<double>(ReelInput), static_cast<double>(DeltaSeconds));
    if (bWasFighting && !Model.IsFighting())
    {
        if (Model.IsLanded())
        {
            OnFishLanded.Broadcast();
        }
        else if (Model.IsLost())
        {
            OnFishLost.Broadcast();
        }
    }
}
