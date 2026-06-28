// Copyright Epic Games, Inc. All Rights Reserved.

#include "WildlifeBehaviorComponent.h"

UWildlifeBehaviorComponent::UWildlifeBehaviorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FWildlifeBehavior::FParams UWildlifeBehaviorComponent::MakeParams() const
{
    FWildlifeBehavior::FParams Params;
    switch (Temperament)
    {
    case EGtcTemperament::Placid:     Params.Temperament = FWildlifeBehavior::ETemperament::Placid; break;
    case EGtcTemperament::Aggressive: Params.Temperament = FWildlifeBehavior::ETemperament::Aggressive; break;
    default:                          Params.Temperament = FWildlifeBehavior::ETemperament::Skittish; break;
    }
    Params.NoticeRange = static_cast<double>(NoticeRange);
    Params.ReactRange = static_cast<double>(ReactRange);
    Params.CalmSeconds = static_cast<double>(CalmSeconds);
    return Params;
}

EGtcWildlifeState UWildlifeBehaviorComponent::GetState() const
{
    switch (Model.State())
    {
    case FWildlifeBehavior::EState::Alert:  return EGtcWildlifeState::Alert;
    case FWildlifeBehavior::EState::Flee:   return EGtcWildlifeState::Flee;
    case FWildlifeBehavior::EState::Charge: return EGtcWildlifeState::Charge;
    default:                                return EGtcWildlifeState::Idle;
    }
}

void UWildlifeBehaviorComponent::UpdateBehavior(float DistanceToThreat, bool bProvoked, float DeltaSeconds)
{
    Model.Update(static_cast<double>(DistanceToThreat), bProvoked, static_cast<double>(DeltaSeconds), MakeParams());

    const EGtcWildlifeState State = GetState();
    if (State != LastState)
    {
        LastState = State;
        OnWildlifeStateChanged.Broadcast(State);
    }
}
