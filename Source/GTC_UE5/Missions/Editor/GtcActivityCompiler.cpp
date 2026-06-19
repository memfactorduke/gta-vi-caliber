// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcActivityCompiler.h"

#include "GtcMissionDefinition.h"

SideJob::FJob GtcActivityCompiler::CompileSideJob(const FGtcSideJobDefinition& D)
{
    return SideJob::MakeJob(D.Kind, D.Pickup, D.Dropoff, D.BaseReward);
}

int32 GtcActivityCompiler::SideJobPayout(const FGtcSideJobDefinition& D, double Distance, double TimeTaken)
{
    const SideJob::FJob Job = SideJob::MakeJob(D.Kind, D.Pickup, D.Dropoff, D.BaseReward);
    return SideJob::Payout(Job, Distance, TimeTaken, D.ParTime);
}

StreetRace GtcActivityCompiler::CompileRace(const FGtcStreetRaceDefinition& D)
{
    return StreetRace(D.Checkpoints, D.Laps);
}

int32 GtcActivityCompiler::RaceReward(const FGtcStreetRaceDefinition& D, int32 Placement)
{
    return StreetRace::Reward(Placement, D.BaseReward);
}
