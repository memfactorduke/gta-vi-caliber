// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCVehicleLocomotionComponent.h"

#include "Math/UnrealMathUtility.h"

FRotator UGTCVehicleLocomotionComponent::DesiredAttitude() const
{
    // Default: face the heading, level. Subclasses add bank/pitch lean.
    return FRotator(0.0, FMath::RadiansToDegrees(HeadingRad()), 0.0);
}
