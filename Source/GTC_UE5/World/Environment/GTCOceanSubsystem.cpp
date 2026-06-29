// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCOceanSubsystem.h"

#include "Engine/World.h"

void UGTCOceanSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // A calm default sea so a level with no authored waves still floats boats sensibly.
    // The water-material adapter should overwrite this with the rendered set via SetWaves.
    Waves.Reset();
    {
        FGerstnerWave A;
        A.DirX = 1.0; A.DirY = 0.2; A.Amplitude = 0.35; A.Wavelength = 14.0; A.Steepness = 0.5;
        Waves.Add(A);
        FGerstnerWave B;
        B.DirX = 0.3; B.DirY = 1.0; B.Amplitude = 0.18; B.Wavelength = 7.0; B.Steepness = 0.4;
        Waves.Add(B);
    }
}

double UGTCOceanSubsystem::NowSeconds() const
{
    const UWorld* World = GetWorld();
    return World != nullptr ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

double UGTCOceanSubsystem::HeightAtCm(double Xcm, double Ycm) const
{
    const double HeightM = FOceanSurface::HeightAt(
        Waves.GetData(), Waves.Num(), Xcm / UuPerM, Ycm / UuPerM, NowSeconds());
    return HeightM * UuPerM + SeaLevelZ;
}

FOceanSample UGTCOceanSubsystem::SampleCm(double Xcm, double Ycm) const
{
    FOceanSample S = FOceanSurface::Sample(
        Waves.GetData(), Waves.Num(), Xcm / UuPerM, Ycm / UuPerM, NowSeconds());
    // Convert the height + horizontal displacement back to cm and lift to sea level.
    S.Height = S.Height * UuPerM + SeaLevelZ;
    S.DispX *= UuPerM;
    S.DispY *= UuPerM;
    return S;
}
