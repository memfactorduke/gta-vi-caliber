// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcDetour.h"

namespace
{
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

float FNpcDetour::GlanceChance(double Curiosity)
{
	const float C = static_cast<float>(FMath::Clamp(Curiosity, 0.0, 1.0));
	// 4% for the incurious up to 20% for the nosiest, per opportunity.
	return 0.04f + 0.16f * C;
}

double FNpcDetour::GlanceCooldown(double Curiosity, int32 Seed)
{
	const double C = FMath::Clamp(Curiosity, 0.0, 1.0);
	// Base 6s, shorter for the curious (down to ~4s), plus up to 4s of jitter so
	// glances don't fall into a fixed rhythm.
	const double Base = 6.0 - 2.0 * C;
	return Base + Hash01(Seed) * 4.0;
}
