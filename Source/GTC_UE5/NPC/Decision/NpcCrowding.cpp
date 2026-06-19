// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcCrowding.h"

ENpcBusyness FNpcCrowding::Classify(int32 Occupancy, int32 Capacity)
{
	if (Occupancy <= 0)
	{
		return ENpcBusyness::Empty;
	}
	if (Capacity <= 0)
	{
		// Uncapped (open street / synthesized): people are around, but it can't be
		// "packed" — there's always room.
		return ENpcBusyness::Quiet;
	}
	if (Occupancy >= Capacity)
	{
		return ENpcBusyness::Packed;
	}
	// Half-full or more reads as busy; below that, comfortable.
	return (Occupancy * 2 >= Capacity) ? ENpcBusyness::Busy : ENpcBusyness::Quiet;
}

bool FNpcCrowding::ShouldAvoid(ENpcBusyness Busyness, double Tolerance)
{
	const double T = FMath::Clamp(Tolerance, 0.0, 1.0);
	switch (Busyness)
	{
	case ENpcBusyness::Packed:
		// Most crowd-averse people steer clear of a packed place; the crowd-loving
		// (high tolerance) dive in.
		return T < 0.6;
	case ENpcBusyness::Busy:
		// Only the genuinely crowd-averse avoid a merely-busy place.
		return T < 0.25;
	case ENpcBusyness::Empty:
	case ENpcBusyness::Quiet:
	default:
		return false;
	}
}

bool FNpcCrowding::ShouldQueue(ENpcBusyness Busyness, bool bAvoiding)
{
	return Busyness == ENpcBusyness::Packed && !bAvoiding;
}

float FNpcCrowding::StandoffDistance(ENpcBusyness Busyness, double Tolerance)
{
	const float T = static_cast<float>(FMath::Clamp(Tolerance, 0.0, 1.0));
	switch (Busyness)
	{
	case ENpcBusyness::Packed:
		// Hang back ~3-6 m: the crowd-loving edge closer, the averse keep their
		// distance. Always > 0, so a packed spot never has bodies on one point.
		return 300.0f + 300.0f * (1.0f - T);
	case ENpcBusyness::Busy:
		// A busy (not full) spot only pushes the crowd-averse back, up to ~2 m.
		return 200.0f * (1.0f - T);
	case ENpcBusyness::Empty:
	case ENpcBusyness::Quiet:
	default:
		return 0.0f; // comfortable — walk right up.
	}
}

const TCHAR* FNpcCrowding::Name(ENpcBusyness Busyness)
{
	switch (Busyness)
	{
	case ENpcBusyness::Empty:  return TEXT("empty");
	case ENpcBusyness::Quiet:  return TEXT("quiet");
	case ENpcBusyness::Busy:   return TEXT("busy");
	case ENpcBusyness::Packed: return TEXT("packed");
	default:                   return TEXT("quiet");
	}
}
