// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WeatherSystem.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;
using GtcTest::ConvergeEps;

/**
 * Tests for FWeatherSystem — presets, the timed blend, and the gradual director.
 * The headline tests prove a weather change is structurally smooth and slow: it
 * never reaches the new preset on the frame it starts, every channel moves in
 * tiny bounded steps, and the director only ever walks to an adjacent severity
 * (clear can become partly-cloudy or fog, never storm). Prefix GTC.World.Weather.
 */

namespace
{
	bool InUnitRange(double V) { return V >= -Eps && V <= 1.0 + Eps; }

	bool ParamsInRange(const FWeatherParams& P)
	{
		return InUnitRange(P.CloudCoverage) && InUnitRange(P.CloudDensity)
			&& InUnitRange(P.FogDensity) && InUnitRange(P.RainIntensity)
			&& InUnitRange(P.Wetness) && InUnitRange(P.Wind)
			&& InUnitRange(P.SunLightScale) && InUnitRange(P.SkyDesaturation);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherPresetTest,
	"GTC.World.Weather.Presets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherPresetTest::RunTest(const FString& Parameters)
{
	for (int32 k = 0; k < (int32)EWeatherKind::Count; ++k)
	{
		const FWeatherParams P = FWeatherParams::Preset((EWeatherKind)k);
		TestTrue(FString::Printf(TEXT("preset %d in [0,1]"), k), ParamsInRange(P));
	}

	const FWeatherParams Clear = FWeatherParams::Preset(EWeatherKind::Clear);
	const FWeatherParams Storm = FWeatherParams::Preset(EWeatherKind::Storm);
	TestTrue(TEXT("clear is dry"), Clear.RainIntensity < Eps);
	TestTrue(TEXT("clear sun full"), FMath::Abs(Clear.SunLightScale - 1.0) < Eps);
	TestTrue(TEXT("storm pours"), Storm.RainIntensity > 0.9);
	TestTrue(TEXT("storm darkens the sun"), Storm.SunLightScale < 0.5);

	// Lerp endpoints + midpoint.
	const FWeatherParams Mid = FWeatherParams::Lerp(Clear, Storm, 0.5);
	TestTrue(TEXT("lerp@0 is A"), FWeatherParams::MaxAbsDelta(FWeatherParams::Lerp(Clear, Storm, 0.0), Clear) < Eps);
	TestTrue(TEXT("lerp@1 is B"), FWeatherParams::MaxAbsDelta(FWeatherParams::Lerp(Clear, Storm, 1.0), Storm) < Eps);
	// Clear rain 0.0 -> Storm rain 1.0, so the half-blend sits at 0.5.
	TestTrue(TEXT("lerp midpoint rain 0.5"), FMath::Abs(Mid.RainIntensity - 0.5) < Eps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherSmoothTransitionTest,
	"GTC.World.Weather.SmoothTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherSmoothTransitionTest::RunTest(const FString& Parameters)
{
	const double Dt = 1.0 / 30.0;            // ~33ms frame
	const double TransitionSecs = 60.0;
	const double Dwell = 100000.0;           // huge — never roll a new spell mid-test

	FWeatherSystem W;
	W.InitializeTo(EWeatherKind::Clear);
	W.BeginTransition(EWeatherKind::Storm, TransitionSecs);

	// One frame in: it must NOT have snapped to storm.
	FWeatherParams Prev = W.Current();
	W.Tick(Dt, Dwell);
	TestTrue(TEXT("not instant: still far from storm after 1 frame"),
		FWeatherParams::MaxAbsDelta(W.Current(), FWeatherParams::Preset(EWeatherKind::Storm)) > 0.5);

	// Per-frame steps are tiny and rain only ever rises (monotone, no overshoot).
	double WorstStep = FWeatherParams::MaxAbsDelta(Prev, W.Current());
	bool bBounded = true;
	bool bRainMonotone = true;
	const double MaxStep = 0.01;             // analytic bound ~0.00083; very generous
	double PrevRain = W.Current().RainIntensity;
	Prev = W.Current();

	const int32 Frames = (int32)(TransitionSecs / Dt) + 30; // run a bit past settle
	for (int32 i = 0; i < Frames; ++i)
	{
		W.Tick(Dt, Dwell);
		const FWeatherParams Cur = W.Current();
		const double StepDelta = FWeatherParams::MaxAbsDelta(Prev, Cur);
		WorstStep = FMath::Max(WorstStep, StepDelta);
		if (StepDelta > MaxStep) { bBounded = false; }
		if (Cur.RainIntensity < PrevRain - Eps) { bRainMonotone = false; }
		if (Cur.RainIntensity > FWeatherParams::Preset(EWeatherKind::Storm).RainIntensity + Eps) { bRainMonotone = false; }
		PrevRain = Cur.RainIntensity;
		Prev = Cur;
	}

	TestTrue(FString::Printf(TEXT("every frame step is tiny (worst %.5f)"), WorstStep), bBounded);
	TestTrue(TEXT("rain rises monotonically, never overshoots"), bRainMonotone);

	// After the full transition it has converged onto the storm preset.
	TestTrue(TEXT("converges to storm after transition"),
		FWeatherParams::MaxAbsDelta(W.Current(), FWeatherParams::Preset(EWeatherKind::Storm)) < ConvergeEps);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherNoInstantTest,
	"GTC.World.Weather.NoInstant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherNoInstantTest::RunTest(const FString& Parameters)
{
	// Even a caller asking for a zero-second change is forced to be gradual.
	FWeatherSystem W;
	W.InitializeTo(EWeatherKind::Clear);
	W.BeginTransition(EWeatherKind::Storm, 0.0);
	W.Tick(0.1, 100000.0);
	TestTrue(TEXT("zero-second request is clamped, not instant"),
		FWeatherParams::MaxAbsDelta(W.Current(), FWeatherParams::Preset(EWeatherKind::Storm)) > 0.5);

	// Re-targeting mid-transition stays continuous (no snap-back).
	FWeatherSystem W2;
	W2.InitializeTo(EWeatherKind::Clear);
	W2.BeginTransition(EWeatherKind::Storm, 60.0);
	for (int32 i = 0; i < 900; ++i) { W2.Tick(1.0 / 30.0, 100000.0); } // ~30s in, mid-blend
	const FWeatherParams Before = W2.Current();
	W2.BeginTransition(EWeatherKind::Clear, 60.0);
	W2.Tick(1.0 / 30.0, 100000.0);
	TestTrue(TEXT("re-target does not snap"),
		FWeatherParams::MaxAbsDelta(Before, W2.Current()) < 0.01);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherDirectorTest,
	"GTC.World.Weather.Director",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherDirectorTest::RunTest(const FString& Parameters)
{
	// From clear, the next spell is ALWAYS calm-adjacent — partly cloudy or fog,
	// never overcast/rain/storm. This is the "weather never teleports" guarantee.
	bool bClearNeverTeleports = true;
	for (int32 i = 0; i < 100; ++i)
	{
		const double Roll = i / 100.0;
		const EWeatherKind Next = FWeatherSystem::ChooseNextKind(EWeatherKind::Clear, Roll);
		const bool bOk = (Next == EWeatherKind::PartlyCloudy) || (Next == EWeatherKind::Fog);
		if (!bOk) { bClearNeverTeleports = false; }
	}
	TestTrue(TEXT("clear only steps to partly-cloudy or fog"), bClearNeverTeleports);

	// Every chain step moves at most one severity rung (fog excepted, it is off-chain).
	bool bAlwaysAdjacent = true;
	const EWeatherKind Chain[] = {
		EWeatherKind::Clear, EWeatherKind::PartlyCloudy, EWeatherKind::Overcast,
		EWeatherKind::Rain, EWeatherKind::Storm };
	for (EWeatherKind K : Chain)
	{
		for (int32 i = 0; i < 100; ++i)
		{
			const EWeatherKind Next = FWeatherSystem::ChooseNextKind(K, i / 100.0);
			if (Next == EWeatherKind::Fog) { continue; }
			const int32 D = FMath::Abs(FWeatherSystem::SeverityIndex(Next) - FWeatherSystem::SeverityIndex(K));
			if (D > 1) { bAlwaysAdjacent = false; }
			if (Next == K) { bAlwaysAdjacent = false; } // must be a genuinely new spell
		}
	}
	TestTrue(TEXT("director always steps to a new adjacent severity"), bAlwaysAdjacent);

	// Ends reflect inward rather than getting stuck.
	TestTrue(TEXT("storm steps back to rain when pushed up"),
		FWeatherSystem::ChooseNextKind(EWeatherKind::Storm, 0.99) == EWeatherKind::Rain);
	TestTrue(TEXT("fog burns off to cloud"),
		FWeatherSystem::ChooseNextKind(EWeatherKind::Fog, 0.0) == EWeatherKind::PartlyCloudy);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherDwellTest,
	"GTC.World.Weather.Dwell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherDwellTest::RunTest(const FString& Parameters)
{
	// A settled spell signals "roll a new one" only after the dwell elapses, then
	// re-arms — so auto weather cycles on its own cadence.
	FWeatherSystem W;
	W.InitializeTo(EWeatherKind::Clear); // settled immediately (no transition)

	int32 Fires = 0;
	int32 FirstFireStep = -1;
	for (int32 i = 0; i < 25; ++i)
	{
		const bool bRoll = W.Tick(1.0, 10.0); // dt=1s, dwell=10s
		if (bRoll)
		{
			++Fires;
			if (FirstFireStep < 0) { FirstFireStep = i + 1; }
		}
	}
	TestTrue(TEXT("dwell fires after ~10s, not before"), FirstFireStep == 10);
	TestTrue(TEXT("dwell re-arms and fires again"), Fires >= 2);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
