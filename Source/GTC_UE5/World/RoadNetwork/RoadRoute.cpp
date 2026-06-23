// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadRoute.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // Planar (XZ) coincidence test, matching the repo's Y-up convention.
    bool PlanarCoincident(const FVector& A, const FVector& B, double Tol)
    {
        const double Dx = A.X - B.X;
        const double Dz = A.Z - B.Z;
        return (Dx * Dx + Dz * Dz) <= Tol * Tol;
    }

    bool PathInRange(const TArray<FVector>& NodePositions, const TArray<int32>& NodePath)
    {
        if (NodePath.IsEmpty())
        {
            return false;
        }
        for (int32 I = 0; I < NodePath.Num(); ++I)
        {
            const int32 Idx = NodePath[I];
            if (Idx < 0 || Idx >= NodePositions.Num())
            {
                return false;
            }
        }
        return true;
    }
}

TArray<FVector> FRoadRoute::Centerline(const TArray<FVector>& NodePositions, const TArray<int32>& NodePath)
{
    TArray<FVector> Out;
    if (!PathInRange(NodePositions, NodePath))
    {
        return Out;
    }
    Out.Reserve(NodePath.Num());
    for (int32 I = 0; I < NodePath.Num(); ++I)
    {
        Out.Add(NodePositions[NodePath[I]]);
    }
    return Out;
}

TArray<FVector> FRoadRoute::CenterlineThrough(
    const TArray<FVector>& NodePositions, const TArray<int32>& NodePath,
    const FVector& Start, const FVector& End)
{
    TArray<FVector> Out = Centerline(NodePositions, NodePath);
    if (Out.IsEmpty())
    {
        return Out;
    }

    TArray<FVector> Threaded;
    Threaded.Reserve(Out.Num() + 2);
    if (!PlanarCoincident(Start, Out[0], Eps))
    {
        Threaded.Add(Start);
    }
    for (int32 I = 0; I < Out.Num(); ++I)
    {
        Threaded.Add(Out[I]);
    }
    if (!PlanarCoincident(End, Threaded.Last(), Eps))
    {
        Threaded.Add(End);
    }
    return Threaded;
}
