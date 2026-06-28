// Copyright Epic Games, Inc. All Rights Reserved.

#include "DownedStateComponent.h"

UDownedStateComponent::UDownedStateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UDownedStateComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyTuning();
}

void UDownedStateComponent::ApplyTuning()
{
    FDownedState::FParams Params;
    Params.BleedOutSeconds = static_cast<double>(BleedOutSeconds);
    Params.MaxDowns = MaxDowns;
    Model.Configure(Params);
}

EGtcDownedState UDownedStateComponent::GetState() const
{
    switch (Model.State())
    {
    case FDownedState::EState::Downed: return EGtcDownedState::Downed;
    case FDownedState::EState::Dead:   return EGtcDownedState::Dead;
    default:                           return EGtcDownedState::Up;
    }
}

void UDownedStateComponent::GoDown()
{
    Model.GoDown();
    if (Model.IsDowned())
    {
        OnDowned.Broadcast();
    }
    else if (Model.IsDead())
    {
        // Downs budget spent -> instant kill.
        OnDied.Broadcast();
    }
}

void UDownedStateComponent::Revive()
{
    const bool bWasDowned = Model.IsDowned();
    Model.Revive();
    if (bWasDowned && Model.IsUp())
    {
        OnRevived.Broadcast();
    }
}

void UDownedStateComponent::FinishOff()
{
    const bool bWasDowned = Model.IsDowned();
    Model.FinishOff();
    if (bWasDowned && Model.IsDead())
    {
        OnDied.Broadcast();
    }
}

void UDownedStateComponent::ResetDownedState()
{
    Model.Reset();
}

void UDownedStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Model.IsDowned())
    {
        return;
    }
    Model.Tick(static_cast<double>(DeltaTime));
    if (Model.IsDead())
    {
        OnDied.Broadcast();
    }
}
