// Copyright (c) 2026 GTC contributors

#include "WantedSubsystem.h"

void UWantedSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Rebuild the owned heat model from the configured tuning (the @export defaults
    // feed WantedSystem.new(decay_rate, heat_cap) in the reference _ready).
    _Wanted = FWantedSystem(DecayRate, HeatCap);
    _Evasion = FWantedEvasion();
    _PendingReports.Reset();
    _CrimeTimer = 0.0;
    _Stars = -1;
    RefreshStars();
}

void UWantedSubsystem::Deinitialize()
{
    _PendingReports.Reset();
    Super::Deinitialize();
}

int32 UWantedSubsystem::Stars() const
{
    return _Wanted.Stars();
}

bool UWantedSubsystem::IsWanted() const
{
    return _Wanted.IsWanted();
}

void UWantedSubsystem::ReportCrime(bool bKilled)
{
    _Wanted.AddCrime(bKilled ? KillHeat : WoundHeat);
    _CrimeTimer = CrimeActiveWindow;
    RefreshStars();
}

void UWantedSubsystem::ReportWitnessedCrime(
    bool bKilled, const FVector& CrimePos, const TArray<FCrimeObserver>& Observers)
{
    // The player just committed a crime (seen or not) — mark them actively committing
    // so heat holds through a rampage even before/without a witness.
    _CrimeTimer = CrimeActiveWindow;

    const FCollectedWitnesses Seen = FCrimeWitness::CollectWitnesses(
        CrimePos,
        Observers,
        PedSightRange,
        FMath::DegreesToRadians(PedFovDegrees),
        PoliceSightRange,
        FMath::DegreesToRadians(PoliceFovDegrees));

    if (Seen.Witnesses.Num() == 0)
    {
        return;
    }

    const double Base = (bKilled ? KillHeat : WoundHeat) * 2.0;
    const double Heat = FCrimeWitness::HeatForCrime(Base, Seen.Witnesses.Num());

    if (Seen.bPoliceSaw)
    {
        // A cop saw it — no phone call needed, the report is instant.
        _Wanted.AddCrime(Heat);
        RefreshStars();
        return;
    }

    // Civilian witnesses: queue a delayed report. The the reference version also stashes the
    // witness Node3D list to silence the report if they all die — that liveness probe is
    // a Wave-3 scene concern and is DEFERRED; the report simply ticks to completion.
    _PendingReports.Emplace(ReportDelaySec, Heat);
}

void UWantedSubsystem::TickFrame(double Delta)
{
    // Hold heat while the player is still actively committing crimes; only decay once
    // the rampage has paused.
    _CrimeTimer = FMath::Max(_CrimeTimer - Delta, 0.0);
    _Wanted.Tick(Delta, _CrimeTimer > 0.0);

    // Advance in-flight witness calls; completed calls convert into heat.
    if (_PendingReports.Num() > 0)
    {
        for (int32 i = _PendingReports.Num() - 1; i >= 0; --i)
        {
            FPendingReport& Entry = _PendingReports[i];
            Entry.Report.Tick(Delta);
            if (Entry.Report.IsReported())
            {
                _Wanted.AddCrime(Entry.Heat);
                _PendingReports.RemoveAt(i);
            }
        }
    }

    RefreshStars();
}

void UWantedSubsystem::Clear()
{
    _Wanted.Heat = 0.0;
    // Drop any in-flight witness reports too: an arrest/death clear must not be undone
    // seconds later when a queued report lands and re-applies the cleared heat.
    _PendingReports.Reset();
    RefreshStars();
}

void UWantedSubsystem::RestoreHeat(double Heat)
{
    _Wanted.Heat = FMath::Max(Heat, 0.0);
    RefreshStars();
}

void UWantedSubsystem::RefreshStars()
{
    const int32 Current = _Wanted.Stars();
    if (Current != _Stars)
    {
        _Stars = Current;
        OnStarsChanged.Broadcast(Current);
    }
}
