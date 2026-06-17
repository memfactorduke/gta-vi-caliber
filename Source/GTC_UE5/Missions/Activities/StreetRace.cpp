// Copyright (c) 2026 GTC contributors

#include "StreetRace.h"

StreetRace::StreetRace(const TArray<FVector>& InCheckpoints, int32 InLaps)
    : Checkpoints(InCheckpoints)
    , Laps(FMath::Max(InLaps, 1))
{
    Reset();
}

FVector StreetRace::Ground(const FVector& V)
{
    return FVector(V.X, 0.0, V.Z);
}

bool StreetRace::Reached(const FVector& Pos, double Radius)
{
    if (bFinished || Checkpoints.Num() == 0)
    {
        return false;
    }
    const FVector Target = Checkpoints[Index];
    if (Ground(Target - Pos).Length() > FMath::Max(Radius, 0.0))
    {
        return false;
    }
    Advance();
    return true;
}

FVector StreetRace::CurrentCheckpoint() const
{
    if (Checkpoints.Num() == 0 || bFinished)
    {
        return FVector::ZeroVector;
    }
    return Checkpoints[Index];
}

int32 StreetRace::CheckpointIndex() const
{
    return Index;
}

int32 StreetRace::CurrentLap() const
{
    if (bFinished)
    {
        return Laps;
    }
    return Lap + 1;
}

int32 StreetRace::TotalLaps() const
{
    return Laps;
}

bool StreetRace::IsFinished() const
{
    return bFinished;
}

double StreetRace::Progress() const
{
    if (bFinished)
    {
        return 1.0;
    }
    const int32 PerLap = Checkpoints.Num();
    if (PerLap == 0)
    {
        return 1.0;
    }
    const int32 Total = PerLap * Laps;
    const int32 Done = Lap * PerLap + Index;
    return FMath::Clamp(static_cast<double>(Done) / static_cast<double>(Total), 0.0, 1.0);
}

int32 StreetRace::CheckpointsRemaining() const
{
    if (bFinished || Checkpoints.Num() == 0)
    {
        return 0;
    }
    const int32 PerLap = Checkpoints.Num();
    const int32 Total = PerLap * Laps;
    const int32 Done = Lap * PerLap + Index;
    return FMath::Max(Total - Done, 0);
}

void StreetRace::Tick(double Delta)
{
    if (bFinished || Delta <= 0.0)
    {
        return;
    }
    ElapsedTime += Delta;
}

double StreetRace::Elapsed() const
{
    return ElapsedTime;
}

double StreetRace::LastLap() const
{
    if (LapSplitTimes.Num() == 0)
    {
        return 0.0;
    }
    return LapSplitTimes[LapSplitTimes.Num() - 1];
}

double StreetRace::BestLap() const
{
    if (LapSplitTimes.Num() == 0)
    {
        return 0.0;
    }
    double Best = LapSplitTimes[0];
    for (int32 i = 1; i < LapSplitTimes.Num(); ++i)
    {
        if (LapSplitTimes[i] < Best)
        {
            Best = LapSplitTimes[i];
        }
    }
    return Best;
}

TArray<double> StreetRace::LapSplits() const
{
    return LapSplitTimes;
}

void StreetRace::Reset()
{
    Index = 0;
    Lap = 0;
    ElapsedTime = 0.0;
    LapStart = 0.0;
    LapSplitTimes.Reset();
    bFinished = Checkpoints.Num() == 0;
}

int32 StreetRace::Placement(double PlayerProgress, const TArray<double>& RivalProgresses)
{
    int32 Place = 1;
    for (double R : RivalProgresses)
    {
        if (R > PlayerProgress + Epsilon)
        {
            Place += 1;
        }
    }
    return Place;
}

double StreetRace::GapTo(double AheadProgress, double MyProgress, double TrackLength)
{
    if (TrackLength <= 0.0)
    {
        return 0.0;
    }
    return FMath::Max((AheadProgress - MyProgress) * TrackLength, 0.0);
}

int32 StreetRace::Reward(int32 Placement, int32 BaseReward)
{
    if (Placement <= 0 || BaseReward <= 0)
    {
        return 0;
    }
    const double Factor = FMath::Max(1.0 - 0.25 * static_cast<double>(Placement - 1), 0.25);
    return FMath::RoundToInt(static_cast<double>(BaseReward) * Factor);
}

// --- helpers ---------------------------------------------------------------

void StreetRace::Advance()
{
    Index += 1;
    if (Index < Checkpoints.Num())
    {
        return;
    }
    // Crossed the last checkpoint of this lap.
    Index = 0;
    LapSplitTimes.Add(ElapsedTime - LapStart);
    LapStart = ElapsedTime;
    Lap += 1;
    if (Lap >= Laps)
    {
        bFinished = true;
    }
}
