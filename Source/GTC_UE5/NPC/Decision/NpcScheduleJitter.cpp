// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcScheduleJitter.h"
#include "../NpcSeqMath.h"

namespace
{
	// Deterministic hash -> [0,1). FNV-1a-ish with an avalanche finalizer, matching
	// the NPC layer's other hashed-but-stable choices (no RNG).
	using GtcSeq::Hash01;
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
