// Copyright Epic Games, Inc. All Rights Reserved.

#include "MissionObjectives.h"

FMissionObjectives::FMissionObjectives(const FString& MissionTitle, const TArray<FObjectiveDef>& ObjectiveDefs)
    : Title(MissionTitle)
{
    // Godot _init: append {id, text, done=false} for each def.
    Objectives.Reserve(ObjectiveDefs.Num());
    for (const FObjectiveDef& Def : ObjectiveDefs)
    {
        Objectives.Add(MissionFlow::FFlowObjective(Def.Id, Def.Text, /*bDone*/ false));
    }
}

void FMissionObjectives::Start()
{
    if (State == EObjectiveSetState::Inactive)
    {
        State = EObjectiveSetState::Active;
    }
}

bool FMissionObjectives::CompleteObjective(const FString& Id)
{
    // Godot: if state != ACTIVE: return false.
    if (State != EObjectiveSetState::Active)
    {
        return false;
    }
    for (MissionFlow::FFlowObjective& Objective : Objectives)
    {
        if (Objective.Id == Id && !Objective.bDone)
        {
            Objective.bDone = true;
            if (AllDone())
            {
                State = EObjectiveSetState::Complete;
            }
            return true;
        }
    }
    return false;
}

void FMissionObjectives::Fail()
{
    if (State == EObjectiveSetState::Active)
    {
        State = EObjectiveSetState::Failed;
    }
}

FIntPoint FMissionObjectives::Progress() const
{
    // Godot progress(): Vector2i(done, total).
    int32 Done = 0;
    for (const MissionFlow::FFlowObjective& Objective : Objectives)
    {
        if (Objective.bDone)
        {
            ++Done;
        }
    }
    return FIntPoint(Done, Objectives.Num());
}

bool FMissionObjectives::AllDone() const
{
    // Godot _all_done(): false for an empty set.
    if (Objectives.Num() == 0)
    {
        return false;
    }
    for (const MissionFlow::FFlowObjective& Objective : Objectives)
    {
        if (!Objective.bDone)
        {
            return false;
        }
    }
    return true;
}
