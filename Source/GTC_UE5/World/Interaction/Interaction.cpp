// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction.h"

#include <limits>

// Ported 1:1 from Interaction.nearest (game/scripts/world/interaction.gd).
// best_distance starts at true +INF so the first qualifying candidate always
// wins; the `Distance < BestDistance` strict-less keeps the FIRST of equal
// distances (ties resolve to the lower index). `Reach <= 0` selects nothing.
int32 FInteraction::Nearest(const TArray<FVector>& Points, const FVector& From, double Reach)
{
    if (Reach <= 0.0)
    {
        return None;
    }

    int32 Best = None;
    double BestDistance = std::numeric_limits<double>::infinity();
    for (int32 Index = 0; Index < Points.Num(); ++Index)
    {
        const double Distance = FVector::Dist(From, Points[Index]);
        if (Distance <= Reach && Distance < BestDistance)
        {
            Best = Index;
            BestDistance = Distance;
        }
    }
    return Best;
}
