// Copyright Epic Games, Inc. All Rights Reserved.

#include "RaceProgressResolver.h"

FRaceProgressResolver::FRivalTick FRaceProgressResolver::RivalTick(double PrevProgress, double Elapsed,
    double PaceSeconds, double PlayerProgress, double RubberBandStrength, double RubberBandMax,
    int32 TotalCheckpoints)
{
    FRivalTick Out;
    Out.Progress = FRivalPacer::MonotonicProgressAt(
        PrevProgress, Elapsed, PaceSeconds, PlayerProgress, RubberBandStrength, RubberBandMax);
    Out.TargetCleared = FRivalPacer::ProgressToCheckpoint(Out.Progress, TotalCheckpoints);
    return Out;
}
