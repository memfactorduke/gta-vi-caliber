// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcCommute.h"

namespace
{
    // Deterministic 0..999 from a seed (FNV-1a avalanche), no RNG object.
    int32 GtcCommuteRoll(int32 Seed)
    {
        uint32 H = 2166136261u ^ static_cast<uint32>(Seed);
        H *= 16777619u; H ^= H >> 15; H *= 2246822519u; H ^= H >> 13;
        return static_cast<int32>(H % 1000u);
    }
}

bool FNpcCommute::HasCar(int32 Seed)
{
    // ~1 in 3 of the population owns a car. Salted so it doesn't correlate with the
    // temper roll (which hashes the same seed differently).
    return GtcCommuteRoll(Seed ^ 0x5bd1e995) < 350;
}

bool FNpcCommute::ShouldDrive(bool bHasCar, const FString& DestinationPlace, double TripDistanceCm)
{
    if (!bHasCar || TripDistanceCm < DriveThresholdCm)
    {
        return false;
    }
    // Only commute anchors are worth driving to — you drive home or to the office,
    // not to the park bench across the street.
    return DestinationPlace == TEXT("home") || DestinationPlace == TEXT("office");
}

double FNpcCommute::StageDuration(ENpcDriveStage Stage)
{
    switch (Stage)
    {
    case ENpcDriveStage::Entering: return EnterSeconds;
    case ENpcDriveStage::Exiting:  return ExitSeconds;
    default:                       return 0.0;
    }
}

bool FNpcCommute::StageDwellElapsed(ENpcDriveStage Stage, double ElapsedSeconds)
{
    return ElapsedSeconds >= StageDuration(Stage);
}
