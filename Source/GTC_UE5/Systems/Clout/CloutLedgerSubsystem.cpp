// Copyright Epic Games, Inc. All Rights Reserved.

#include "CloutLedgerSubsystem.h"

void UCloutLedgerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ApplyTuning();
}

void UCloutLedgerSubsystem::ApplyTuning()
{
    FCloutLedger::FParams Params;
    Params.BaseReach = static_cast<double>(BaseReach);
    Params.AudienceFactor = static_cast<double>(AudienceFactor);
    Params.ViralAppeal = static_cast<double>(ViralAppeal);
    Params.ViralMultiplier = static_cast<double>(ViralMultiplier);
    Params.DecayPerHour = static_cast<double>(DecayPerHour);
    Params.NanoAt = static_cast<double>(NanoAt);
    Params.MicroAt = static_cast<double>(MicroAt);
    Params.MacroAt = static_cast<double>(MacroAt);
    Params.CelebrityAt = static_cast<double>(CelebrityAt);
    Model.Configure(Params);
    LastTier = GetTier();
}

EGtcCloutTier UCloutLedgerSubsystem::GetTier() const
{
    switch (Model.Tier())
    {
    case FCloutLedger::ETier::Nano:      return EGtcCloutTier::Nano;
    case FCloutLedger::ETier::Micro:     return EGtcCloutTier::Micro;
    case FCloutLedger::ETier::Macro:     return EGtcCloutTier::Macro;
    case FCloutLedger::ETier::Celebrity: return EGtcCloutTier::Celebrity;
    default:                             return EGtcCloutTier::Unknown;
    }
}

float UCloutLedgerSubsystem::Post(float Appeal)
{
    const double NetChange = Model.Post(static_cast<double>(Appeal));
    OnPosted.Broadcast(static_cast<float>(NetChange));
    BroadcastFollowers();
    return static_cast<float>(NetChange);
}

void UCloutLedgerSubsystem::Advance(float Hours)
{
    Model.Advance(static_cast<double>(Hours));
    BroadcastFollowers();
}

void UCloutLedgerSubsystem::BroadcastFollowers()
{
    OnFollowersChanged.Broadcast(static_cast<float>(Model.Followers()));
    const EGtcCloutTier Tier = GetTier();
    if (Tier != LastTier)
    {
        LastTier = Tier;
        OnCloutTierChanged.Broadcast(Tier);
    }
}
