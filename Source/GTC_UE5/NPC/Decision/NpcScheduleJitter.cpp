// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcScheduleJitter.h"

namespace
{
	// Deterministic hash -> [0,1). FNV-1a-ish with an avalanche finalizer, matching
	// the NPC layer's other hashed-but-stable choices (no RNG).
	double Hash01(int32 Seed)
	{
		uint32 H = 2166136261u;
		const uint32 S = static_cast<uint32>(Seed);
		for (int32 Byte = 0; Byte < 4; ++Byte)
		{
			H ^= (S >> (Byte * 8)) & 0xFFu;
			H *= 16777619u;
		}
		H ^= H >> 15;
		H *= 2246822519u;
		H ^= H >> 13;
		return static_cast<double>(H & 0xFFFFFFu) / static_cast<double>(0x1000000u);
	}
}

double FNpcScheduleJitter::HourOffset(int32 Seed, double MaxHours)
{
	if (MaxHours <= 0.0)
	{
		return 0.0;
	}
	// Map [0,1) -> [-MaxHours, +MaxHours).
	return (Hash01(Seed) * 2.0 - 1.0) * MaxHours;
}

double FNpcScheduleJitter::Apply(double Hour, int32 Seed, double MaxHours)
{
	double H = Hour + HourOffset(Seed, MaxHours);
	// Wrap into [0,24) so a shift across midnight still reads as a valid clock hour.
	H = FMath::Fmod(H, 24.0);
	if (H < 0.0)
	{
		H += 24.0;
	}
	return H;
}
