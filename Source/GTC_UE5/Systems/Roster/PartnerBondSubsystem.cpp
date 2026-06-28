// Copyright Epic Games, Inc. All Rights Reserved.

#include "PartnerBondSubsystem.h"

void UPartnerBondSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ApplyTuning();
}

void UPartnerBondSubsystem::ApplyTuning()
{
    FPartnerBond::FParams Params;
    Params.StartBond = static_cast<double>(StartBond);
    Params.Baseline = static_cast<double>(Baseline);
    Params.WarmGain = static_cast<double>(WarmGain);
    Params.CoolLoss = static_cast<double>(CoolLoss);
    Params.DecayPerHour = static_cast<double>(DecayPerHour);
    Params.WaryAt = static_cast<double>(WaryAt);
    Params.SolidAt = static_cast<double>(SolidAt);
    Params.RideOrDieAt = static_cast<double>(RideOrDieAt);
    Model.Configure(Params);
    LastTier = GetTier();
}

EGtcBondTier UPartnerBondSubsystem::GetTier() const
{
    switch (Model.Tier())
    {
    case FPartnerBond::ETier::Wary:      return EGtcBondTier::Wary;
    case FPartnerBond::ETier::Solid:     return EGtcBondTier::Solid;
    case FPartnerBond::ETier::RideOrDie: return EGtcBondTier::RideOrDie;
    default:                             return EGtcBondTier::Estranged;
    }
}

float UPartnerBondSubsystem::Warm(float Strength)
{
    const double Delta = Model.Warm(static_cast<double>(Strength));
    BroadcastChange();
    return static_cast<float>(Delta);
}

float UPartnerBondSubsystem::Cool(float Strength)
{
    const double Delta = Model.Cool(static_cast<double>(Strength));
    BroadcastChange();
    return static_cast<float>(Delta);
}

void UPartnerBondSubsystem::Advance(float Hours)
{
    Model.Advance(static_cast<double>(Hours));
    BroadcastChange();
}

void UPartnerBondSubsystem::BroadcastChange()
{
    OnBondChanged.Broadcast(static_cast<float>(Model.Bond()));
    const EGtcBondTier Tier = GetTier();
    if (Tier != LastTier)
    {
        LastTier = Tier;
        OnBondTierChanged.Broadcast(Tier);
    }
}
