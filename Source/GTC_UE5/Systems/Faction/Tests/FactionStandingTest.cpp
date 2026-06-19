// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FactionStanding.h"
#include "../GangTerritory.h"

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_faction_standing.gd.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingDefaultFactionsLoadedTest,
    "GTC.Systems.Faction.FactionStanding.DefaultFactionsLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingDefaultFactionsLoadedTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    TestEqual(TEXT("faction_count == 4"), F.FactionCount(), 4);
    TestTrue(TEXT("has coast_kings"), F.HasFaction(TEXT("coast_kings")));
    TestTrue(TEXT("has police"), F.HasFaction(TEXT("police")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingMalformedDroppedTest,
    "GTC.Systems.Faction.FactionStanding.MalformedFactionsDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingMalformedDroppedTest::RunTest(const FString& Parameters)
{
    TArray<FactionStanding::FFactionDef> Defs;
    Defs.Emplace(TEXT("ok"), TEXT(""));   // {"id": "ok"}
    Defs.Emplace(TEXT(""), TEXT(""));     // {"id": ""} — empty id dropped
    Defs.Emplace(TEXT(""), TEXT("x"));    // {"rival": "x"} — no id dropped
    Defs.Emplace(TEXT("ok"), TEXT("y"));  // duplicate id dropped
    FactionStanding F(Defs);
    TestEqual(TEXT("faction_count == 1"), F.FactionCount(), 1);
    TestTrue(TEXT("has ok"), F.HasFaction(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingDefaultNeutralTest,
    "GTC.Systems.Faction.FactionStanding.DefaultStandingNeutral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingDefaultNeutralTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    TestEqual(TEXT("standing coast_kings == 0"), F.StandingOf(TEXT("coast_kings")), 0);
    TestEqual(TEXT("tier coast_kings == neutral"), F.TierOf(TEXT("coast_kings")), FString(TEXT("neutral")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingUnknownNeutralTest,
    "GTC.Systems.Faction.FactionStanding.UnknownIsNeutral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingUnknownNeutralTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    TestEqual(TEXT("standing nope == 0"), F.StandingOf(TEXT("nope")), 0);
    TestEqual(TEXT("tier nope == empty"), F.TierOf(TEXT("nope")), FString());
    TestFalse(TEXT("not hostile nope"), F.IsHostile(TEXT("nope")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingAdjustRaisesLowersTest,
    "GTC.Systems.Faction.FactionStanding.AdjustRaisesAndLowers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingAdjustRaisesLowersTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    F.Adjust(TEXT("biscayne_set"), 30, 0.0);
    const int32 Up = F.StandingOf(TEXT("biscayne_set"));
    F.Adjust(TEXT("biscayne_set"), -50, 0.0);
    TestEqual(TEXT("up == 30"), Up, 30);
    TestEqual(TEXT("after -50 == -20"), F.StandingOf(TEXT("biscayne_set")), -20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingClampsTest,
    "GTC.Systems.Faction.FactionStanding.StandingClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingClampsTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    F.Adjust(TEXT("police"), 9999, 0.0);
    const int32 Hi = F.StandingOf(TEXT("police"));
    F.Adjust(TEXT("police"), -9999, 0.0);
    TestEqual(TEXT("hi == MaxStanding"), Hi, FactionStanding::MaxStanding);
    TestEqual(TEXT("lo == MinStanding"), F.StandingOf(TEXT("police")), FactionStanding::MinStanding);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingRivalrySpilloverTest,
    "GTC.Systems.Faction.FactionStanding.RivalrySpillover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingRivalrySpilloverTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    // Help coast_kings; its rival marina_cartel sours by half.
    F.Adjust(TEXT("coast_kings"), 40, 0.5);
    TestEqual(TEXT("coast_kings == 40"), F.StandingOf(TEXT("coast_kings")), 40);
    TestEqual(TEXT("marina_cartel == -20"), F.StandingOf(TEXT("marina_cartel")), -20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingTiersTest,
    "GTC.Systems.Faction.FactionStanding.Tiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingTiersTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    F.SetStanding(TEXT("police"), -50);
    const FString Hostile = F.TierOf(TEXT("police"));
    F.SetStanding(TEXT("police"), 20);
    const FString Friendly = F.TierOf(TEXT("police"));
    F.SetStanding(TEXT("police"), 60);
    const FString Allied = F.TierOf(TEXT("police"));
    TestEqual(TEXT("hostile"), Hostile, FString(TEXT("hostile")));
    TestEqual(TEXT("friendly"), Friendly, FString(TEXT("friendly")));
    TestEqual(TEXT("allied"), Allied, FString(TEXT("allied")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingAttackAssistGatesTest,
    "GTC.Systems.Faction.FactionStanding.AttackAndAssistGates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingAttackAssistGatesTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    F.SetStanding(TEXT("coast_kings"), -60);
    TestTrue(TEXT("will_attack"), F.WillAttack(TEXT("coast_kings")));
    TestTrue(TEXT("is_hostile"), F.IsHostile(TEXT("coast_kings")));
    F.SetStanding(TEXT("coast_kings"), 50);
    TestTrue(TEXT("will_assist"), F.WillAssist(TEXT("coast_kings")));
    TestTrue(TEXT("is_allied"), F.IsAllied(TEXT("coast_kings")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingSaveRoundTripTest,
    "GTC.Systems.Faction.FactionStanding.SaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingSaveRoundTripTest::RunTest(const FString& Parameters)
{
    FactionStanding F;
    F.SetStanding(TEXT("coast_kings"), 35);
    F.SetStanding(TEXT("police"), -25);
    FactionStanding Restored;
    Restored.LoadMap(F.ToMap());
    TestEqual(TEXT("coast_kings == 35"), Restored.StandingOf(TEXT("coast_kings")), 35);
    TestEqual(TEXT("police == -25"), Restored.StandingOf(TEXT("police")), -25);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFactionStandingAlignsWithTerritoryTest,
    "GTC.Systems.Faction.FactionStanding.AlignsWithGangTerritory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFactionStandingAlignsWithTerritoryTest::RunTest(const FString& Parameters)
{
    // Cross-class alignment: a district's turf owner is a faction you hold standing with.
    GangTerritory GT;
    const FString Owner = GT.OwnerOf(TEXT("downtown"));
    FactionStanding F;
    TestEqual(TEXT("owner == coast_kings"), Owner, FString(TEXT("coast_kings")));
    TestTrue(TEXT("faction has owner"), F.HasFaction(Owner));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
