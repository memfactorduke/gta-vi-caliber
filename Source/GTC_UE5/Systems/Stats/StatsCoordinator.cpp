// Copyright (c) 2026 GTC contributors

#include "StatsCoordinator.h"

void UStatsCoordinator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    EnsureModel();
}

void UStatsCoordinator::Deinitialize()
{
    _Stats.Reset();
    Super::Deinitialize();
}

void UStatsCoordinator::EnsureModel() const
{
    if (!_Stats.IsValid())
    {
        _Stats = MakeUnique<FStatTracker>();
    }
}

void UStatsCoordinator::OnMissionPassed()
{
    EnsureModel();
    _Stats->Add(TEXT("missions_passed"), 1.0);
}

void UStatsCoordinator::OnPlayerDied()
{
    // Only arm if currently wanted, so a death while clean can't suppress a later genuine
    // evasion. Mirrors Godot _on_player_died.
    if (_bWasWanted)
    {
        _bDeathClearedWanted = true;
    }
}

void UStatsCoordinator::UpdateWanted(bool bWanted)
{
    EnsureModel();
    // A wanted level that rises then clears counts as one "evaded" bust dodge.
    if (_bWasWanted && !bWanted)
    {
        if (_bDeathClearedWanted)
        {
            // Consume: this clear was a death, not an escape.
            _bDeathClearedWanted = false;
        }
        else
        {
            _Stats->Add(TEXT("busts_evaded"), 1.0);
        }
    }
    _bWasWanted = bWanted;
}

double UStatsCoordinator::Stat(const FString& StatId) const
{
    EnsureModel();
    return _Stats->GetStat(StatId);
}

double UStatsCoordinator::CompletionPercent() const
{
    EnsureModel();
    return _Stats->CompletionPercent();
}

TArray<TPair<FString, double>> UStatsCoordinator::SerializeState() const
{
    EnsureModel();
    return _Stats->Serialize();
}

void UStatsCoordinator::Restore(const TArray<TPair<FString, double>>& Data)
{
    EnsureModel();
    _Stats->Restore(Data);
    _bWasWanted = false;
    _bDeathClearedWanted = false;
}
