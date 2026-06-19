// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkyModel.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	// Smoothstep with explicit edges, clamped — C1-continuous 0->1 ramp.
	double SkySmoothStep(double Edge0, double Edge1, double X)
	{
		if (Edge1 <= Edge0)
		{
			return X < Edge0 ? 0.0 : 1.0;
		}
		const double T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.0, 1.0);
		return T * T * (3.0 - 2.0 * T);
	}
}

double FSkyModel::WrapHours(double Hours)
{
	// FMath::Fmod keeps the sign of the dividend, so fold negatives back up.
	double H = FMath::Fmod(Hours, 24.0);
	if (H < 0.0)
	{
		H += 24.0;
	}
	return H;
}

double FSkyModel::AdvanceHours(double Hours, double DeltaSeconds, double DayLengthSeconds)
{
	if (DayLengthSeconds <= 0.0)
	{
		// Frozen clock — never advance, never divide by zero.
		return WrapHours(Hours);
	}
	const double HoursPerSecond = 24.0 / DayLengthSeconds;
	return WrapHours(Hours + DeltaSeconds * HoursPerSecond);
}

double FSkyModel::SunElevation01(double Hours)
{
	// One sine period over 24h, zero at 6h and 18h, +1 at noon, -1 at midnight.
	// Single transcendental of the hour => smooth and seamless across the wrap.
	const double Phase = (WrapHours(Hours) - 6.0) / 24.0; // 0 at sunrise
	return FMath::Sin(2.0 * PI * Phase);
}

double FSkyModel::SunPitchDegrees(double Hours)
{
	// Overhead sun (elevation +1) => light points straight down => pitch -90.
	return -90.0 * SunElevation01(Hours);
}

double FSkyModel::SunYawDegrees(double Hours, double BaseYaw, double SwingDeg)
{
	// Smooth azimuth sweep: cosine over the day so the sun is east-ish at dawn,
	// crosses through BaseYaw near noon, west-ish at dusk. Continuous in the hour.
	const double Phase = (WrapHours(Hours) - 6.0) / 24.0;
	return BaseYaw + SwingDeg * -FMath::Cos(2.0 * PI * Phase);
}

double FSkyModel::DaylightFactor(double Hours)
{
	// Smooth twilight band centred on the horizon. Elevation ~ -0.12..+0.12 maps
	// the civil-twilight wedge, so the world fades up/down over real minutes.
	const double Elev = SunElevation01(Hours);
	return SkySmoothStep(-0.12, 0.12, Elev);
}

double FSkyModel::SunBrightnessLux(double Hours, double DayLux, double NightLux)
{
	return FMath::Lerp(NightLux, DayLux, DaylightFactor(Hours));
}

double FSkyModel::MoonBrightnessLux(double Hours, double MoonLux)
{
	// Inverse of daylight — moonlight floor that fades out as the sun comes up.
	return FMath::Lerp(MoonLux, 0.0, DaylightFactor(Hours));
}

double FSkyModel::SkyLightFactor(double Hours, double NightFloor)
{
	return FMath::Lerp(NightFloor, 1.0, DaylightFactor(Hours));
}

double FSkyModel::SunWarmth(double Hours)
{
	// Warmth peaks at the horizon (|elevation| small) and fades as the sun climbs.
	// 1 - smoothstep(|elev|) gives a smooth golden-hour bump around dawn/dusk.
	const double AbsElev = FMath::Abs(SunElevation01(Hours));
	return 1.0 - SkySmoothStep(0.02, 0.30, AbsElev);
}

double FSkyModel::StarOpacity(double Hours)
{
	return 1.0 - DaylightFactor(Hours);
}
