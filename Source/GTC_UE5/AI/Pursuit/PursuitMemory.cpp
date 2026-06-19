// Copyright Epic Games, Inc. All Rights Reserved.

#include "PursuitMemory.h"

#include "Math/UnrealMathUtility.h"

FVector FPursuitMemory::Target(bool bSeen, const FVector& TargetPos, const FVector& LastKnown)
{
    return bSeen ? TargetPos : LastKnown;
}

bool FPursuitMemory::ShouldGiveUp(double TimeUnseen, double GiveUpTime)
{
    return GiveUpTime > 0.0 && TimeUnseen >= GiveUpTime;
}

EPursuitMemoryState FPursuitMemory::State(
    bool bSeen, double TimeUnseen, bool bReachedLastKnown, double GiveUpTime)
{
    if (bSeen)
    {
        return EPursuitMemoryState::Pursue;
    }
    if (ShouldGiveUp(TimeUnseen, GiveUpTime))
    {
        return EPursuitMemoryState::Lost;
    }
    return bReachedLastKnown ? EPursuitMemoryState::Search : EPursuitMemoryState::Pursue;
}

FVector FPursuitMemory::SearchPoint(
    const FVector& LastKnown, double URadius, double UAngle, double Radius)
{
    const double R = FMath::Sqrt(FMath::Clamp(URadius, 0.0, 1.0)) * Radius;
    const double A = UAngle * UE_DOUBLE_TWO_PI;
    return LastKnown + FVector(FMath::Cos(A) * R, 0.0, FMath::Sin(A) * R);
}
