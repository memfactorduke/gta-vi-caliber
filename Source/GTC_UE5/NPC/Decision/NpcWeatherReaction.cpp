// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcWeatherReaction.h"

bool FNpcWeatherReaction::IsNight(double Hour)
{
	// Wrap into [0,24) so a stray clock value still classifies sanely.
	double H = FMath::Fmod(Hour, 24.0);
	if (H < 0.0)
	{
		H += 24.0;
	}
	return H < 6.0 || H >= 20.0;
}

float FNpcWeatherReaction::BaseDiscomfort(int32 Severity)
{
	// 0 clear .. 4 storm. Smooth-ish ramp: a drizzle is mildly annoying, a storm
	// is miserable. Clamped so an out-of-range index is safe.
	switch (FMath::Clamp(Severity, 0, 4))
	{
	case 0:  return 0.00f; // clear
	case 1:  return 0.10f; // partly cloudy
	case 2:  return 0.25f; // overcast / fog
	case 3:  return 0.65f; // rain
	case 4:  return 0.95f; // storm
	default: return 0.00f;
	}
}

FNpcWeatherResponse FNpcWeatherReaction::Assess(int32 Severity, double Hour, double Bravery)
{
	FNpcWeatherResponse Out;

	float Discomfort = BaseDiscomfort(Severity);
	// Night is a touch less comfortable (cooler, darker) — but only when there's
	// already some weather or it's genuinely dark; a clear evening is still fine.
	if (IsNight(Hour))
	{
		Discomfort += 0.10f;
	}
	Out.Discomfort = FMath::Clamp(Discomfort, 0.0f, 1.0f);

	// Anything past a light drizzle makes people pick up the pace.
	Out.bHurry = Out.Discomfort >= 0.40f;

	// Seek shelter when it's genuinely unpleasant — but the bold tough it out
	// longer. Threshold scales with bravery: a timid citizen (Bravery 0) bails at
	// ~0.6 discomfort (so a drizzle's-worth of rain sends them in); a bold one
	// (Bravery 1) holds out to ~0.9 — which a storm (0.95) still clears, so a storm
	// drives everyone to cover while rain (0.65) only moves the timid.
	const float B = static_cast<float>(FMath::Clamp(Bravery, 0.0, 1.0));
	const float ShelterThreshold = 0.60f + 0.30f * B;
	Out.bSeekShelter = Out.Discomfort >= ShelterThreshold;

	return Out;
}
