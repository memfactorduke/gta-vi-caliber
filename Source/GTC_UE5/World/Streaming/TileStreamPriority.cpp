// Copyright Epic Games, Inc. All Rights Reserved.

#include "TileStreamPriority.h"

FVector2D FTileStreamPriority::LookAheadPoint(const FVector2D& CameraPos, const FVector2D& Velocity, double LookAheadSeconds)
{
	if (LookAheadSeconds <= 0.0)
	{
		return CameraPos;
	}
	return CameraPos + Velocity * LookAheadSeconds;
}

double FTileStreamPriority::EffectiveDistance(
	const FVector2D& CameraPos,
	const FVector2D& Velocity,
	const FVector2D& TileCenter,
	double LookAheadSeconds)
{
	return FVector2D::Distance(TileCenter, LookAheadPoint(CameraPos, Velocity, LookAheadSeconds));
}

double FTileStreamPriority::AlongTrack(const FVector2D& CameraPos, const FVector2D& Velocity, const FVector2D& TileCenter)
{
	const double Speed = Velocity.Size();
	if (Speed <= UE_DOUBLE_SMALL_NUMBER)
	{
		return 0.0;
	}
	const FVector2D Heading = Velocity / Speed;
	return FVector2D::DotProduct(TileCenter - CameraPos, Heading);
}

bool FTileStreamPriority::IsMoreUrgent(
	const FVector2D& CameraPos,
	const FVector2D& Velocity,
	const FVector2D& TileCenterA,
	const FVector2D& TileCenterB,
	double LookAheadSeconds)
{
	return EffectiveDistance(CameraPos, Velocity, TileCenterA, LookAheadSeconds)
		<= EffectiveDistance(CameraPos, Velocity, TileCenterB, LookAheadSeconds);
}
