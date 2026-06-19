// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeatherSystem.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	double SmoothStep01(double T)
	{
		const double C = FMath::Clamp(T, 0.0, 1.0);
		return C * C * (3.0 - 2.0 * C);
	}

	// The severity chain the director walks. Fog sits off-chain (see SeverityIndex).
	EWeatherKind ChainKind(int32 Index)
	{
		switch (FMath::Clamp(Index, 0, 4))
		{
		case 0:  return EWeatherKind::Clear;
		case 1:  return EWeatherKind::PartlyCloudy;
		case 2:  return EWeatherKind::Overcast;
		case 3:  return EWeatherKind::Rain;
		default: return EWeatherKind::Storm;
		}
	}
}

FWeatherParams FWeatherParams::Lerp(const FWeatherParams& A, const FWeatherParams& B, double Alpha)
{
	const double T = FMath::Clamp(Alpha, 0.0, 1.0);
	FWeatherParams R;
	R.CloudCoverage = FMath::Lerp(A.CloudCoverage, B.CloudCoverage, T);
	R.CloudDensity = FMath::Lerp(A.CloudDensity, B.CloudDensity, T);
	R.FogDensity = FMath::Lerp(A.FogDensity, B.FogDensity, T);
	R.RainIntensity = FMath::Lerp(A.RainIntensity, B.RainIntensity, T);
	R.Wetness = FMath::Lerp(A.Wetness, B.Wetness, T);
	R.Wind = FMath::Lerp(A.Wind, B.Wind, T);
	R.SunLightScale = FMath::Lerp(A.SunLightScale, B.SunLightScale, T);
	R.SkyDesaturation = FMath::Lerp(A.SkyDesaturation, B.SkyDesaturation, T);
	return R;
}

FWeatherParams FWeatherParams::Preset(EWeatherKind Kind)
{
	FWeatherParams P;
	switch (Kind)
	{
	case EWeatherKind::Clear:
		P = { 0.08, 0.25, 0.03, 0.0, 0.0, 0.12, 1.00, 0.00 };
		break;
	case EWeatherKind::PartlyCloudy:
		P = { 0.40, 0.45, 0.05, 0.0, 0.0, 0.25, 0.92, 0.05 };
		break;
	case EWeatherKind::Overcast:
		P = { 0.85, 0.70, 0.12, 0.0, 0.15, 0.35, 0.60, 0.35 };
		break;
	case EWeatherKind::Rain:
		P = { 0.92, 0.85, 0.18, 0.65, 0.85, 0.45, 0.42, 0.45 };
		break;
	case EWeatherKind::Storm:
		P = { 1.00, 1.00, 0.25, 1.00, 1.00, 0.85, 0.22, 0.55 };
		break;
	case EWeatherKind::Fog:
		P = { 0.35, 0.50, 0.80, 0.0, 0.25, 0.08, 0.65, 0.30 };
		break;
	default:
		break;
	}
	return P;
}

double FWeatherParams::MaxAbsDelta(const FWeatherParams& A, const FWeatherParams& B)
{
	double M = 0.0;
	M = FMath::Max(M, FMath::Abs(A.CloudCoverage - B.CloudCoverage));
	M = FMath::Max(M, FMath::Abs(A.CloudDensity - B.CloudDensity));
	M = FMath::Max(M, FMath::Abs(A.FogDensity - B.FogDensity));
	M = FMath::Max(M, FMath::Abs(A.RainIntensity - B.RainIntensity));
	M = FMath::Max(M, FMath::Abs(A.Wetness - B.Wetness));
	M = FMath::Max(M, FMath::Abs(A.Wind - B.Wind));
	M = FMath::Max(M, FMath::Abs(A.SunLightScale - B.SunLightScale));
	M = FMath::Max(M, FMath::Abs(A.SkyDesaturation - B.SkyDesaturation));
	return M;
}

FWeatherSystem::FWeatherSystem()
{
	InitializeTo(EWeatherKind::Clear);
}

void FWeatherSystem::InitializeTo(EWeatherKind Kind)
{
	TargetKind = Kind;
	StartParams = FWeatherParams::Preset(Kind);
	CurrentParams = StartParams;
	TransitionSeconds = 0.0;
	Elapsed = 0.0;
	DwellElapsed = 0.0;
}

void FWeatherSystem::BeginTransition(EWeatherKind Kind, double Seconds)
{
	// Start from where the sky *currently* is (the live blend), so re-targeting
	// mid-transition stays continuous.
	StartParams = CurrentParams;
	TargetKind = Kind;
	TransitionSeconds = FMath::Max(Seconds, MinTransitionSeconds);
	Elapsed = 0.0;
	DwellElapsed = 0.0;
}

double FWeatherSystem::TransitionAlpha() const
{
	if (TransitionSeconds <= 0.0)
	{
		return 1.0;
	}
	return SmoothStep01(Elapsed / TransitionSeconds);
}

bool FWeatherSystem::Tick(double DeltaSeconds, double DwellSeconds)
{
	if (DeltaSeconds <= 0.0)
	{
		return false;
	}

	// Advance and recompute the live blend. Current is always a convex blend of
	// StartParams and the target preset, so it moves monotonically and is bounded
	// per frame — it cannot snap.
	Elapsed += DeltaSeconds;
	CurrentParams = FWeatherParams::Lerp(StartParams, FWeatherParams::Preset(TargetKind), TransitionAlpha());

	const bool bSettled = (TransitionSeconds <= 0.0) || (Elapsed >= TransitionSeconds);
	if (!bSettled)
	{
		return false;
	}

	// Spell has settled — count dwell and signal once it has lasted long enough.
	const bool bWasBelow = DwellElapsed < DwellSeconds;
	DwellElapsed += DeltaSeconds;
	if (bWasBelow && DwellElapsed >= DwellSeconds)
	{
		DwellElapsed = 0.0; // re-arm so auto mode keeps cycling even if ignored
		return true;
	}
	return false;
}

int32 FWeatherSystem::SeverityIndex(EWeatherKind Kind)
{
	switch (Kind)
	{
	case EWeatherKind::Clear:        return 0;
	case EWeatherKind::PartlyCloudy: return 1;
	case EWeatherKind::Overcast:     return 2;
	case EWeatherKind::Fog:          return 2; // off-chain, but as calm as overcast
	case EWeatherKind::Rain:         return 3;
	case EWeatherKind::Storm:        return 4;
	default:                         return 0;
	}
}

EWeatherKind FWeatherSystem::ChooseNextKind(EWeatherKind Current, double Roll)
{
	const double R = FMath::Clamp(Roll, 0.0, 0.999999);

	// Fog burns off gradually back into ordinary cloud.
	if (Current == EWeatherKind::Fog)
	{
		return R < 0.5 ? EWeatherKind::PartlyCloudy : EWeatherKind::Overcast;
	}

	const int32 Idx = SeverityIndex(Current); // 0..4 on the chain

	// From calm weather there is a small chance the next spell rolls in as fog.
	if (Idx <= 1 && R < 0.15)
	{
		return EWeatherKind::Fog;
	}

	// Otherwise step one rung along the severity chain, reflecting at the ends so
	// we always move to a genuinely new, adjacent spell (never a teleport, never
	// a no-op).
	const bool bStormier = (R >= 0.5);
	int32 Next = bStormier ? Idx + 1 : Idx - 1;
	if (Next < 0) { Next = 1; } // reflect off Clear
	if (Next > 4) { Next = 3; } // reflect off Storm
	return ChainKind(Next);
}
