// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdBudget.h"

#include "Math/UnrealMathUtility.h"

int32 FCrowdBudget::SpawnCount(int32 LiveCount, int32 TargetPopulation, int32 MaxSpawnsPerPass)
{
    const int32 Deficit = TargetPopulation - LiveCount;
    const int32 Cap = FMath::Max(0, MaxSpawnsPerPass);
    return FMath::Clamp(Deficit, 0, Cap);
}

bool FCrowdBudget::ShouldDespawn(double DistanceFromCamera, double DespawnRadius)
{
    return DistanceFromCamera > DespawnRadius;
}

bool FCrowdBudget::InSpawnRing(double DistanceFromCamera, double RingMin, double RingMax)
{
    return DistanceFromCamera >= RingMin && DistanceFromCamera <= RingMax;
}

double FCrowdBudget::TickInterval(double DistanceFromCamera, double NearRadius, double FarInterval)
{
    return DistanceFromCamera <= NearRadius ? 0.0 : FarInterval;
}

int32 FCrowdBudget::FarthestBeyond(const TArray<double>& Distances, double DespawnRadius)
{
    int32 Best = -1;
    double BestDist = DespawnRadius; // must strictly exceed the radius to count
    for (int32 I = 0; I < Distances.Num(); ++I)
    {
        if (Distances[I] > BestDist)
        {
            BestDist = Distances[I];
            Best = I;
        }
    }
    return Best;
}
