// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcDetour.h"
#include "../NpcSeqMath.h"

namespace
{
	using GtcSeq::Hash01;
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
