// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleChaseCamera.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // Lerp Base -> Max as Speed climbs 0 -> SpeedAtMax, holding Max beyond it.
    // A non-positive SpeedAtMax disables the ramp (returns Base).
    float SpeedRamp(float Base, float Max, float Speed, float SpeedAtMax)
    {
        if (SpeedAtMax <= 0.0f)
        {
            return Base;
        }
        const float T = FMath::Clamp(FMath::Max(0.0f, Speed) / SpeedAtMax, 0.0f, 1.0f);
        return FMath::Lerp(Base, Max, T);
    }
}

float FVehicleChaseCamera::SpeedFov(float BaseFov, float MaxFov, float Speed, float SpeedAtMaxFov)
{
    return SpeedRamp(BaseFov, MaxFov, Speed, SpeedAtMaxFov);
}

float FVehicleChaseCamera::FollowDistance(float BaseDistance, float MaxDistance, float Speed, float SpeedAtMaxDistance)
{
    return SpeedRamp(BaseDistance, MaxDistance, Speed, SpeedAtMaxDistance);
}

float FVehicleChaseCamera::LookBehindYawOffset(bool bLookBehind)
{
    return bLookBehind ? PI : 0.0f;
}
