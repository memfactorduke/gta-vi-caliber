// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleSpeedometer.h"

#include "Math/UnrealMathUtility.h"

double FVehicleSpeedometer::SpeedKmPerHour(double SpeedUnitsPerSec)
{
    return SpeedUnitsPerSec * CmPerSecToKmPerHour;
}

double FVehicleSpeedometer::SpeedMilesPerHour(double SpeedUnitsPerSec)
{
    return SpeedKmPerHour(SpeedUnitsPerSec) * KmPerHourToMilesPerHour;
}

double FVehicleSpeedometer::NeedleFraction(double SpeedUnitsPerSec, double MaxSpeedUnitsPerSec)
{
    // Magnitude so reversing still sweeps the needle; guard a zero/negative max.
    if (MaxSpeedUnitsPerSec <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(FMath::Abs(SpeedUnitsPerSec) / MaxSpeedUnitsPerSec, 0.0, 1.0);
}

double FVehicleSpeedometer::RpmFraction(double Rpm, double RedlineRpm)
{
    if (RedlineRpm <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(Rpm / RedlineRpm, 0.0, 1.0);
}

bool FVehicleSpeedometer::IsRedlineFlashing(double Rpm, double RedlineRpm)
{
    return RedlineRpm > 0.0 && Rpm >= RedlineRpm;
}

FString FVehicleSpeedometer::GearLabel(EGear Gear, int32 ForwardGearNumber)
{
    switch (Gear)
    {
    case EGear::Park:
        return TEXT("P");
    case EGear::Reverse:
        return TEXT("R");
    case EGear::Neutral:
        return TEXT("N");
    case EGear::Drive:
    default:
        // A forward gear: present its 1..N number; a not-yet-engaged 0 still reads as "1".
        return FString::FromInt(FMath::Max(1, ForwardGearNumber));
    }
}
