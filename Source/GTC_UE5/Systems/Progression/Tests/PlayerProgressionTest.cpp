// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PlayerProgression.h"
#include "../ProgressionTracker.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

namespace
{
    // UGameInstanceSubsystem requires a UGameInstance outer (ClassWithin); a transient
    // one suffices for these owned-model behavior assertions.
    UProgressionTracker* MakeProgressionTrackerForParityTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UProgressionTracker>(GameInstance);
    }
}

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_player_progression.gd. Curve: leaving level L costs 100*L;
// cumulative reach(L) is the triangular sum 100*(L-1)*L/2.
// The two final tracker tests exercise the UProgressionTracker subsystem's owned
// FPlayerProgression directly (the subsystem owns and drives the pure model).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgStartsAtLevelOneTest,
    "GTC.Systems.Progression.PlayerProgression.StartsAtLevelOneZeroXp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgStartsAtLevelOneTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    TestEqual(TEXT("level == 1"), P.Level(), 1);
    TestEqual(TEXT("xp == 0"), P.Xp(), 0);
    TestEqual(TEXT("total_xp == 0"), P.TotalXp(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgXpBelowThresholdTest,
    "GTC.Systems.Progression.PlayerProgression.XpBelowThresholdRaisesProgressNotLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgXpBelowThresholdTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(60);
    TestEqual(TEXT("level == 1"), P.Level(), 1);
    TestEqual(TEXT("xp_into_level == 60"), P.XpIntoLevel(), 60);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgTotalXpTracksTest,
    "GTC.Systems.Progression.PlayerProgression.TotalXpTracksLifetime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgTotalXpTracksTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(60);
    P.AddXp(20);
    TestEqual(TEXT("total_xp == 80"), P.TotalXp(), 80);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgCrossThresholdLeftoverTest,
    "GTC.Systems.Progression.PlayerProgression.CrossingThresholdLevelsUpWithLeftover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgCrossThresholdLeftoverTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    // Leaving L1 costs 100; 150 -> level 2 with 50 carried in.
    P.AddXp(150);
    TestEqual(TEXT("level == 2"), P.Level(), 2);
    TestEqual(TEXT("xp_into_level == 50"), P.XpIntoLevel(), 50);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgExactThresholdTest,
    "GTC.Systems.Progression.PlayerProgression.ExactThresholdLevelsUpClean",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgExactThresholdTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(100);
    TestEqual(TEXT("level == 2"), P.Level(), 2);
    TestEqual(TEXT("xp_into_level == 0"), P.XpIntoLevel(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgBigPayoutMultiLevelTest,
    "GTC.Systems.Progression.PlayerProgression.BigPayoutMultiLevelsWithRemainder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgBigPayoutMultiLevelTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    // 700: -100 (L2) -200 (L3) -300 (L4) leaves 100 < 400.
    P.AddXp(700);
    TestEqual(TEXT("level == 4"), P.Level(), 4);
    TestEqual(TEXT("xp_into_level == 100"), P.XpIntoLevel(), 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgXpForNextCurveTest,
    "GTC.Systems.Progression.PlayerProgression.XpForNextFollowsCurve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgXpForNextCurveTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    const int32 At1 = P.XpForNext();
    P.AddXp(100);
    const int32 At2 = P.XpForNext();
    P.AddXp(200);
    const int32 At3 = P.XpForNext();
    TestEqual(TEXT("at1 == 100"), At1, 100);
    TestEqual(TEXT("at2 == 200"), At2, 200);
    TestEqual(TEXT("at3 == 300"), At3, 300);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgLevelProgressFractionTest,
    "GTC.Systems.Progression.PlayerProgression.LevelProgressFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgLevelProgressFractionTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(25);
    // 25 of the 100 needed to leave level 1.
    TestEqual(TEXT("level_progress == 0.25"), P.LevelProgress(), 0.25, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgLevelProgressUnitRangeTest,
    "GTC.Systems.Progression.PlayerProgression.LevelProgressInUnitRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgLevelProgressUnitRangeTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(99);
    const double Lo = P.LevelProgress();
    P.Reset();
    P.AddXp(1);
    const double Hi = P.LevelProgress();
    TestTrue(TEXT("lo in [0,1]"), Lo >= 0.0 && Lo <= 1.0);
    TestTrue(TEXT("hi in [0,1]"), Hi >= 0.0 && Hi <= 1.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgXpToReachCurveTest,
    "GTC.Systems.Progression.PlayerProgression.XpToReachCurve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgXpToReachCurveTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("reach(1) == 0"), FPlayerProgression::XpToReach(1), 0);
    TestEqual(TEXT("reach(2) == 100"), FPlayerProgression::XpToReach(2), 100);
    TestEqual(TEXT("reach(3) == 300"), FPlayerProgression::XpToReach(3), 300);
    TestEqual(TEXT("reach(4) == 600"), FPlayerProgression::XpToReach(4), 600);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgXpToReachMatchesAddTest,
    "GTC.Systems.Progression.PlayerProgression.XpToReachMatchesAddXp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgXpToReachMatchesAddTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    // Exactly enough cumulative respect to sit at the start of level 5.
    P.AddXp(FPlayerProgression::XpToReach(5));
    TestEqual(TEXT("level == 5"), P.Level(), 5);
    TestEqual(TEXT("xp_into_level == 0"), P.XpIntoLevel(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgUnlocksAtFeaturesTest,
    "GTC.Systems.Progression.PlayerProgression.UnlocksAtReturnsFeatures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgUnlocksAtFeaturesTest::RunTest(const FString& Parameters)
{
    const TArray<FString> At5 = FPlayerProgression::UnlocksAt(5);
    TestTrue(TEXT("has sports_car"), At5.Contains(TEXT("sports_car")));
    TestTrue(TEXT("has ammo_discount"), At5.Contains(TEXT("ammo_discount")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgUnlocksAtEmptyTest,
    "GTC.Systems.Progression.PlayerProgression.UnlocksAtEmptyWhenNone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgUnlocksAtEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("unlocks_at(3) empty"), FPlayerProgression::UnlocksAt(3).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgIsUnlockedLockedBelowTest,
    "GTC.Systems.Progression.PlayerProgression.IsUnlockedLockedBelowGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgIsUnlockedLockedBelowTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    // Level 1: pistol_slot (gate 2) not yet earned.
    TestFalse(TEXT("pistol_slot locked"), P.IsUnlocked(TEXT("pistol_slot")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgIsUnlockedFlipsTest,
    "GTC.Systems.Progression.PlayerProgression.IsUnlockedFlipsAtGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgIsUnlockedFlipsTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(FPlayerProgression::XpToReach(2));
    TestTrue(TEXT("pistol_slot unlocked"), P.IsUnlocked(TEXT("pistol_slot")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgIsUnlockedCumulativeTest,
    "GTC.Systems.Progression.PlayerProgression.IsUnlockedCumulative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgIsUnlockedCumulativeTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(FPlayerProgression::XpToReach(10));
    // Higher level still keeps lower-gate unlocks.
    TestTrue(TEXT("pistol_slot"), P.IsUnlocked(TEXT("pistol_slot")));
    TestTrue(TEXT("smg_slot"), P.IsUnlocked(TEXT("smg_slot")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgMaxLevelCapsTest,
    "GTC.Systems.Progression.PlayerProgression.MaxLevelCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgMaxLevelCapsTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(10000000);
    TestEqual(TEXT("level == MAX"), P.Level(), FPlayerProgression::MaxLevel);
    TestTrue(TEXT("is_max_level"), P.IsMaxLevel());
    TestEqual(TEXT("xp_for_next == 0"), P.XpForNext(), 0);
    TestEqual(TEXT("level_progress == 1.0"), P.LevelProgress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgXpAtMaxNoOverflowTest,
    "GTC.Systems.Progression.PlayerProgression.XpAtMaxDoesNotOverflowLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgXpAtMaxNoOverflowTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(10000000);
    const int32 Before = P.Level();
    P.AddXp(500);
    // Surplus respect at cap is dropped; level holds, into-level stays 0.
    TestEqual(TEXT("level holds"), P.Level(), Before);
    TestEqual(TEXT("xp_into_level == 0"), P.XpIntoLevel(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgNegativeXpIgnoredTest,
    "GTC.Systems.Progression.PlayerProgression.NegativeXpIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgNegativeXpIgnoredTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(-500);
    TestEqual(TEXT("level == 1"), P.Level(), 1);
    TestEqual(TEXT("xp_into_level == 0"), P.XpIntoLevel(), 0);
    TestEqual(TEXT("total_xp == 0"), P.TotalXp(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgResetRestoresStartTest,
    "GTC.Systems.Progression.PlayerProgression.ResetRestoresStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgResetRestoresStartTest::RunTest(const FString& Parameters)
{
    FPlayerProgression P;
    P.AddXp(5000);
    P.Reset();
    TestEqual(TEXT("level == 1"), P.Level(), 1);
    TestEqual(TEXT("xp_into_level == 0"), P.XpIntoLevel(), 0);
    TestEqual(TEXT("total_xp == 0"), P.TotalXp(), 0);
    return true;
}

// --- ProgressionTracker subsystem behavior (owns + drives FPlayerProgression) -----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgTrackerSaveRoundTripTest,
    "GTC.Systems.Progression.ProgressionTracker.SaveRoundTripReplaysCurve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgTrackerSaveRoundTripTest::RunTest(const FString& Parameters)
{
    // The subsystem persists lifetime XP only; restoring replays it through the curve,
    // reconstructing level and within-level progress. Exercise its owned model directly
    // (no GameInstance needed): MakeProgressionTrackerForParityTest() owns a fresh model.
    UProgressionTracker* Tracker = MakeProgressionTrackerForParityTest();
    Tracker->Restore(1730);
    FPlayerProgression Reference;
    Reference.AddXp(1730);
    TestEqual(TEXT("total_xp == 1730"), Tracker->TotalXp(), 1730);
    TestEqual(TEXT("level matches reference"), Tracker->Level(), Reference.Level());
    TestTrue(TEXT("level_progress matches"),
        FMath::Abs(Tracker->LevelProgress() - Reference.LevelProgress()) < 0.0001);
    TestEqual(TEXT("serialize == 1730"), Tracker->SerializeState(), 1730);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgTrackerRestoreGarbageTest,
    "GTC.Systems.Progression.ProgressionTracker.RestoreGarbageResetsClean",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgTrackerRestoreGarbageTest::RunTest(const FString& Parameters)
{
    UProgressionTracker* Tracker = MakeProgressionTrackerForParityTest();
    Tracker->Restore(990);
    Tracker->RestoreGarbage();  // Godot restore({"total_xp": "junk"}) -> number_or 0.
    TestEqual(TEXT("total_xp == 0"), Tracker->TotalXp(), 0);
    TestEqual(TEXT("level == 1"), Tracker->Level(), 1);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
