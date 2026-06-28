// Copyright Epic Games, Inc. All Rights Reserved.

#include "HostageStandoffComponent.h"

UHostageStandoffComponent::UHostageStandoffComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHostageStandoffComponent::ApplyTuning()
{
    FHostageStandoff::FParams Params;
    Params.StrugglePerSec = static_cast<double>(StrugglePerSec);
    Params.MoveStruggleMult = static_cast<double>(MoveStruggleMult);
    Params.IntimidateRelief = static_cast<double>(IntimidateRelief);
    Model.Configure(Params);
}

EGtcHostageState UHostageStandoffComponent::GetState() const
{
    switch (Model.State())
    {
    case FHostageStandoff::EState::Holding:   return EGtcHostageState::Holding;
    case FHostageStandoff::EState::BrokeFree: return EGtcHostageState::BrokeFree;
    case FHostageStandoff::EState::Released:  return EGtcHostageState::Released;
    default:                                  return EGtcHostageState::None;
    }
}

void UHostageStandoffComponent::Grab()
{
    Model.Grab();
    if (Model.IsHolding())
    {
        OnHostageGrabbed.Broadcast();
    }
}

void UHostageStandoffComponent::UpdateHold(bool bMoving, float DeltaSeconds)
{
    const bool bWasHolding = Model.IsHolding();
    Model.Update(bMoving, static_cast<double>(DeltaSeconds));
    if (bWasHolding && Model.State() == FHostageStandoff::EState::BrokeFree)
    {
        OnHostageBrokeFree.Broadcast();
    }
}

float UHostageStandoffComponent::Intimidate()
{
    return static_cast<float>(Model.Intimidate());
}

void UHostageStandoffComponent::Release()
{
    const bool bWasHolding = Model.IsHolding();
    Model.Release();
    if (bWasHolding && Model.State() == FHostageStandoff::EState::Released)
    {
        OnHostageReleased.Broadcast();
    }
}
