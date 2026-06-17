// Copyright (c) 2026 GTC contributors

#include "MissionController.h"

void UMissionController::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMissionController::Deinitialize()
{
    Super::Deinitialize();
}

void UMissionController::Build()
{
    // Godot _build: _mission = MissionObjectives.new(title, objective_defs).
    TArray<FMissionObjectives::FObjectiveDef> Defs;
    Defs.Reserve(ObjectiveDefs.Num());
    for (const FObjectiveDef& Def : ObjectiveDefs)
    {
        Defs.Add({ Def.Id, Def.Text });
    }
    Mission = FMissionObjectives(Title, Defs);
    TimeLeft = TimeLimit;
    bEnded = false;
    bBuilt = true;
}

void UMissionController::Begin()
{
    // Godot begin(): build if needed, then _mission.start().
    if (!bBuilt)
    {
        Build();
    }
    Mission.Start();
}

void UMissionController::Complete(const FString& Id)
{
    // Godot complete(id): no-op unless active; otherwise delegate to the owned
    // model, which marks the objective and auto-completes the mission internally.
    if (!Mission.IsActive())
    {
        return;
    }
    if (Mission.CompleteObjective(Id))
    {
        OnObjectiveCompleted.Broadcast(Id);
        if (Mission.IsComplete())
        {
            Finish(/*bCompleted*/ true);
        }
    }
}

void UMissionController::Fail()
{
    // Godot fail(): if active, _mission.fail() then _finish(false).
    if (Mission.IsActive())
    {
        Mission.Fail();
        Finish(/*bCompleted*/ false);
    }
}

void UMissionController::Reset()
{
    Build();
    Begin();
}

FString UMissionController::HudText() const
{
    return bBuilt ? MissionFlow::HudLine(Title, Mission.Objectives) : FString();
}

FString UMissionController::CurrentObjectiveId() const
{
    if (!bBuilt)
    {
        return FString();
    }
    return MissionFlow::Current(Mission.Objectives).Id;
}

FVector UMissionController::CurrentWaypoint(const FVector& Fallback) const
{
    if (!bBuilt)
    {
        return Fallback;
    }
    // Godot converts authored coords through _local_waypoints() (FloatingOrigin
    // offset). That origin-shift is Wave 3 wiring; here waypoints are authored.
    return MissionFlow::CurrentWaypoint(Mission.Objectives, Waypoints, Fallback);
}

bool UMissionController::Tick(bool bPlayerDead, double Delta)
{
    // Godot _process: only ticks while active.
    if (!Mission.IsActive())
    {
        return false;
    }
    if (TimeLimit > 0.0)
    {
        TimeLeft = FMath::Max(TimeLeft - Delta, 0.0);
    }
    if (MissionFlow::ShouldFail(bPlayerDead, TimeLimit, TimeLeft))
    {
        Fail();
        return true;
    }
    return false;
}

void UMissionController::Finish(bool bCompleted)
{
    if (bEnded)
    {
        return;
    }
    bEnded = true;
    if (bCompleted)
    {
        OnMissionCompleted.Broadcast();
    }
    else
    {
        OnMissionFailed.Broadcast();
    }
}
