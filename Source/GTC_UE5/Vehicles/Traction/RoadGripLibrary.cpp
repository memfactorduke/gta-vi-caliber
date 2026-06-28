// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadGripLibrary.h"
#include "RoadGrip.h"

namespace
{
    // File-local helper (uniquely named to avoid unity-build collisions).
    FRoadGrip::FParams MakeRoadGripParams_RGL(float WetGripLoss, float AquaplaneSpeedKmh,
        float AquaplaneSpeedFullKmh, float AquaplaneFloor)
    {
        FRoadGrip::FParams P;
        P.WetGripLoss = static_cast<double>(WetGripLoss);
        P.AquaplaneSpeedKmh = static_cast<double>(AquaplaneSpeedKmh);
        P.AquaplaneSpeedFullKmh = static_cast<double>(AquaplaneSpeedFullKmh);
        P.AquaplaneFloor = static_cast<double>(AquaplaneFloor);
        return P;
    }
}

float URoadGripLibrary::RoadGripFactor(float Wetness, float SpeedKmh,
    float WetGripLoss, float AquaplaneSpeedKmh, float AquaplaneSpeedFullKmh, float AquaplaneFloor)
{
    const FRoadGrip::FParams P = MakeRoadGripParams_RGL(WetGripLoss, AquaplaneSpeedKmh, AquaplaneSpeedFullKmh, AquaplaneFloor);
    return static_cast<float>(FRoadGrip::GripFactor(Wetness, SpeedKmh, P));
}

float URoadGripLibrary::RoadAquaplaneAmount(float Wetness, float SpeedKmh,
    float WetGripLoss, float AquaplaneSpeedKmh, float AquaplaneSpeedFullKmh, float AquaplaneFloor)
{
    const FRoadGrip::FParams P = MakeRoadGripParams_RGL(WetGripLoss, AquaplaneSpeedKmh, AquaplaneSpeedFullKmh, AquaplaneFloor);
    return static_cast<float>(FRoadGrip::AquaplaneAmount(Wetness, SpeedKmh, P));
}

bool URoadGripLibrary::RoadIsAquaplaning(float Wetness, float SpeedKmh,
    float WetGripLoss, float AquaplaneSpeedKmh, float AquaplaneSpeedFullKmh, float AquaplaneFloor)
{
    const FRoadGrip::FParams P = MakeRoadGripParams_RGL(WetGripLoss, AquaplaneSpeedKmh, AquaplaneSpeedFullKmh, AquaplaneFloor);
    return FRoadGrip::IsAquaplaning(Wetness, SpeedKmh, P);
}
