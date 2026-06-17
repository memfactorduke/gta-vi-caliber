// Copyright (c) 2026 GTC contributors

#include "ProgressionTracker.h"

void UProgressionTracker::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // The owned FPlayerProgression is value-constructed (Reset to start) with the
    // subsystem; nothing else to wire here (mission-signal binding is Wave 3).
    _Progression.Reset();
}

void UProgressionTracker::Deinitialize()
{
    Super::Deinitialize();
}

void UProgressionTracker::AwardObjective()
{
    _Progression.AddXp(ObjectiveXp);
}

void UProgressionTracker::AwardMission()
{
    _Progression.AddXp(MissionXp);
}

int32 UProgressionTracker::Level() const
{
    return _Progression.Level();
}

double UProgressionTracker::LevelProgress() const
{
    return _Progression.LevelProgress();
}

int32 UProgressionTracker::TotalXp() const
{
    return _Progression.TotalXp();
}

int32 UProgressionTracker::SerializeState() const
{
    return TotalXp();
}

void UProgressionTracker::Restore(int32 InTotalXp)
{
    _Progression.Reset();
    _Progression.AddXp(FMath::Max(InTotalXp, 0));
}

void UProgressionTracker::RestoreGarbage()
{
    // Godot SaveData.number_or("junk", 0) -> 0, so a garbage restore is a clean reset.
    _Progression.Reset();
    _Progression.AddXp(0);
}
