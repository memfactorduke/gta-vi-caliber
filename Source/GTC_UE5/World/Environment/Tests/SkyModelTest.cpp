// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SkyModel.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FSkyModel — the continuous time-of-day -> sun/sky math. The headline
 * tests prove the day<->night transition is structurally smooth: sweeping the
 * clock through a full 24h (including the midnight wrap) never produces a
 * discontinuity in sun pitch or daylight strength. Prefix GTC.World.Sky.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkyModelClockTest,
	"GTC.World.Sky.Clock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkyModelClockTest::RunTest(const FString& Parameters)
{
	// WrapHours folds anything into [0,24).
	TestTrue(TEXT("wrap negative"), FMath::Abs(FSkyModel::WrapHours(-1.0) - 23.0) < Eps);
	TestTrue(TEXT("wrap over"), FMath::Abs(FSkyModel::WrapHours(25.0) - 1.0) < Eps);
	TestTrue(TEXT("wrap 24 -> 0"), FSkyModel::WrapHours(24.0) < Eps);

	// AdvanceHours: 1 real second at a 24s day = +1h; crossing midnight wraps.
	TestTrue(TEXT("advance one hour"),
		FMath::Abs(FSkyModel::AdvanceHours(10.0, 1.0, 24.0) - 11.0) < Eps);
	TestTrue(TEXT("advance wraps past midnight"),
		FMath::Abs(FSkyModel::AdvanceHours(23.5, 1.0, 24.0) - 0.5) < Eps);

	// A non-positive day length freezes the clock — no jump, no div-by-zero.
	TestTrue(TEXT("zero day length freezes"),
		FMath::Abs(FSkyModel::AdvanceHours(7.0, 5.0, 0.0) - 7.0) < Eps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkyModelSunGeometryTest,
	"GTC.World.Sky.SunGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkyModelSunGeometryTest::RunTest(const FString& Parameters)
{
	// Elevation: ~0 at the horizons, +1 at noon, -1 at midnight.
	TestTrue(TEXT("sunrise on horizon"), FMath::Abs(FSkyModel::SunElevation01(6.0)) < Eps);
	TestTrue(TEXT("sunset on horizon"), FMath::Abs(FSkyModel::SunElevation01(18.0)) < Eps);
	TestTrue(TEXT("noon overhead"), FMath::Abs(FSkyModel::SunElevation01(12.0) - 1.0) < Eps);
	TestTrue(TEXT("midnight lowest"), FMath::Abs(FSkyModel::SunElevation01(0.0) + 1.0) < Eps);

	// Pitch follows: overhead -> -90, midnight -> +90, horizon -> 0.
	TestTrue(TEXT("noon pitch -90"), FMath::Abs(FSkyModel::SunPitchDegrees(12.0) + 90.0) < Eps);
	TestTrue(TEXT("midnight pitch +90"), FMath::Abs(FSkyModel::SunPitchDegrees(0.0) - 90.0) < Eps);
	TestTrue(TEXT("sunrise pitch 0"), FMath::Abs(FSkyModel::SunPitchDegrees(6.0)) < Eps);

	// Daylight factor: full at noon, dark at midnight, exactly the twilight
	// midpoint at the horizon crossing.
	TestTrue(TEXT("noon fully lit"), FMath::Abs(FSkyModel::DaylightFactor(12.0) - 1.0) < Eps);
	TestTrue(TEXT("midnight dark"), FSkyModel::DaylightFactor(0.0) < Eps);
	TestTrue(TEXT("horizon is twilight midpoint"),
		FMath::Abs(FSkyModel::DaylightFactor(6.0) - 0.5) < Eps);

	// Warmth: neutral at high noon, warm (golden hour) near the horizon.
	TestTrue(TEXT("noon neutral"), FSkyModel::SunWarmth(12.0) < Eps);
	TestTrue(TEXT("sunrise warm"), FSkyModel::SunWarmth(6.0) > 0.9);

	// Stars are the complement of daylight.
	TestTrue(TEXT("stars out at midnight"), FSkyModel::StarOpacity(0.0) > 0.99);
	TestTrue(TEXT("no stars at noon"), FSkyModel::StarOpacity(12.0) < Eps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkyModelSmoothnessTest,
	"GTC.World.Sky.Smoothness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkyModelSmoothnessTest::RunTest(const FString& Parameters)
{
	// THE day->night-is-smooth proof: walk the clock all the way around in tiny
	// 0.01h steps (covering the midnight wrap) and assert no single step ever
	// jumps daylight or sun pitch. If anything were a discrete switch, one of
	// these deltas would spike.
	const double Step = 0.01;                 // hours per sample
	const double MaxDaylightStep = 0.05;      // analytic bound ~0.017; generous
	const double MaxPitchStepDeg = 1.5;       // analytic bound ~0.95 deg; generous

	double PrevDay = FSkyModel::DaylightFactor(0.0);
	double PrevPitch = FSkyModel::SunPitchDegrees(0.0);
	double H = 0.0;
	bool bDaylightSmooth = true;
	bool bPitchSmooth = true;
	double WorstDay = 0.0;
	double WorstPitch = 0.0;

	for (int32 i = 0; i < 2400; ++i)
	{
		H = FSkyModel::WrapHours(H + Step);
		const double Day = FSkyModel::DaylightFactor(H);
		const double Pitch = FSkyModel::SunPitchDegrees(H);

		const double DDay = FMath::Abs(Day - PrevDay);
		const double DPitch = FMath::Abs(Pitch - PrevPitch);
		WorstDay = FMath::Max(WorstDay, DDay);
		WorstPitch = FMath::Max(WorstPitch, DPitch);
		if (DDay > MaxDaylightStep) { bDaylightSmooth = false; }
		if (DPitch > MaxPitchStepDeg) { bPitchSmooth = false; }

		PrevDay = Day;
		PrevPitch = Pitch;
	}

	TestTrue(FString::Printf(TEXT("daylight never jumps (worst step %.4f)"), WorstDay), bDaylightSmooth);
	TestTrue(FString::Printf(TEXT("sun pitch never jumps (worst step %.4f deg)"), WorstPitch), bPitchSmooth);

	// And monotone across a half-day: brighter every step from midnight to noon.
	bool bMonotoneUp = true;
	double Prev = FSkyModel::DaylightFactor(0.0);
	for (int32 i = 1; i <= 1200; ++i)
	{
		const double Cur = FSkyModel::DaylightFactor(i * Step);
		if (Cur < Prev - Eps) { bMonotoneUp = false; }
		Prev = Cur;
	}
	TestTrue(TEXT("daylight rises monotonically midnight->noon"), bMonotoneUp);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
