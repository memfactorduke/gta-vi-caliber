// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrainMover.h"

void FTrainMover::Configure(const FParams& InParams, const TArray<double>& InStations)
{
    Params = InParams;
    Stations = InStations;
    Pos = 0.0;
    Vel = 0.0;
    Target = 0;
    bDwelling = false;
    DwellTimer = 0.0;
}

double FTrainMover::ForwardDistanceTo(double TargetPos) const
{
    const double Len = FMath::Max(KINDA_SMALL_NUMBER, Params.TrackLength);
    double D = FMath::Fmod(TargetPos - Pos, Len);
    if (D < 0.0)
    {
        D += Len;
    }
    return D;
}

double FTrainMover::DistanceToNextStop() const
{
    if (Stations.Num() == 0)
    {
        return 0.0;
    }
    return ForwardDistanceTo(Stations[Target]);
}

void FTrainMover::Advance(double Dt)
{
    const double Step = FMath::Max(0.0, Dt);
    if (Step <= 0.0)
    {
        return;
    }

    const double Len = FMath::Max(KINDA_SMALL_NUMBER, Params.TrackLength);

    // Dwelling at a platform: count down, then pull away to the next station.
    if (bDwelling)
    {
        DwellTimer -= Step;
        if (DwellTimer <= 0.0)
        {
            bDwelling = false;
            if (Stations.Num() > 0)
            {
                Target = (Target + 1) % Stations.Num();
            }
        }
        return;
    }

    // No stations: just cruise the loop at line speed.
    if (Stations.Num() == 0)
    {
        Vel = FMath::Min(FMath::Max(0.0, Params.LineSpeed), Vel + FMath::Max(0.0, Params.Accel) * Step);
        Pos = FMath::Fmod(Pos + Vel * Step, Len);
        return;
    }

    const double DistToStop = ForwardDistanceTo(Stations[Target]);

    // Speed cap that still allows braking to a stop at the platform, plus the line speed.
    const double VCap = FMath::Sqrt(2.0 * FMath::Max(0.0, Params.Brake) * DistToStop);
    const double TargetSpeed = FMath::Min(FMath::Max(0.0, Params.LineSpeed), VCap);

    if (Vel < TargetSpeed)
    {
        Vel = FMath::Min(TargetSpeed, Vel + FMath::Max(0.0, Params.Accel) * Step);
    }
    else
    {
        Vel = FMath::Max(TargetSpeed, Vel - FMath::Max(0.0, Params.Brake) * Step);
    }
    Vel = FMath::Max(0.0, Vel);

    // Arrive: if this step reaches the platform, snap to it and begin the dwell.
    const double MoveStep = Vel * Step;
    if (MoveStep >= DistToStop)
    {
        Pos = Stations[Target];
        Vel = 0.0;
        bDwelling = true;
        DwellTimer = FMath::Max(0.0, Params.DwellSeconds);
        return;
    }

    Pos = FMath::Fmod(Pos + MoveStep, Len);
}
