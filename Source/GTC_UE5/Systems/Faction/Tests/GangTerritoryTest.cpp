// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GangTerritory.h"

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_gang_territory.gd. Float comparisons use absolute tolerance 1e-4.

namespace
{
    constexpr double FloatTolerance = 1e-4;

    GangTerritory MakeSample()
    {
        TArray<GangTerritory::FDistrictDef> Defs;
        Defs.Emplace(TEXT("north"), TEXT("coast_kings"));
        Defs.Emplace(TEXT("south"), TEXT("marina_cartel"));
        return GangTerritory(Defs);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryDefaultsOwnedTest,
    "GTC.Systems.Faction.GangTerritory.DefaultsOwnedByGangs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryDefaultsOwnedTest::RunTest(const FString& Parameters)
{
    GangTerritory T;
    TestEqual(TEXT("district_count == 4"), T.DistrictCount(), 4);
    TestEqual(TEXT("downtown owner"), T.OwnerOf(TEXT("downtown")), FString(TEXT("coast_kings")));
    TestEqual(TEXT("beach owner"), T.OwnerOf(TEXT("beach")), FString(TEXT("biscayne_set")));
    TestEqual(TEXT("player_districts empty"), T.PlayerDistricts().Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryDefaultInfluenceZeroTest,
    "GTC.Systems.Faction.GangTerritory.DefaultInfluenceZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryDefaultInfluenceZeroTest::RunTest(const FString& Parameters)
{
    GangTerritory T;
    TestEqual(TEXT("downtown influence == 0"), T.InfluenceIn(TEXT("downtown")), 0.0, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryAddInfluenceTest,
    "GTC.Systems.Faction.GangTerritory.AddInfluenceRaises",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryAddInfluenceTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 0.3);
    TestEqual(TEXT("north == 0.3"), T.InfluenceIn(TEXT("north")), 0.3, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryAddClampedTest,
    "GTC.Systems.Faction.GangTerritory.AddInfluenceClampedAtOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryAddClampedTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 0.8);
    T.AddInfluence(TEXT("north"), 0.8);
    TestEqual(TEXT("north == 1.0"), T.InfluenceIn(TEXT("north")), 1.0, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryNegativeIgnoredTest,
    "GTC.Systems.Faction.GangTerritory.NegativeInfluenceIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryNegativeIgnoredTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), -0.5);
    TestEqual(TEXT("north == 0"), T.InfluenceIn(TEXT("north")), 0.0, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryLoseLowersTest,
    "GTC.Systems.Faction.GangTerritory.LoseInfluenceLowers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryLoseLowersTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 0.6);
    T.LoseInfluence(TEXT("north"), 0.2);
    TestEqual(TEXT("north == 0.4"), T.InfluenceIn(TEXT("north")), 0.4, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryLoseFlooredTest,
    "GTC.Systems.Faction.GangTerritory.LoseInfluenceFlooredAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryLoseFlooredTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 0.3);
    T.LoseInfluence(TEXT("north"), 0.9);
    TestEqual(TEXT("north == 0"), T.InfluenceIn(TEXT("north")), 0.0, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryLoseNegativeIgnoredTest,
    "GTC.Systems.Faction.GangTerritory.LoseInfluenceNegativeIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryLoseNegativeIgnoredTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 0.5);
    T.LoseInfluence(TEXT("north"), -0.2);
    TestEqual(TEXT("north == 0.5"), T.InfluenceIn(TEXT("north")), 0.5, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryIsContestedTest,
    "GTC.Systems.Faction.GangTerritory.IsContestedThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryIsContestedTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 0.4);
    TestTrue(TEXT("contested at 0.3"), T.IsContested(TEXT("north"), 0.3));
    TestFalse(TEXT("not contested at 0.5"), T.IsContested(TEXT("north"), 0.5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryTakeOverFailsBelowFullTest,
    "GTC.Systems.Faction.GangTerritory.TakeOverFailsBelowFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryTakeOverFailsBelowFullTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 0.9);
    const bool bFlipped = T.TakeOver(TEXT("north"));
    TestFalse(TEXT("not flipped"), bFlipped);
    TestEqual(TEXT("owner still coast_kings"), T.OwnerOf(TEXT("north")), FString(TEXT("coast_kings")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryTakeOverSucceedsTest,
    "GTC.Systems.Faction.GangTerritory.TakeOverSucceedsAtFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryTakeOverSucceedsTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 1.0);
    const bool bFlipped = T.TakeOver(TEXT("north"));
    TestTrue(TEXT("flipped"), bFlipped);
    TestEqual(TEXT("owner player"), T.OwnerOf(TEXT("north")), FString(TEXT("player")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryTakeOverTwiceTest,
    "GTC.Systems.Faction.GangTerritory.TakeOverTwiceReturnsFalse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryTakeOverTwiceTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 1.0);
    T.TakeOver(TEXT("north"));
    TestFalse(TEXT("second take_over false"), T.TakeOver(TEXT("north")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryPlayerDistrictsUpdateTest,
    "GTC.Systems.Faction.GangTerritory.PlayerDistrictsUpdate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryPlayerDistrictsUpdateTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("south"), 1.0);
    T.TakeOver(TEXT("south"));
    const TArray<FString> Owned = T.PlayerDistricts();
    TestEqual(TEXT("one district"), Owned.Num(), 1);
    if (Owned.Num() == 1)
    {
        TestEqual(TEXT("== south"), Owned[0], FString(TEXT("south")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryControlledFractionTest,
    "GTC.Systems.Faction.GangTerritory.ControlledFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryControlledFractionTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 1.0);
    T.TakeOver(TEXT("north"));
    TestEqual(TEXT("fraction == 0.5"), T.ControlledFraction(), 0.5, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryAllOwnedTest,
    "GTC.Systems.Faction.GangTerritory.AllOwnedAfterFullTakeover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryAllOwnedTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 1.0);
    T.AddInfluence(TEXT("south"), 1.0);
    T.TakeOver(TEXT("north"));
    T.TakeOver(TEXT("south"));
    TestTrue(TEXT("all_owned"), T.AllOwned());
    TestEqual(TEXT("fraction == 1.0"), T.ControlledFraction(), 1.0, FloatTolerance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryAllOwnedFalseInitiallyTest,
    "GTC.Systems.Faction.GangTerritory.AllOwnedFalseInitially",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryAllOwnedFalseInitiallyTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    TestFalse(TEXT("not all_owned"), T.AllOwned());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryUnknownSafeTest,
    "GTC.Systems.Faction.GangTerritory.UnknownDistrictSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryUnknownSafeTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("ghost"), 0.5);
    T.LoseInfluence(TEXT("ghost"), 0.5);
    TestEqual(TEXT("ghost owner empty"), T.OwnerOf(TEXT("ghost")), FString());
    TestEqual(TEXT("ghost influence 0"), T.InfluenceIn(TEXT("ghost")), 0.0, FloatTolerance);
    TestFalse(TEXT("ghost not contested"), T.IsContested(TEXT("ghost"), 0.0));
    TestFalse(TEXT("ghost take_over false"), T.TakeOver(TEXT("ghost")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryFractionZeroEmptyTest,
    "GTC.Systems.Faction.GangTerritory.ControlledFractionZeroWhenEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryFractionZeroEmptyTest::RunTest(const FString& Parameters)
{
    // Godot: GangTerritory.new([]) then restore({"districts": []}).
    GangTerritory T;
    T.Restore(TArray<GangTerritory::FDistrictDef>());
    TestEqual(TEXT("fraction == 0"), T.ControlledFraction(), 0.0, FloatTolerance);
    TestFalse(TEXT("not all_owned"), T.AllOwned());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritorySerializeRestoreTest,
    "GTC.Systems.Faction.GangTerritory.SerializeRestoreRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritorySerializeRestoreTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    T.AddInfluence(TEXT("north"), 1.0);
    T.TakeOver(TEXT("north"));
    T.AddInfluence(TEXT("south"), 0.45);
    const TArray<GangTerritory::FDistrictDef> Snapshot = T.Serialize();
    GangTerritory Other;
    Other.Restore(Snapshot);
    TestEqual(TEXT("district_count == 2"), Other.DistrictCount(), 2);
    TestEqual(TEXT("north owner player"), Other.OwnerOf(TEXT("north")), FString(TEXT("player")));
    TestEqual(TEXT("south influence 0.45"), Other.InfluenceIn(TEXT("south")), 0.45, FloatTolerance);
    TestEqual(TEXT("south owner marina_cartel"), Other.OwnerOf(TEXT("south")), FString(TEXT("marina_cartel")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryRestoreMalformedTest,
    "GTC.Systems.Faction.GangTerritory.RestoreMalformedEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryRestoreMalformedTest::RunTest(const FString& Parameters)
{
    GangTerritory T = MakeSample();
    // Godot: restore({"districts": "junk"}) — non-array payload leaves an empty map.
    T.RestoreEmpty();
    TestEqual(TEXT("district_count == 0"), T.DistrictCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGangTerritoryResetClearsTest,
    "GTC.Systems.Faction.GangTerritory.ResetClears",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGangTerritoryResetClearsTest::RunTest(const FString& Parameters)
{
    GangTerritory T;
    T.AddInfluence(TEXT("downtown"), 1.0);
    T.TakeOver(TEXT("downtown"));
    T.Reset();
    TestEqual(TEXT("downtown influence 0"), T.InfluenceIn(TEXT("downtown")), 0.0, FloatTolerance);
    TestEqual(TEXT("downtown owner coast_kings"), T.OwnerOf(TEXT("downtown")), FString(TEXT("coast_kings")));
    TestEqual(TEXT("player_districts empty"), T.PlayerDistricts().Num(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
