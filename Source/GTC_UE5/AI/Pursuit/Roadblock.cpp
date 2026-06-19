// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roadblock.h"

#include "Math/UnrealMathUtility.h"

double FRoadblock::LeadDistance(double PlayerSpeed, double SetupLeadSeconds)
{
    const double Speed = FMath::Max(0.0, PlayerSpeed);
    const double Lead = FMath::Max(0.0, SetupLeadSeconds);
    return FMath::Max(MinLeadDistance, Speed * Lead);
}

FVector FRoadblock::BlockadeCenter(const FVector& PlayerPos, const FVector& FleeDir, double Lead)
{
    const FVector Dir = FleeDir.GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        return PlayerPos; // no heading — nothing to place ahead
    }
    return PlayerPos + Dir * FMath::Max(0.0, Lead);
}

int32 FRoadblock::UnitsToSpan(double RoadWidth)
{
    if (RoadWidth <= 0.0)
    {
        return 1;
    }
    return FMath::Max(1, FMath::CeilToInt(RoadWidth / UnitWidth));
}

bool FRoadblock::FullyBlocks(double RoadWidth, int32 UnitCount)
{
    return UnitCount >= UnitsToSpan(RoadWidth);
}

TArray<FVector> FRoadblock::UnitPositions(const FVector& Center, const FVector& RoadRight,
    double RoadWidth, int32 UnitCount)
{
    TArray<FVector> Positions;
    if (UnitCount <= 0)
    {
        return Positions;
    }

    const FVector Right = RoadRight.GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        return Positions;
    }

    const double Width = FMath::Max(0.0, RoadWidth);
    if (UnitCount == 1)
    {
        Positions.Add(Center); // a lone cruiser blocks the middle
        return Positions;
    }

    // Spread evenly edge to edge: unit i at -W/2 + i*(W/(n-1)).
    const double Step = Width / static_cast<double>(UnitCount - 1);
    for (int32 i = 0; i < UnitCount; ++i)
    {
        const double Offset = -0.5 * Width + i * Step;
        Positions.Add(Center + Right * Offset);
    }
    return Positions;
}

bool FRoadblock::HasPassed(const FVector& PlayerPos, const FVector& Center, const FVector& FleeDir)
{
    const FVector Dir = FleeDir.GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        return false;
    }
    return FVector::DotProduct(PlayerPos - Center, Dir) > 0.0;
}
