// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StuntScore.h"
#include "../../../Systems/Progression/PlayerProgression.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_stunt_score.gd. Scores are integer; multiplier uses Eps.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntFreshEmptyTest,
    "GTC.Missions.StuntScore.StuntScore.FreshIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntFreshEmptyTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    TestFalse(TEXT("no combo"), S.HasCombo());
    TestEqual(TEXT("combo_count == 0"), S.ComboCount(), 0);
    TestEqual(TEXT("multiplier == 1.0"), S.Multiplier(), 1.0, Eps);
    TestEqual(TEXT("pending == 0"), S.PendingScore(), 0);
    TestEqual(TEXT("total == 0"), S.TotalScore(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntTrickKindsTest,
    "GTC.Missions.StuntScore.StuntScore.TrickKindsPresent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntTrickKindsTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    const TArray<FString> Kinds = S.TrickKinds();
    TestTrue(TEXT("has flip"), Kinds.Contains(TEXT("flip")));
    TestTrue(TEXT("has near_miss"), Kinds.Contains(TEXT("near_miss")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntAddScoresTest,
    "GTC.Missions.StuntScore.StuntScore.AddTrickScoresPoints",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntAddScoresTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    const int32 Pts = S.AddTrick(TEXT("jump"), 2.0);  // 50 * 2 = 100
    TestEqual(TEXT("pts == 100"), Pts, 100);
    TestEqual(TEXT("combo_count == 1"), S.ComboCount(), 1);
    TestEqual(TEXT("pending == 100"), S.PendingScore(), 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntRejectsBadInputTest,
    "GTC.Missions.StuntScore.StuntScore.AddTrickRejectsBadInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntRejectsBadInputTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    TestEqual(TEXT("unknown kind 0"), S.AddTrick(TEXT("moonwalk"), 1.0), 0);
    TestEqual(TEXT("zero magnitude 0"), S.AddTrick(TEXT("jump"), 0.0), 0);
    TestFalse(TEXT("no combo"), S.HasCombo());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntMultiplierGrowsTest,
    "GTC.Missions.StuntScore.StuntScore.MultiplierGrowsWithCombo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntMultiplierGrowsTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    S.AddTrick(TEXT("jump"), 2.0);  // count 1 -> mult 1.0
    const double M1 = S.Multiplier();
    S.AddTrick(TEXT("flip"), 1.0);  // count 2 -> mult 1.5
    TestEqual(TEXT("m1 == 1.0"), M1, 1.0, Eps);
    TestEqual(TEXT("mult == 1.5"), S.Multiplier(), 1.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntPendingMultiplierTest,
    "GTC.Missions.StuntScore.StuntScore.PendingAppliesMultiplier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntPendingMultiplierTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    S.AddTrick(TEXT("jump"), 2.0);  // 100
    S.AddTrick(TEXT("flip"), 1.0);  // +250 -> 350 raw, mult 1.5
    TestEqual(TEXT("pending == 525"), S.PendingScore(), 525);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntMultiplierCapsTest,
    "GTC.Missions.StuntScore.StuntScore.MultiplierCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntMultiplierCapsTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    for (int32 I = 0; I < 20; ++I)
    {
        S.AddTrick(TEXT("wheelie"), 1.0);
    }
    TestEqual(TEXT("mult == MAX_MULT"), S.Multiplier(), FStuntScore::MaxMult, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntBankResetsTest,
    "GTC.Missions.StuntScore.StuntScore.BankBanksAndResets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntBankResetsTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    S.AddTrick(TEXT("jump"), 2.0);
    S.AddTrick(TEXT("flip"), 1.0);  // pending 525
    const int32 Banked = S.Bank();
    TestEqual(TEXT("banked == 525"), Banked, 525);
    TestEqual(TEXT("total == 525"), S.TotalScore(), 525);
    TestFalse(TEXT("no combo"), S.HasCombo());
    TestEqual(TEXT("pending == 0"), S.PendingScore(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntWipeForfeitsTest,
    "GTC.Missions.StuntScore.StuntScore.WipeForfeitsAndResets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntWipeForfeitsTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    S.AddTrick(TEXT("jump"), 2.0);  // pending 100
    const int32 Lost = S.Wipe();
    TestEqual(TEXT("lost == 100"), Lost, 100);
    TestEqual(TEXT("total == 0"), S.TotalScore(), 0);
    TestFalse(TEXT("no combo"), S.HasCombo());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntTotalBestAccumulateTest,
    "GTC.Missions.StuntScore.StuntScore.TotalAndBestAccumulate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntTotalBestAccumulateTest::RunTest(const FString& Parameters)
{
    FStuntScore S;
    S.AddTrick(TEXT("flip"), 1.0);  // 250
    S.Bank();
    S.AddTrick(TEXT("jump"), 2.0);  // 100
    S.AddTrick(TEXT("spin"), 1.0);  // +150 -> 250 raw, mult 1.5 -> 375
    S.Bank();
    TestEqual(TEXT("total == 625"), S.TotalScore(), 625);
    TestEqual(TEXT("best == 375"), S.BestCombo(), 375);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntRewardHelpersTest,
    "GTC.Missions.StuntScore.StuntScore.RewardHelpers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntRewardHelpersTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("cash_for(500) == 500"), FStuntScore::CashFor(500), 500);
    TestEqual(TEXT("respect_for(500) == 50"), FStuntScore::RespectFor(500), 50);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStuntBankedGrantsRespectTest,
    "GTC.Missions.StuntScore.StuntScore.BankedComboGrantsProgressionRespect",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStuntBankedGrantsRespectTest::RunTest(const FString& Parameters)
{
    // Composition: a clean landing's respect payout feeds PlayerProgression.
    FStuntScore S;
    S.AddTrick(TEXT("flip"), 2.0);  // 500
    S.AddTrick(TEXT("spin"), 2.0);  // +300 -> 800 raw, mult 1.5 -> 1200
    const int32 Banked = S.Bank();
    FPlayerProgression Prog;
    Prog.AddXp(FStuntScore::RespectFor(Banked));
    TestEqual(TEXT("banked == 1200"), Banked, 1200);
    TestEqual(TEXT("total_xp == 120"), Prog.TotalXp(), 120);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
