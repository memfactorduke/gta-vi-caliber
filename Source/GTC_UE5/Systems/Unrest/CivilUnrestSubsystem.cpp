// Copyright Epic Games, Inc. All Rights Reserved.

#include "CivilUnrestSubsystem.h"

FCivilUnrest::FParams UCivilUnrestSubsystem::MakeParams() const
{
    FCivilUnrest::FParams Params;
    Params.Flashpoint = static_cast<double>(Flashpoint);
    Params.TenseAt = static_cast<double>(TenseAt);
    Params.DecayPerSec = static_cast<double>(DecayPerSec);
    Params.GrowthPerSec = static_cast<double>(GrowthPerSec);
    return Params;
}

EGtcUnrestStage UCivilUnrestSubsystem::MapStage(FCivilUnrest::EStage Stage)
{
    switch (Stage)
    {
    case FCivilUnrest::EStage::Tense:   return EGtcUnrestStage::Tense;
    case FCivilUnrest::EStage::Rioting: return EGtcUnrestStage::Rioting;
    default:                            return EGtcUnrestStage::Calm;
    }
}

UCivilUnrestSubsystem::FDistrict& UCivilUnrestSubsystem::FindOrAddDistrict(const FString& District)
{
    if (FDistrict* Existing = Districts.Find(District))
    {
        return *Existing;
    }
    FDistrict& Added = Districts.Add(District);
    Added.Model.Configure(MakeParams());
    Added.LastStage = MapStage(Added.Model.Stage());
    return Added;
}

void UCivilUnrestSubsystem::DetectStageChange(const FString& District, FDistrict& D)
{
    const EGtcUnrestStage Stage = MapStage(D.Model.Stage());
    if (Stage != D.LastStage)
    {
        D.LastStage = Stage;
        OnDistrictStageChanged.Broadcast(District, Stage);
    }
}

void UCivilUnrestSubsystem::RegisterDistrict(const FString& District)
{
    FDistrict& D = FindOrAddDistrict(District);
    D.Model.Configure(MakeParams());
    D.LastStage = MapStage(D.Model.Stage());
}

void UCivilUnrestSubsystem::Provoke(const FString& District, float Amount)
{
    FDistrict& D = FindOrAddDistrict(District);
    D.Model.Provoke(static_cast<double>(Amount));
    DetectStageChange(District, D);
}

void UCivilUnrestSubsystem::Suppress(const FString& District, float Amount)
{
    FDistrict& D = FindOrAddDistrict(District);
    D.Model.Suppress(static_cast<double>(Amount));
    DetectStageChange(District, D);
}

void UCivilUnrestSubsystem::AdvanceAll(float DeltaSeconds)
{
    for (TPair<FString, FDistrict>& Pair : Districts)
    {
        Pair.Value.Model.Advance(static_cast<double>(DeltaSeconds));
        DetectStageChange(Pair.Key, Pair.Value);
    }
}

float UCivilUnrestSubsystem::GetUnrest(const FString& District) const
{
    if (const FDistrict* D = Districts.Find(District))
    {
        return static_cast<float>(D->Model.Unrest());
    }
    return 0.0f;
}

EGtcUnrestStage UCivilUnrestSubsystem::GetStage(const FString& District) const
{
    if (const FDistrict* D = Districts.Find(District))
    {
        return MapStage(D->Model.Stage());
    }
    return EGtcUnrestStage::Calm;
}

bool UCivilUnrestSubsystem::IsRioting(const FString& District) const
{
    if (const FDistrict* D = Districts.Find(District))
    {
        return D->Model.IsRioting();
    }
    return false;
}
