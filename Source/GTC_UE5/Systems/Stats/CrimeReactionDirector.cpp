// Copyright (c) 2026 GTC contributors

#include "CrimeReactionDirector.h"

void UCrimeReactionDirector::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    EnsureModels();
}

void UCrimeReactionDirector::Deinitialize()
{
    _News.Reset();
    _Districts.Reset();
    Super::Deinitialize();
}

void UCrimeReactionDirector::EnsureModels() const
{
    if (!_News.IsValid())
    {
        _News = MakeUnique<FNewsBulletin>();
    }
    if (!_Districts.IsValid())
    {
        // Default roster (Godot DistrictEconomy.new() with no args).
        _Districts = MakeUnique<DistrictEconomy>();
    }
}

void UCrimeReactionDirector::OnStarsChanged(int32 Stars)
{
    EnsureModels();
    if (Stars > _LastStars)
    {
        const int32 Gained = Stars - _LastStars;
        _Districts->AddHeat(ActiveDistrict, HeatPerStar * static_cast<double>(Gained));
        const FString Kind = (Stars >= RampageStars) ? FString(TEXT("rampage")) : FString(TEXT("crime"));
        _News->Report(Kind, Stars, ActiveDistrict);
    }
    _LastStars = Stars;
}

void UCrimeReactionDirector::Tick(double DeltaSeconds)
{
    EnsureModels();
    if (HeatDecayPerSec > 0.0 && DeltaSeconds > 0.0)
    {
        _Districts->DecayAllHeat(HeatDecayPerSec * DeltaSeconds);
    }
}
