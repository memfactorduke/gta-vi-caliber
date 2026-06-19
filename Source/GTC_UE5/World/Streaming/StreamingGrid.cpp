// Copyright Epic Games, Inc. All Rights Reserved.

#include "StreamingGrid.h"

FIntPoint FStreamingGrid::WorldToTile(const FVector2D& WorldXY, double TileSize)
{
	if (TileSize <= 0.0)
	{
		return FIntPoint::ZeroValue;
	}
	// FloorToInt (not truncation) so negative coordinates land in the correct
	// tile: -1 unit at size 128 is tile -1, not tile 0.
	return FIntPoint(
		FMath::FloorToInt(WorldXY.X / TileSize),
		FMath::FloorToInt(WorldXY.Y / TileSize));
}

FVector2D FStreamingGrid::TileMin(const FIntPoint& Tile, double TileSize)
{
	return FVector2D(static_cast<double>(Tile.X) * TileSize, static_cast<double>(Tile.Y) * TileSize);
}

FVector2D FStreamingGrid::TileCenter(const FIntPoint& Tile, double TileSize)
{
	return FVector2D(
		(static_cast<double>(Tile.X) + 0.5) * TileSize,
		(static_cast<double>(Tile.Y) + 0.5) * TileSize);
}

int32 FStreamingGrid::TileRing(const FIntPoint& A, const FIntPoint& B)
{
	return FMath::Max(FMath::Abs(A.X - B.X), FMath::Abs(A.Y - B.Y));
}

double FStreamingGrid::CenterDistance(const FIntPoint& Tile, const FVector2D& WorldXY, double TileSize)
{
	return FVector2D::Distance(TileCenter(Tile, TileSize), WorldXY);
}

TArray<FIntPoint> FStreamingGrid::TilesInWindow(const FIntPoint& CenterTile, int32 RingRadius)
{
	TArray<FIntPoint> Tiles;
	if (RingRadius < 0)
	{
		return Tiles;
	}

	const int32 Side = 2 * RingRadius + 1;
	Tiles.Reserve(Side * Side);
	for (int32 Y = CenterTile.Y - RingRadius; Y <= CenterTile.Y + RingRadius; ++Y)
	{
		for (int32 X = CenterTile.X - RingRadius; X <= CenterTile.X + RingRadius; ++X)
		{
			Tiles.Emplace(X, Y);
		}
	}
	return Tiles;
}
