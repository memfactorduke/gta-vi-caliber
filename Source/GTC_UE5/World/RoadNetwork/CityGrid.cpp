// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityGrid.h"

#include "Math/UnrealMathUtility.h"

FVector FCityGrid::Intersection(const FSpec& Spec, int32 i, int32 j)
{
    const double BlocksX = FMath::Max(1, Spec.BlocksX);
    const double BlocksZ = FMath::Max(1, Spec.BlocksZ);
    // Centre the lattice on Origin: index BlocksX/2 lands on Origin.X, and so on.
    const double X = Spec.Origin.X + (static_cast<double>(i) - BlocksX * 0.5) * Spec.BlockSize;
    const double Z = Spec.Origin.Z + (static_cast<double>(j) - BlocksZ * 0.5) * Spec.BlockSize;
    return FVector(X, Spec.Origin.Y, Z);
}

TArray<TArray<FVector>> FCityGrid::Polylines(const FSpec& Spec)
{
    const int32 Nx = CountX(Spec);
    const int32 Nz = CountZ(Spec);

    TArray<TArray<FVector>> Out;
    Out.Reserve(Nx + Nz);

    // Rows: streets running along X, one per intersection row.
    for (int32 j = 0; j < Nz; ++j)
    {
        TArray<FVector> Row;
        Row.Reserve(Nx);
        for (int32 i = 0; i < Nx; ++i)
        {
            Row.Add(Intersection(Spec, i, j));
        }
        Out.Add(MoveTemp(Row));
    }

    // Columns: streets running along Z, one per intersection column.
    for (int32 i = 0; i < Nx; ++i)
    {
        TArray<FVector> Col;
        Col.Reserve(Nz);
        for (int32 j = 0; j < Nz; ++j)
        {
            Col.Add(Intersection(Spec, i, j));
        }
        Out.Add(MoveTemp(Col));
    }

    return Out;
}

FVector FCityGrid::SnapOriginTo(const FSpec& Spec, const FVector& WorldXZ)
{
    const double Block = FMath::Max(KINDA_SMALL_NUMBER, Spec.BlockSize);
    const double SnappedX =
        Spec.Origin.X + FMath::RoundToDouble((WorldXZ.X - Spec.Origin.X) / Block) * Block;
    const double SnappedZ =
        Spec.Origin.Z + FMath::RoundToDouble((WorldXZ.Z - Spec.Origin.Z) / Block) * Block;
    return FVector(SnappedX, Spec.Origin.Y, SnappedZ);
}
