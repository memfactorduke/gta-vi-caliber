// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CharacterRoster.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to a test_* function in the the reference reference behavior
// game/tests/unit/test_character_roster.gd (14 functions). Identical literals/tolerances;
// compound boolean returns are split into independent assertions. Deterministic numbers.

// 1/14 — test_default_roster_loaded
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterDefaultLoadedTest,
    "GTC.Systems.Roster.CharacterRoster.DefaultRosterLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterDefaultLoadedTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    TestEqual(TEXT("count == 2"), Roster.CharacterCount(), 2);
    TestTrue(TEXT("has mara"), Roster.HasCharacter(TEXT("mara")));
    TestTrue(TEXT("has rico"), Roster.HasCharacter(TEXT("rico")));
    return true;
}

// 2/14 — test_first_character_is_active
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterFirstActiveTest,
    "GTC.Systems.Roster.CharacterRoster.FirstCharacterIsActive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterFirstActiveTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    TestEqual(TEXT("active == mara"), Roster.Active(), FString(TEXT("mara")));
    TestEqual(TEXT("active_name == Mara"), Roster.ActiveName(), FString(TEXT("Mara")));
    return true;
}

// 3/14 — test_malformed_characters_dropped
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterMalformedDroppedTest,
    "GTC.Systems.Roster.CharacterRoster.MalformedCharactersDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterMalformedDroppedTest::RunTest(const FString& Parameters)
{
    // the reference rows: {"ok","OK"}, {"","Empty"} (empty id), {"NoId"} (missing id -> empty),
    // {"ok","Dupe"} (duplicate id). Typed-port note: a missing-"id" dict is unrepresentable
    // with a typed FRosterCharacter; the empty-id row covers the same drop path 1:1, and a
    // second empty-id row stands in for the missing-id row.
    FCharacterRoster Roster({
        FRosterCharacter(TEXT("ok"), TEXT("OK"), 0),
        FRosterCharacter(TEXT(""), TEXT("Empty"), 0),
        FRosterCharacter(TEXT(""), TEXT("NoId"), 0),
        FRosterCharacter(TEXT("ok"), TEXT("Dupe"), 0), // duplicate id
    });
    TestEqual(TEXT("count == 1"), Roster.CharacterCount(), 1);
    TestTrue(TEXT("has ok"), Roster.HasCharacter(TEXT("ok")));
    return true;
}

// 4/14 — test_money_defaults
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterMoneyDefaultsTest,
    "GTC.Systems.Roster.CharacterRoster.MoneyDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterMoneyDefaultsTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    TestEqual(TEXT("mara == 2500"), Roster.MoneyOf(TEXT("mara")), 2500);
    TestEqual(TEXT("rico == 1500"), Roster.MoneyOf(TEXT("rico")), 1500);
    TestEqual(TEXT("nope == 0"), Roster.MoneyOf(TEXT("nope")), 0);
    return true;
}

// 5/14 — test_switch_changes_active
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSwitchChangesActiveTest,
    "GTC.Systems.Roster.CharacterRoster.SwitchChangesActive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSwitchChangesActiveTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    const bool bOk = Roster.SwitchTo(TEXT("rico"));
    TestTrue(TEXT("switch ok"), bOk);
    TestEqual(TEXT("active == rico"), Roster.Active(), FString(TEXT("rico")));
    return true;
}

// 6/14 — test_switch_to_unknown_fails
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSwitchUnknownFailsTest,
    "GTC.Systems.Roster.CharacterRoster.SwitchToUnknownFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSwitchUnknownFailsTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    TestFalse(TEXT("switch nope fails"), Roster.SwitchTo(TEXT("nope")));
    TestEqual(TEXT("active still mara"), Roster.Active(), FString(TEXT("mara")));
    return true;
}

// 7/14 — test_switch_to_active_fails
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSwitchActiveFailsTest,
    "GTC.Systems.Roster.CharacterRoster.SwitchToActiveFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSwitchActiveFailsTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    TestFalse(TEXT("switch to active fails"), Roster.SwitchTo(TEXT("mara")));
    TestEqual(TEXT("active still mara"), Roster.Active(), FString(TEXT("mara")));
    return true;
}

// 8/14 — test_switch_cooldown
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSwitchCooldownTest,
    "GTC.Systems.Roster.CharacterRoster.SwitchCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSwitchCooldownTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    Roster.SwitchTo(TEXT("rico"), 0.0);
    // Immediately switching back is on cooldown; after the window it's allowed.
    const bool bBlocked = Roster.SwitchTo(TEXT("mara"), 1.0);
    const bool bAllowed = Roster.SwitchTo(TEXT("mara"), FCharacterRoster::SwitchCooldown);
    TestFalse(TEXT("blocked at 1.0"), bBlocked);
    TestTrue(TEXT("allowed at cooldown"), bAllowed);
    TestEqual(TEXT("active == mara"), Roster.Active(), FString(TEXT("mara")));
    return true;
}

// 9/14 — test_repeated_untimed_switches
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterRepeatedUntimedTest,
    "GTC.Systems.Roster.CharacterRoster.RepeatedUntimedSwitches",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterRepeatedUntimedTest::RunTest(const FString& Parameters)
{
    // A caller that doesn't pass a timestamp can switch back and forth freely.
    FCharacterRoster Roster;
    TestTrue(TEXT("-> rico"), Roster.SwitchTo(TEXT("rico")));
    TestTrue(TEXT("-> mara"), Roster.SwitchTo(TEXT("mara")));
    TestTrue(TEXT("-> rico"), Roster.SwitchTo(TEXT("rico")));
    TestEqual(TEXT("active == rico"), Roster.Active(), FString(TEXT("rico")));
    return true;
}

// 10/14 — test_independent_money_persists_across_switch
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterIndependentMoneyTest,
    "GTC.Systems.Roster.CharacterRoster.IndependentMoneyPersistsAcrossSwitch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterIndependentMoneyTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    Roster.AddMoney(TEXT("mara"), 1000); // 3500
    Roster.SwitchTo(TEXT("rico"));
    Roster.AddMoney(TEXT("rico"), 500); // 2000
    // Mara's wallet is untouched by Rico's activity.
    TestEqual(TEXT("mara == 3500"), Roster.MoneyOf(TEXT("mara")), 3500);
    TestEqual(TEXT("rico == 2000"), Roster.MoneyOf(TEXT("rico")), 2000);
    return true;
}

// 11/14 — test_add_money_floors_at_zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterAddMoneyFloorsTest,
    "GTC.Systems.Roster.CharacterRoster.AddMoneyFloorsAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterAddMoneyFloorsTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    Roster.AddMoney(TEXT("rico"), -9999);
    TestEqual(TEXT("rico == 0"), Roster.MoneyOf(TEXT("rico")), 0);
    return true;
}

// 12/14 — test_wanted_clamps
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterWantedClampsTest,
    "GTC.Systems.Roster.CharacterRoster.WantedClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterWantedClampsTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    Roster.SetWanted(TEXT("mara"), 9);
    const int32 High = Roster.WantedOf(TEXT("mara"));
    Roster.SetWanted(TEXT("mara"), -2);
    TestEqual(TEXT("clamped to MaxStars"), High, FCharacterRoster::MaxStars);
    TestEqual(TEXT("clamped to 0"), Roster.WantedOf(TEXT("mara")), 0);
    return true;
}

// 13/14 — test_position_persists_per_character
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterPositionPersistsTest,
    "GTC.Systems.Roster.CharacterRoster.PositionPersistsPerCharacter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterPositionPersistsTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    // No Z-up remap: the reference Vector3(10,0,20)/(-5,0,8) carried 1:1 into FVector.
    Roster.SetPosition(TEXT("mara"), FVector(10, 0, 20));
    Roster.SetPosition(TEXT("rico"), FVector(-5, 0, 8));
    TestTrue(TEXT("mara pos"), Roster.PositionOf(TEXT("mara")).Equals(FVector(10, 0, 20), Eps));
    TestTrue(TEXT("rico pos"), Roster.PositionOf(TEXT("rico")).Equals(FVector(-5, 0, 8), Eps));
    return true;
}

// 14/14 — test_save_round_trip
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSaveRoundTripTest,
    "GTC.Systems.Roster.CharacterRoster.SaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSaveRoundTripTest::RunTest(const FString& Parameters)
{
    FCharacterRoster Roster;
    Roster.AddMoney(TEXT("mara"), 1000); // 3500
    Roster.SetWanted(TEXT("rico"), 3);
    Roster.SetPosition(TEXT("rico"), FVector(1, 2, 3));
    Roster.SwitchTo(TEXT("rico"));

    FString SavedActive;
    TArray<TPair<FString, FCharacterRoster::FSnapshot>> SavedChars;
    Roster.Serialize(SavedActive, SavedChars);

    FCharacterRoster Restored;
    Restored.Deserialize(SavedActive, SavedChars);
    TestEqual(TEXT("active == rico"), Restored.Active(), FString(TEXT("rico")));
    TestEqual(TEXT("mara == 3500"), Restored.MoneyOf(TEXT("mara")), 3500);
    TestEqual(TEXT("rico wanted == 3"), Restored.WantedOf(TEXT("rico")), 3);
    TestTrue(TEXT("rico pos == (1,2,3)"), Restored.PositionOf(TEXT("rico")).Equals(FVector(1, 2, 3), Eps));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
