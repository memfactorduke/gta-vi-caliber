// Copyright (c) 2026 GTC contributors

#include "StatTracker.h"

const TArray<FStatTracker::FAchievement>& FStatTracker::Achievements()
{
    // Mirrors the Godot ACHIEVEMENTS Dictionary in declaration order.
    static const TArray<FAchievement> Rules = {
        {TEXT("centurion"), TEXT("kills"), 100.0},
        {TEXT("sharpshooter"), TEXT("headshots"), 50.0},
        {TEXT("road_trip"), TEXT("distance_m"), 10000.0},
        {TEXT("made_man"), TEXT("missions_passed"), 10.0},
        {TEXT("grand_theft"), TEXT("vehicles_jacked"), 25.0},
    };
    return Rules;
}

FStatTracker::FStatTracker()
{
    Reset();
}

void FStatTracker::Store(const FString& StatId, double Value)
{
    if (const int32* Found = _Index.Find(StatId))
    {
        _Values[*Found] = Value;
        return;
    }
    const int32 NewIndex = _Order.Add(StatId);
    _Values.Add(Value);
    _Index.Add(StatId, NewIndex);
}

void FStatTracker::Add(const FString& StatId, double Amount)
{
    // Godot add(): negative amounts are ignored so a total never drifts backwards.
    if (Amount < 0.0)
    {
        return;
    }
    Store(StatId, GetStat(StatId) + Amount);
}

void FStatTracker::SetStat(const FString& StatId, double Value)
{
    Store(StatId, Value);
}

void FStatTracker::Reset()
{
    _Values.Reset();
    _Order.Reset();
    _Index.Reset();
}

double FStatTracker::GetStat(const FString& StatId) const
{
    if (const int32* Found = _Index.Find(StatId))
    {
        return _Values[*Found];
    }
    return 0.0;
}

TArray<TPair<FString, double>> FStatTracker::AllStats() const
{
    TArray<TPair<FString, double>> Out;
    Out.Reserve(_Order.Num());
    for (int32 i = 0; i < _Order.Num(); ++i)
    {
        Out.Emplace(_Order[i], _Values[i]);
    }
    return Out;
}

double FStatTracker::HeadshotRatio() const
{
    const double Kills = GetStat(TEXT("kills"));
    if (Kills <= 0.0)
    {
        return 0.0;
    }
    return GetStat(TEXT("headshots")) / Kills;
}

double FStatTracker::DistanceKm() const
{
    return GetStat(TEXT("distance_m")) / MetresPerKm;
}

bool FStatTracker::IsAchieved(const FString& AchievementId) const
{
    for (const FAchievement& Rule : Achievements())
    {
        if (Rule.Id == AchievementId)
        {
            return GetStat(Rule.Stat) >= Rule.Threshold;
        }
    }
    return false;
}

TArray<FString> FStatTracker::AchievedList() const
{
    TArray<FString> Earned;
    for (const FAchievement& Rule : Achievements())
    {
        if (IsAchieved(Rule.Id))
        {
            Earned.Add(Rule.Id);
        }
    }
    return Earned;
}

double FStatTracker::CompletionPercent() const
{
    const int32 Total = Achievements().Num();
    if (Total <= 0)
    {
        return 0.0;
    }
    return 100.0 * static_cast<double>(AchievedList().Num()) / static_cast<double>(Total);
}

TArray<TPair<FString, double>> FStatTracker::Serialize() const
{
    return AllStats();
}

void FStatTracker::Restore(const TArray<TPair<FString, double>>& Stats)
{
    Reset();
    for (const TPair<FString, double>& Pair : Stats)
    {
        Store(Pair.Key, Pair.Value);
    }
}
