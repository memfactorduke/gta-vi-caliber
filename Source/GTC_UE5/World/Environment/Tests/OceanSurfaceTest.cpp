// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../OceanSurface.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FOceanSurface — the analytic Gerstner sum. Proves the surface is
 * well-formed (calm stays flat, height stays bounded by the amplitude sum, the
 * normal is unit length and faces up), that the wave geometry is correct (known
 * phase -> known height, deep-water dispersion), and that it is temporally
 * periodic and spatially continuous. Prefix GTC.World.Ocean.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanSurfaceCalmTest,
	"GTC.World.Ocean.Surface.Calm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanSurfaceCalmTest::RunTest(const FString& Parameters)
{
	// No waves at all -> dead-flat calm sample, never a NaN.
	const FOceanSample Empty = FOceanSurface::Sample(nullptr, 0, 10.0, -3.0, 7.0);
	TestTrue(TEXT("empty height 0"), FMath::Abs(Empty.Height) < Eps);
	TestTrue(TEXT("empty normal up"), FMath::Abs(Empty.NormalZ - 1.0) < Eps);
	TestTrue(TEXT("empty jacobian 1"), FMath::Abs(Empty.Jacobian - 1.0) < Eps);

	// A zero-amplitude wave is also calm.
	FGerstnerWave Flat;
	Flat.Amplitude = 0.0;
	const FOceanSample S = FOceanSurface::Sample(&Flat, 1, 4.0, 4.0, 1.0);
	TestTrue(TEXT("flat height 0"), FMath::Abs(S.Height) < Eps);
	TestTrue(TEXT("flat disp 0"), FMath::Abs(S.DispX) + FMath::Abs(S.DispY) < Eps);
	TestTrue(TEXT("flat normal up"), FMath::Abs(S.NormalZ - 1.0) < Eps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanSurfaceGeometryTest,
	"GTC.World.Ocean.Surface.Geometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanSurfaceGeometryTest::RunTest(const FString& Parameters)
{
	// Dispersion: omega = Speed*k when Speed>0, else sqrt(g*k).
	FGerstnerWave W;
	W.DirX = 1.0; W.DirY = 0.0;
	W.Amplitude = 0.5; W.Wavelength = 8.0; W.Steepness = 0.3; W.Speed = 0.0;
	const double K = FOceanSurface::Wavenumber(8.0);              // 2*pi/8
	TestTrue(TEXT("dispersion deep water"),
		FMath::Abs(FOceanSurface::AngularFrequency(W) - FMath::Sqrt(9.81 * K)) < Eps);
	W.Speed = 4.0;
	TestTrue(TEXT("dispersion fixed speed"),
		FMath::Abs(FOceanSurface::AngularFrequency(W) - 4.0 * K) < Eps);

	// Known phase -> known height. At T=0 along +X: phase = k*X, so height = A*sin(k*X).
	// X=0 -> 0 ; X=L/4 -> phase pi/2 -> height = A.
	TestTrue(TEXT("trough at origin"),
		FMath::Abs(FOceanSurface::HeightAt(&W, 1, 0.0, 0.0, 0.0)) < Eps);
	TestTrue(TEXT("crest at quarter wavelength"),
		FMath::Abs(FOceanSurface::HeightAt(&W, 1, 2.0, 0.0, 0.0) - 0.5) < Eps);

	// Steepness 0 => no horizontal displacement; steepness > 0 => some.
	FGerstnerWave Round = W; Round.Steepness = 0.0;
	const FOceanSample R = FOceanSurface::Sample(&Round, 1, 1.0, 0.0, 0.0);
	TestTrue(TEXT("round wave no disp"), FMath::Abs(R.DispX) + FMath::Abs(R.DispY) < Eps);
	const FOceanSample P = FOceanSurface::Sample(&W, 1, 1.0, 0.0, 0.0);
	TestTrue(TEXT("pinched wave has disp"), FMath::Abs(P.DispX) > Eps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanSurfaceWellFormedTest,
	"GTC.World.Ocean.Surface.WellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanSurfaceWellFormedTest::RunTest(const FString& Parameters)
{
	// A small realistic swell: three crossing waves.
	FGerstnerWave Waves[3];
	Waves[0].DirX = 1.0;  Waves[0].DirY = 0.1; Waves[0].Amplitude = 0.6; Waves[0].Wavelength = 18.0; Waves[0].Steepness = 0.5; Waves[0].Speed = 0.0;
	Waves[1].DirX = 0.6;  Waves[1].DirY = 0.8; Waves[1].Amplitude = 0.3; Waves[1].Wavelength = 9.0;  Waves[1].Steepness = 0.4; Waves[1].Speed = 0.0;
	Waves[2].DirX = -0.3; Waves[2].DirY = 1.0; Waves[2].Amplitude = 0.15;Waves[2].Wavelength = 4.0;  Waves[2].Steepness = 0.3; Waves[2].Speed = 0.0;
	const double AmpSum = 0.6 + 0.3 + 0.15;

	bool bBounded = true, bUnitNormal = true, bNormalUp = true;
	double WorstHeight = 0.0, WorstNormalErr = 0.0, MinNz = 1.0;
	for (int32 ix = 0; ix < 40; ++ix)
	{
		for (int32 iy = 0; iy < 40; ++iy)
		{
			const double X = ix * 0.7;
			const double Y = iy * 0.7;
			const double T = (ix + iy) * 0.05;
			const FOceanSample S = FOceanSurface::Sample(Waves, 3, X, Y, T);

			WorstHeight = FMath::Max(WorstHeight, FMath::Abs(S.Height));
			if (FMath::Abs(S.Height) > AmpSum + Eps) { bBounded = false; }

			const double NLen = FMath::Sqrt(S.NormalX * S.NormalX + S.NormalY * S.NormalY + S.NormalZ * S.NormalZ);
			WorstNormalErr = FMath::Max(WorstNormalErr, FMath::Abs(NLen - 1.0));
			if (FMath::Abs(NLen - 1.0) > 1e-6) { bUnitNormal = false; }

			MinNz = FMath::Min(MinNz, S.NormalZ);
			if (S.NormalZ <= 0.0) { bNormalUp = false; }
		}
	}

	TestTrue(FString::Printf(TEXT("height bounded by amp sum (worst %.3f <= %.3f)"), WorstHeight, AmpSum), bBounded);
	TestTrue(FString::Printf(TEXT("normal is unit length (worst err %.2e)"), WorstNormalErr), bUnitNormal);
	TestTrue(FString::Printf(TEXT("normal faces up (min Nz %.3f)"), MinNz), bNormalUp);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOceanSurfacePeriodicContinuousTest,
	"GTC.World.Ocean.Surface.PeriodicContinuous",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOceanSurfacePeriodicContinuousTest::RunTest(const FString& Parameters)
{
	// Single wave, fixed phase speed so the temporal period is exact: T = 2*pi/omega.
	FGerstnerWave W;
	W.DirX = 1.0; W.DirY = 0.0; W.Amplitude = 0.5; W.Wavelength = 8.0; W.Steepness = 0.4; W.Speed = 4.0;
	const double K = FOceanSurface::Wavenumber(8.0);
	const double Omega = 4.0 * K;
	const double Period = (2.0 * PI) / Omega;

	bool bPeriodic = true;
	for (int32 i = 0; i < 16; ++i)
	{
		const double T = i * 0.11;
		const double H0 = FOceanSurface::HeightAt(&W, 1, 1.3, 0.0, T);
		const double H1 = FOceanSurface::HeightAt(&W, 1, 1.3, 0.0, T + Period);
		if (FMath::Abs(H0 - H1) > 1e-6) { bPeriodic = false; }
	}
	TestTrue(TEXT("height is temporally periodic"), bPeriodic);

	// Spatial continuity: tiny ΔX never jumps the height (Lipschitz by k*A).
	const double DX = 0.01;
	const double MaxStep = K * W.Amplitude * DX * 2.0; // generous analytic bound
	bool bContinuous = true;
	double WorstStep = 0.0;
	double Prev = FOceanSurface::HeightAt(&W, 1, 0.0, 0.0, 1.0);
	for (int32 i = 1; i <= 2000; ++i)
	{
		const double Cur = FOceanSurface::HeightAt(&W, 1, i * DX, 0.0, 1.0);
		const double Step = FMath::Abs(Cur - Prev);
		WorstStep = FMath::Max(WorstStep, Step);
		if (Step > MaxStep) { bContinuous = false; }
		Prev = Cur;
	}
	TestTrue(FString::Printf(TEXT("height is spatially continuous (worst %.4f)"), WorstStep), bContinuous);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
