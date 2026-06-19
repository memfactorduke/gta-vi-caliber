// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Disguise.h"
#include "../../Wanted/WantedEvasion.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to a test_* func in the Godot parity oracle
// game/tests/unit/test_disguise.gd (11 funcs). Recognition sums float slot weights, so
// those use Eps; exact-equality oracle checks (== 0.0, counts, bools) stay exact. Compound
// `a and b` oracle returns are split into separate Test* calls. FWantedEvasion is REUSED
// from Systems/Wanted (NOT re-ported) for the cross-system test, exactly as the oracle.

// 1. test_fresh_has_no_description
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseFreshNoDescriptionTest,
    "GTC.Systems.Disguise.FreshHasNoDescription",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseFreshNoDescriptionTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    TestFalse(TEXT("no description"), D.HasDescription());
    TestEqual(TEXT("recognition == 0"), D.Recognition(), 0.0, Eps);
    TestEqual(TEXT("changed_slots == 0"), D.ChangedSlots(), 0);
    return true;
}

// 2. test_default_slots_present
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseDefaultSlotsPresentTest,
    "GTC.Systems.Disguise.DefaultSlotsPresent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseDefaultSlotsPresentTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    TestEqual(TEXT("4 slots"), D.Slots().Num(), 4);
    TestEqual(TEXT("mask == default"), D.Current(TEXT("mask")), FDisguise::DefaultLook);
    return true;
}

// 3. test_unknown_slot_is_neutral
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseUnknownSlotNeutralTest,
    "GTC.Systems.Disguise.UnknownSlotIsNeutral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseUnknownSlotNeutralTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    D.SetAppearance(TEXT("nope"), TEXT("x"));
    TestEqual(TEXT("unknown slot == \"\""), D.Current(TEXT("nope")), FString());
    TestFalse(TEXT("unknown slot != x"), D.Current(TEXT("nope")) == TEXT("x"));
    return true;
}

// 4. test_logged_unchanged_is_fully_recognized
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseLoggedUnchangedRecognizedTest,
    "GTC.Systems.Disguise.LoggedUnchangedIsFullyRecognized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseLoggedUnchangedRecognizedTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    D.LogSighting();
    TestTrue(TEXT("has description"), D.HasDescription());
    TestEqual(TEXT("recognition == 1.0"), D.Recognition(), 1.0, Eps);
    return true;
}

// 5. test_changing_mask_drops_recognition
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseChangingMaskDropsRecognitionTest,
    "GTC.Systems.Disguise.ChangingMaskDropsRecognition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseChangingMaskDropsRecognitionTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    D.LogSighting();
    D.SetAppearance(TEXT("mask"), TEXT("ski_mask"));  // mask weight 0.4 -> recognition 0.6
    TestEqual(TEXT("recognition == 0.6"), D.Recognition(), 0.6, Eps);
    TestEqual(TEXT("changed_slots == 1"), D.ChangedSlots(), 1);
    return true;
}

// 6. test_changing_everything_zeroes_recognition
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseChangingEverythingZeroesTest,
    "GTC.Systems.Disguise.ChangingEverythingZeroesRecognition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseChangingEverythingZeroesTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    D.LogSighting();
    D.SetAppearance(TEXT("outfit"), TEXT("tux"));
    D.SetAppearance(TEXT("mask"), TEXT("ski"));
    D.SetAppearance(TEXT("vehicle"), TEXT("taxi"));
    D.SetAppearance(TEXT("hair"), TEXT("blonde"));
    TestEqual(TEXT("recognition == 0.0"), D.Recognition(), 0.0, Eps);
    TestEqual(TEXT("changed_slots == 4"), D.ChangedSlots(), 4);
    return true;
}

// 7. test_speedup_scales_with_disguise
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseSpeedupScalesTest,
    "GTC.Systems.Disguise.SpeedupScalesWithDisguise",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseSpeedupScalesTest::RunTest(const FString& Parameters)
{
    FDisguise Recognized;
    Recognized.LogSighting();  // recognition 1 -> speedup 1.0
    FDisguise Disguised;
    Disguised.LogSighting();
    Disguised.SetAppearance(TEXT("mask"), TEXT("ski"));  // recognition 0.6 -> speedup 1.8
    TestEqual(TEXT("recognized speedup == 1.0"), Recognized.EvasionSpeedup(), 1.0, Eps);
    TestEqual(TEXT("disguised speedup == 1.8"), Disguised.EvasionSpeedup(), 1.8, Eps);
    return true;
}

// 8. test_speedup_max_when_no_description
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseSpeedupMaxNoDescriptionTest,
    "GTC.Systems.Disguise.SpeedupMaxWhenNoDescription",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseSpeedupMaxNoDescriptionTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    TestEqual(TEXT("speedup == MaxEvasionSpeedup"), D.EvasionSpeedup(), FDisguise::MaxEvasionSpeedup, Eps);
    return true;
}

// 9. test_is_recognized_threshold
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseIsRecognizedThresholdTest,
    "GTC.Systems.Disguise.IsRecognizedThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseIsRecognizedThresholdTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    D.LogSighting();
    D.SetAppearance(TEXT("mask"), TEXT("ski"));  // recognition 0.6
    const bool bAbove = D.IsRecognized(0.5);
    D.SetAppearance(TEXT("outfit"), TEXT("tux"));  // recognition 0.3
    const bool bBelow = D.IsRecognized(0.5);
    TestTrue(TEXT("above threshold at 0.6"), bAbove);
    TestFalse(TEXT("below threshold at 0.3"), bBelow);
    return true;
}

// 10. test_reset_to_clean_clears_description
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseResetToCleanTest,
    "GTC.Systems.Disguise.ResetToCleanClearsDescription",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseResetToCleanTest::RunTest(const FString& Parameters)
{
    FDisguise D;
    D.LogSighting();
    D.ResetToClean();
    TestFalse(TEXT("no description"), D.HasDescription());
    TestEqual(TEXT("recognition == 0"), D.Recognition(), 0.0, Eps);
    return true;
}

// 11. test_disguised_evades_faster_than_recognized (cross-system; reuses FWantedEvasion)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDisguiseEvadesFasterTest,
    "GTC.Systems.Disguise.DisguisedEvadesFasterThanRecognized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDisguiseEvadesFasterTest::RunTest(const FString& Parameters)
{
    FDisguise Disguised;
    Disguised.LogSighting();
    Disguised.SetAppearance(TEXT("outfit"), TEXT("tux"));
    Disguised.SetAppearance(TEXT("mask"), TEXT("ski"));
    Disguised.SetAppearance(TEXT("vehicle"), TEXT("taxi"));
    Disguised.SetAppearance(TEXT("hair"), TEXT("blonde"));  // recognition 0 -> speedup 3.0
    FDisguise Recognized;
    Recognized.LogSighting();  // recognition 1 -> speedup 1.0

    FWantedEvasion EvDisg(10.0);
    FWantedEvasion EvRecog(10.0);
    EvDisg.NotifyCrime();
    EvRecog.NotifyCrime();
    for (int32 i = 0; i < 6; ++i)
    {
        EvDisg.Update(false, 1.0 * Disguised.EvasionSpeedup());   // 6 * 3 = 18 > 10
        EvRecog.Update(false, 1.0 * Recognized.EvasionSpeedup()); // 6 * 1 = 6 < 10
    }
    TestTrue(TEXT("disguised is cold"), EvDisg.IsCold());
    TestTrue(TEXT("recognized still searching"), EvRecog.IsSearching());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
