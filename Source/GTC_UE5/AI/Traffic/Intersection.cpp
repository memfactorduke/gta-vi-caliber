// Copyright Epic Games, Inc. All Rights Reserved.

#include "Intersection.h"

#include "Math/UnrealMathUtility.h"

bool FIntersection::Yields(const FApproach& A, const FApproach& B)
{
    // A yields to B iff B strictly outranks A under the total order
    // (Priority desc, ArrivalTime asc, ApproachId asc).
    if (A.Priority != B.Priority)
    {
        return B.Priority > A.Priority;
    }
    if (A.ArrivalTime != B.ArrivalTime)
    {
        return B.ArrivalTime < A.ArrivalTime;
    }
    return B.ApproachId < A.ApproachId;
}

int32 FIntersection::RightOfWay(const TArray<FApproach>& Claims)
{
    int32 Best = -1;
    for (int32 I = 0; I < Claims.Num(); ++I)
    {
        // The winner is the claim that yields to no one: if Best yields to I, I is better.
        if (Best == -1 || Yields(Claims[Best], Claims[I]))
        {
            Best = I;
        }
    }
    return Best;
}

bool FIntersection::MayProceed(const TArray<FApproach>& Claims, int32 SelfIndex)
{
    return SelfIndex >= 0 && SelfIndex < Claims.Num() && RightOfWay(Claims) == SelfIndex;
}

double FIntersection::StopLineGap(double DistanceToStopLine, double VehicleHalfLength)
{
    return FMath::Max(0.0, DistanceToStopLine - VehicleHalfLength);
}
