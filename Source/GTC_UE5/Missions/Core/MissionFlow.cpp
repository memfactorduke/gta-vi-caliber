// Copyright Epic Games, Inc. All Rights Reserved.

#include "MissionFlow.h"

int32 MissionFlow::CurrentIndex(const TArray<FFlowObjective>& Objectives)
{
    for (int32 i = 0; i < Objectives.Num(); ++i)
    {
        if (!Objectives[i].bDone)
        {
            return i;
        }
    }
    return NoIndex;
}

MissionFlow::FFlowObjective MissionFlow::Current(const TArray<FFlowObjective>& Objectives)
{
    const int32 i = CurrentIndex(Objectives);
    return i != NoIndex ? Objectives[i] : FFlowObjective();
}

FString MissionFlow::CurrentText(const TArray<FFlowObjective>& Objectives)
{
    const FFlowObjective Objective = Current(Objectives);
    return Objective.IsEmpty() ? FString() : Objective.Text;
}

int32 MissionFlow::DoneCount(const TArray<FFlowObjective>& Objectives)
{
    int32 N = 0;
    for (const FFlowObjective& Objective : Objectives)
    {
        if (Objective.bDone)
        {
            ++N;
        }
    }
    return N;
}

FString MissionFlow::HudLine(const FString& Title, const TArray<FFlowObjective>& Objectives)
{
    const int32 Total = Objectives.Num();
    const int32 Done = DoneCount(Objectives);
    const FString Text = CurrentText(Objectives);
    if (Text.IsEmpty())
    {
        return FString::Printf(TEXT("%s — complete (%d/%d)"), *Title, Done, Total);
    }
    return FString::Printf(TEXT("%s — %s (%d/%d)"), *Title, *Text, Done + 1, Total);
}

bool MissionFlow::TimedOut(double TimeLimit, double TimeLeft)
{
    return TimeLimit > 0.0 && TimeLeft <= 0.0;
}

bool MissionFlow::ShouldFail(bool bPlayerDead, double TimeLimit, double TimeLeft)
{
    return bPlayerDead || TimedOut(TimeLimit, TimeLeft);
}

FVector MissionFlow::CurrentWaypoint(
    const TArray<FFlowObjective>& Objectives,
    const TMap<FString, FVector>& Waypoints,
    const FVector& Fallback)
{
    const FFlowObjective Objective = Current(Objectives);
    if (Objective.IsEmpty())
    {
        return Fallback;
    }
    const FVector* Found = Waypoints.Find(Objective.Id);
    return Found ? *Found : Fallback;
}
