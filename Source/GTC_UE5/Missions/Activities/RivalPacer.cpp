// Copyright Epic Games, Inc. All Rights Reserved.

#include "RivalPacer.h"

double FRivalPacer::BaseProgress(double Elapsed, double PaceSeconds)
{
    if (Elapsed <= 0.0)
    {
        return 0.0;
    }
    if (PaceSeconds <= 0.0)
    {
        // Degenerate "instant" rival: already at the finish, never a div-by-zero.
        return 1.0;
    }
    return FMath::Clamp(Elapsed / PaceSeconds, 0.0, 1.0);
}

double FRivalPacer::RubberBand(
    double BaseRivalProgress, double PlayerProgress, double Strength, double MaxAdjust)
{
    if (Strength <= 0.0 || MaxAdjust <= 0.0)
    {
        return 0.0;
    }
    const double Rival = FMath::Clamp(BaseRivalProgress, 0.0, 1.0);
    const double Player = FMath::Clamp(PlayerProgress, 0.0, 1.0);
    const double Nudge = Strength * (Player - Rival);
    return FMath::Clamp(Nudge, -MaxAdjust, MaxAdjust);
}

double FRivalPacer::ProgressAt(
    double Elapsed, double PaceSeconds, double PlayerProgress, double Strength, double MaxAdjust)
{
    const double Base = BaseProgress(Elapsed, PaceSeconds);
    const double Adjusted = Base + RubberBand(Base, PlayerProgress, Strength, MaxAdjust);
    return FMath::Clamp(Adjusted, 0.0, 1.0);
}

double FRivalPacer::MonotonicProgressAt(
    double PrevProgress,
    double Elapsed,
    double PaceSeconds,
    double PlayerProgress,
    double Strength,
    double MaxAdjust)
{
    const double Prev = FMath::Clamp(PrevProgress, 0.0, 1.0);
    const double Now = ProgressAt(Elapsed, PaceSeconds, PlayerProgress, Strength, MaxAdjust);
    return FMath::Max(Prev, Now);
}

int32 FRivalPacer::ProgressToCheckpoint(double Progress, int32 TotalCheckpoints)
{
    if (TotalCheckpoints <= 0)
    {
        return 0;
    }
    const double Clamped = FMath::Clamp(Progress, 0.0, 1.0);
    const int32 Cleared = FMath::FloorToInt(Clamped * static_cast<double>(TotalCheckpoints));
    return FMath::Clamp(Cleared, 0, TotalCheckpoints);
}
