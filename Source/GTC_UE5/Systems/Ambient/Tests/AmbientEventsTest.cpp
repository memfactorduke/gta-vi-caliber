// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../AmbientEvents.h"

#include <limits>

// Each test below maps 1:1 to a test_* function in the Godot parity oracle
// the upstream Godot test test_ambient_events.gd (12 functions). Godot's compound boolean
// returns are split into independent TestTrue/TestFalse/TestEqual assertions with the oracle's exact
// constants. All helper names are system-prefixed (MakeAmbient*) to stay ODR-unique under unity.

namespace
{
	// Godot const CALM := {"stars": 0, "district": "downtown"}.
	FAmbientEvents::FAmbientContext MakeAmbientCalm()
	{
		FAmbientEvents::FAmbientContext Ctx;
		Ctx.Stars = 0.0;
		Ctx.District = TEXT("downtown");
		return Ctx;
	}

	FAmbientEvents::FAmbientContext MakeAmbientContext(double Stars, const FString& District)
	{
		FAmbientEvents::FAmbientContext Ctx;
		Ctx.Stars = Stars;
		Ctx.District = District;
		return Ctx;
	}

	// The -INF "never fired" sentinel must be asserted with EXACT equality, never with a tolerance
	// compare: FMath::Abs(-INF - (-INF)) is NaN and NaN < Eps is false, so TestEqual(..., -INF) (which
	// is tolerance-based) reports a false failure even when both values are -inf. This mirrors the
	// established INF-sentinel pattern (VehicleHealth TimeToExplosion +inf, WorldSaveGame) and the
	// Godot oracle's exact `last_fired_of(...) == -INF` (test_ambient_events.gd::test_reset_clears_cooldowns).
	bool IsAmbientNegInf(double Value)
	{
		return Value == -std::numeric_limits<double>::infinity();
	}
}

// test_default_events_loaded
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsDefaultsLoadedTest,
	"GTC.Systems.Ambient.AmbientEvents.DefaultEventsLoaded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsDefaultsLoadedTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	TestEqual(TEXT("event_count == 6"), A.EventCount(), 6);
	TestTrue(TEXT("has mugging"), A.HasEvent(TEXT("mugging")));
	TestTrue(TEXT("has gang_shootout"), A.HasEvent(TEXT("gang_shootout")));
	return true;
}

// test_malformed_events_dropped
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsMalformedDroppedTest,
	"GTC.Systems.Ambient.AmbientEvents.MalformedEventsDropped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsMalformedDroppedTest::RunTest(const FString& Parameters)
{
	// Godot roster: {"id":"ok","weight":1}, {"id":"","weight":1}, {"weight":1} (no id),
	// {"id":"zero","weight":0} (non-positive weight), {"id":"ok","weight":2} (duplicate id).
	// The "no id" Dictionary maps to a def with an empty Id, dropped by Register exactly as the
	// explicit empty-id case is.
	TArray<FAmbientEvents::FEventDef> Defs;
	Defs.Add({TEXT("ok"), TEXT("misc"), 1.0, 0, 5, FString(), 60.0});
	Defs.Add({TEXT(""), TEXT("misc"), 1.0, 0, 5, FString(), 60.0});
	Defs.Add({TEXT(""), TEXT("misc"), 1.0, 0, 5, FString(), 60.0}); // no id
	Defs.Add({TEXT("zero"), TEXT("misc"), 0.0, 0, 5, FString(), 60.0}); // non-positive weight
	Defs.Add({TEXT("ok"), TEXT("misc"), 2.0, 0, 5, FString(), 60.0}); // duplicate id

	FAmbientEvents A(Defs);
	TestEqual(TEXT("event_count == 1"), A.EventCount(), 1);
	TestTrue(TEXT("has ok"), A.HasEvent(TEXT("ok")));
	return true;
}

// test_kind_lookup
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsKindLookupTest,
	"GTC.Systems.Ambient.AmbientEvents.KindLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsKindLookupTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	TestEqual(TEXT("street_race kind == race"), A.KindOf(TEXT("street_race")), FString(TEXT("race")));
	TestEqual(TEXT("nope kind == \"\""), A.KindOf(TEXT("nope")), FString());
	return true;
}

// test_eligibility_by_stars
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsEligibilityByStarsTest,
	"GTC.Systems.Ambient.AmbientEvents.EligibilityByStars",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsEligibilityByStarsTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	// getaway_driver needs 1+ stars; not eligible at 0.
	const bool bCalm = A.CanFire(TEXT("getaway_driver"), 0.0, MakeAmbientCalm());
	const bool bHot = A.CanFire(TEXT("getaway_driver"), 0.0, MakeAmbientContext(2.0, TEXT("downtown")));
	TestFalse(TEXT("not eligible at 0 stars"), bCalm);
	TestTrue(TEXT("eligible at 2 stars"), bHot);
	return true;
}

// test_eligibility_by_district
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsEligibilityByDistrictTest,
	"GTC.Systems.Ambient.AmbientEvents.EligibilityByDistrict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsEligibilityByDistrictTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	// gang_shootout is docks-only.
	const bool bElsewhere = A.CanFire(TEXT("gang_shootout"), 0.0, MakeAmbientContext(3.0, TEXT("downtown")));
	const bool bAtDocks = A.CanFire(TEXT("gang_shootout"), 0.0, MakeAmbientContext(3.0, TEXT("docks")));
	TestFalse(TEXT("not eligible elsewhere"), bElsewhere);
	TestTrue(TEXT("eligible at docks"), bAtDocks);
	return true;
}

// test_eligible_ids_excludes_ineligible
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsEligibleIdsExcludesTest,
	"GTC.Systems.Ambient.AmbientEvents.EligibleIdsExcludesIneligible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsEligibleIdsExcludesTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	const TArray<FString> Elig = A.EligibleIds(0.0, MakeAmbientCalm());
	TestTrue(TEXT("includes mugging"), Elig.Contains(TEXT("mugging")));
	TestFalse(TEXT("excludes getaway_driver"), Elig.Contains(TEXT("getaway_driver")));
	TestFalse(TEXT("excludes gang_shootout"), Elig.Contains(TEXT("gang_shootout")));
	return true;
}

// test_cooldown_blocks_refire
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsCooldownBlocksRefireTest,
	"GTC.Systems.Ambient.AmbientEvents.CooldownBlocksRefire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsCooldownBlocksRefireTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	A.Trigger(TEXT("mugging"), 0.0);
	// mugging cooldown is 60s.
	const bool bSoon = A.CanFire(TEXT("mugging"), 30.0, MakeAmbientCalm());
	const bool bLater = A.CanFire(TEXT("mugging"), 60.0, MakeAmbientCalm());
	TestFalse(TEXT("blocked at 30s"), bSoon);
	TestTrue(TEXT("allowed at 60s"), bLater);
	return true;
}

// test_trigger_next_respects_global_gap
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsGlobalGapTest,
	"GTC.Systems.Ambient.AmbientEvents.TriggerNextRespectsGlobalGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsGlobalGapTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	FRandomStream RngFirst(1);
	const FString First = A.TriggerNext(RngFirst, 0.0, MakeAmbientCalm());
	// Within GLOBAL_GAP nothing else may fire.
	FRandomStream RngBlocked(2);
	const FString Blocked = A.TriggerNext(RngBlocked, 10.0, MakeAmbientCalm());
	TestNotEqual(TEXT("first fired"), First, FString());
	TestEqual(TEXT("second blocked == \"\""), Blocked, FString());
	return true;
}

// test_trigger_next_null_rng
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsNullRngTest,
	"GTC.Systems.Ambient.AmbientEvents.TriggerNextNullRng",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsNullRngTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	// Godot passes rng == null; UE expresses the null-guard as the TriggerNextNoRng() overload.
	TestEqual(TEXT("null rng == \"\""), A.TriggerNextNoRng(), FString());
	return true;
}

// test_trigger_next_picks_eligible_only
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsPicksEligibleOnlyTest,
	"GTC.Systems.Ambient.AmbientEvents.TriggerNextPicksEligibleOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsPicksEligibleOnlyTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	// At 0 stars in downtown, never pick a stars-gated or docks-only event.
	FRandomStream Rng(5);
	const FString Picked = A.TriggerNext(Rng, 0.0, MakeAmbientCalm());
	TestNotEqual(TEXT("picked something"), Picked, FString());
	TestNotEqual(TEXT("not getaway_driver"), Picked, FString(TEXT("getaway_driver")));
	TestNotEqual(TEXT("not gang_shootout"), Picked, FString(TEXT("gang_shootout")));
	return true;
}

// test_trigger_next_empty_when_nothing_eligible
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsEmptyWhenNothingEligibleTest,
	"GTC.Systems.Ambient.AmbientEvents.TriggerNextEmptyWhenNothingEligible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsEmptyWhenNothingEligibleTest::RunTest(const FString& Parameters)
{
	// A roster whose only event is docks-only; in another district nothing fires.
	TArray<FAmbientEvents::FEventDef> Defs;
	Defs.Add({TEXT("docks_only"), TEXT("misc"), 1.0, 0, 5, TEXT("docks"), 60.0});
	FAmbientEvents A(Defs);
	FRandomStream Rng(3);
	const FString Result = A.TriggerNext(Rng, 100.0, MakeAmbientContext(0.0, TEXT("beach")));
	TestEqual(TEXT("nothing eligible == \"\""), Result, FString());
	return true;
}

// test_reset_clears_cooldowns
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAmbientEventsResetClearsCooldownsTest,
	"GTC.Systems.Ambient.AmbientEvents.ResetClearsCooldowns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAmbientEventsResetClearsCooldownsTest::RunTest(const FString& Parameters)
{
	FAmbientEvents A;
	A.Trigger(TEXT("mugging"), 100.0);
	A.Reset();
	TestTrue(TEXT("last_fired == -INF"), IsAmbientNegInf(A.LastFiredOf(TEXT("mugging"))));
	TestTrue(TEXT("can_fire after reset"), A.CanFire(TEXT("mugging"), 0.0, MakeAmbientCalm()));
	return true;
}

#endif // WITH_AUTOMATION_TESTS
