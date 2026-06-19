// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcMissionCompiler.h"

#include "GtcMissionDefinition.h"

FGtcCompiledMission GtcMissionCompiler::CompileMission(const FGtcMissionDefinition& D)
{
    FGtcCompiledMission C;
    C.Id = D.Id;
    C.Title = D.Title;
    C.TimeLimit = D.TimeLimit;

    for (const FGtcObjectiveDefinition& O : D.Objectives)
    {
        FGtcCompiledObjective CO;
        CO.Id = O.Id;
        CO.Text = O.Text.ToString();
        CO.bHasWaypoint = O.bHasWaypoint;
        CO.Waypoint = O.Waypoint;
        C.Objectives.Add(MoveTemp(CO));
    }

    // Runnable only if the source parsed cleanly and there is something to complete.
    C.bValid = D.bValid && C.Objectives.Num() > 0;
    return C;
}

TArray<MissionChain::FMissionDef> GtcMissionCompiler::CompileCampaign(const TArray<FGtcMissionDefinition>& Defs)
{
    TArray<MissionChain::FMissionDef> Out;
    Out.Reserve(Defs.Num());

    for (const FGtcMissionDefinition& D : Defs)
    {
        MissionChain::FMissionDef M(D.Id, D.Title);
        for (const FGtcObjectiveDefinition& O : D.Objectives)
        {
            M.ObjectiveDefs.Add(O.Id);
            if (O.bHasWaypoint)
            {
                M.Waypoints.Add(O.Id, O.Waypoint);
            }
        }
        Out.Add(MoveTemp(M));
    }

    return Out;
}

TMap<FString, FMissionObjectiveDriver::FObjectiveDef> GtcMissionCompiler::CompileObjectiveDrivers(const FGtcMissionDefinition& D)
{
    TMap<FString, FMissionObjectiveDriver::FObjectiveDef> Out;
    for (const FGtcObjectiveDefinition& O : D.Objectives)
    {
        if (!O.bHasWaypoint)
        {
            continue;
        }
        if (O.Kind == EGtcObjectiveKind::Survive)
        {
            // Survive completes on a timer (subsystem tick), not from proximity.
            continue;
        }
        FMissionObjectiveDriver::FObjectiveDef Def;
        Def.Kind = (O.DriverKind == TEXT("hold")) ? TEXT("hold") : TEXT("reach");
        Def.Radius = O.Radius;
        Def.Duration = O.Duration;
        Out.Add(O.Id, Def);
    }
    return Out;
}
