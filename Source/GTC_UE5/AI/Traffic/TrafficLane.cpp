// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrafficLane.h"

#include "Math/UnrealMathUtility.h"

int32 FTrafficLane::LeaderIndex(const TArray<FAgent>& Agents, int32 Self)
{
    if (Self < 0 || Self >= Agents.Num())
    {
        return -1;
    }
    const double SelfS = Agents[Self].S;
    int32 Best = -1;
    double BestDist = 0.0;
    for (int32 I = 0; I < Agents.Num(); ++I)
    {
        if (I == Self)
        {
            continue;
        }
        const double D = Agents[I].S - SelfS;
        if (D > 0.0 && (Best == -1 || D < BestDist))
        {
            Best = I;
            BestDist = D;
        }
    }
    return Best;
}

double FTrafficLane::BumperGap(const FAgent& Follower, const FAgent& Leader)
{
    return (Leader.S - Follower.S) - 0.5 * (Leader.Length + Follower.Length);
}

bool FTrafficLane::Overlaps(const FAgent& A, const FAgent& B)
{
    return FMath::Abs(A.S - B.S) < 0.5 * (A.Length + B.Length);
}
