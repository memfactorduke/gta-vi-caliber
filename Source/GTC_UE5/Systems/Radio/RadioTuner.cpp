// Copyright Epic Games, Inc. All Rights Reserved.

#include "RadioTuner.h"

void FRadioTuner::Configure(TArray<FStation> InStations)
{
    Stations = MoveTemp(InStations);
    SelectedStation = OffStation;
    ClockSeconds = 0.0;
}

void FRadioTuner::Advance(double Dt)
{
    // The clock never runs backwards: a paused/rewound frame must not desync the dial.
    if (Dt > 0.0)
    {
        ClockSeconds += Dt;
    }
}

void FRadioTuner::Next()
{
    // Walk a virtual ring of NumStations()+1 slots where the last slot is Radio Off.
    const int32 RingSize = Stations.Num() + 1;
    const int32 OffSlot = Stations.Num();
    const int32 Current = (SelectedStation == OffStation) ? OffSlot : SelectedStation;
    const int32 Stepped = (Current + 1) % RingSize;
    SelectedStation = (Stepped == OffSlot) ? OffStation : Stepped;
}

void FRadioTuner::Previous()
{
    const int32 RingSize = Stations.Num() + 1;
    const int32 OffSlot = Stations.Num();
    const int32 Current = (SelectedStation == OffStation) ? OffSlot : SelectedStation;
    const int32 Stepped = (Current - 1 + RingSize) % RingSize;
    SelectedStation = (Stepped == OffSlot) ? OffStation : Stepped;
}

void FRadioTuner::TuneTo(int32 StationIndex)
{
    // Any out-of-range index (including OffStation) resolves to Radio Off.
    SelectedStation = Stations.IsValidIndex(StationIndex) ? StationIndex : OffStation;
}

void FRadioTuner::TuneOff()
{
    SelectedStation = OffStation;
}

FRadioTuner::FNowPlaying FRadioTuner::NowPlaying() const
{
    FNowPlaying Result;
    if (SelectedStation == OffStation || !Stations.IsValidIndex(SelectedStation))
    {
        return Result;
    }

    const FStation& Tuned = Stations[SelectedStation];

    // Total airtime of one loop, counting only playable (positive-length) tracks.
    double TotalSeconds = 0.0;
    for (const double Length : Tuned.TrackSeconds)
    {
        if (Length > 0.0)
        {
            TotalSeconds += Length;
        }
    }

    // A station with nothing playable broadcasts dead air.
    if (TotalSeconds <= 0.0)
    {
        return Result;
    }

    // Where on the looping timeline this station sits right now. FMath::Fmod keeps the
    // sign of the dividend, so a negative phase could land negative — fold it positive.
    double Position = FMath::Fmod(ClockSeconds + Tuned.PhaseSeconds, TotalSeconds);
    if (Position < 0.0)
    {
        Position += TotalSeconds;
    }

    // Walk the playlist accumulating airtime until Position falls inside a track.
    double Accumulated = 0.0;
    for (int32 Index = 0; Index < Tuned.TrackSeconds.Num(); ++Index)
    {
        const double Length = Tuned.TrackSeconds[Index];
        if (Length <= 0.0)
        {
            continue; // skipped tracks contribute no airtime
        }

        if (Position < Accumulated + Length)
        {
            Result.TrackIndex = Index;
            Result.TrackOffset = Position - Accumulated;
            return Result;
        }
        Accumulated += Length;
    }

    // Floating-point drift could leave Position == TotalSeconds exactly; snap to the
    // start of the first playable track rather than reporting dead air.
    for (int32 Index = 0; Index < Tuned.TrackSeconds.Num(); ++Index)
    {
        if (Tuned.TrackSeconds[Index] > 0.0)
        {
            Result.TrackIndex = Index;
            Result.TrackOffset = 0.0;
            break;
        }
    }
    return Result;
}
