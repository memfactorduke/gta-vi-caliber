// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../HeistCrew.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_heist_crew.gd. Rolls use a seeded FRandomStream (seed-
// reproducible WITHIN UE5, not byte-identical to Godot); the oracle only pins
// determinism and certain/hopeless boundaries, which FRandomStream satisfies.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistStartsEmptyTest,
    "GTC.Systems.Economy.HeistCrew.StartsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistStartsEmptyTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew;
    TestEqual(TEXT("count 0"), Crew.MemberCount(), 0);
    TestEqual(TEXT("total_cut 0"), Crew.TotalCut(), 0.0, Eps);
    TestEqual(TEXT("crew_skill 0"), Crew.CrewSkill(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistAddSucceedsTest,
    "GTC.Systems.Economy.HeistCrew.AddMemberSucceeds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistAddSucceedsTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    const bool bOk = Crew.AddMember(TEXT("driver"), 0.7, 0.2);
    TestTrue(TEXT("ok"), bOk);
    TestEqual(TEXT("count 1"), Crew.MemberCount(), 1);
    const TArray<FString> Roles = Crew.Roles();
    TestEqual(TEXT("roles size"), Roles.Num(), 1);
    TestEqual(TEXT("driver"), Roles[0], FString(TEXT("driver")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistMaxThenFailsTest,
    "GTC.Systems.Economy.HeistCrew.AddUpToMaxThenFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistMaxThenFailsTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(2);
    const bool A = Crew.AddMember(TEXT("driver"), 0.5, 0.1);
    const bool B = Crew.AddMember(TEXT("hacker"), 0.5, 0.1);
    const bool C = Crew.AddMember(TEXT("gunman"), 0.5, 0.1);
    TestTrue(TEXT("a"), A);
    TestTrue(TEXT("b"), B);
    TestFalse(TEXT("c fails"), C);
    TestEqual(TEXT("count 2"), Crew.MemberCount(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistDuplicateRoleTest,
    "GTC.Systems.Economy.HeistCrew.DuplicateRoleRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistDuplicateRoleTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    Crew.AddMember(TEXT("driver"), 0.5, 0.1);
    const bool Dup = Crew.AddMember(TEXT("driver"), 0.9, 0.1);
    TestFalse(TEXT("dup fails"), Dup);
    TestEqual(TEXT("count 1"), Crew.MemberCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistEmptyRoleTest,
    "GTC.Systems.Economy.HeistCrew.EmptyRoleRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistEmptyRoleTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    TestFalse(TEXT("empty role fails"), Crew.AddMember(TEXT(""), 0.5, 0.1));
    TestEqual(TEXT("count 0"), Crew.MemberCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistCutOverTest,
    "GTC.Systems.Economy.HeistCrew.CutOverOneHundredPercentRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistCutOverTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    const bool A = Crew.AddMember(TEXT("driver"), 0.5, 0.6);
    const bool B = Crew.AddMember(TEXT("hacker"), 0.5, 0.5);
    TestTrue(TEXT("a"), A);
    TestFalse(TEXT("b fails"), B);
    TestEqual(TEXT("total_cut 0.6"), Crew.TotalCut(), 0.6, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistTotalCutTest,
    "GTC.Systems.Economy.HeistCrew.TotalCutSumsMembers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistTotalCutTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    Crew.AddMember(TEXT("driver"), 0.5, 0.2);
    Crew.AddMember(TEXT("hacker"), 0.5, 0.3);
    TestEqual(TEXT("total_cut 0.5"), Crew.TotalCut(), 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistPlayerShareTest,
    "GTC.Systems.Economy.HeistCrew.PlayerShareIsRemainder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistPlayerShareTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    Crew.AddMember(TEXT("driver"), 0.5, 0.2);
    Crew.AddMember(TEXT("hacker"), 0.5, 0.3);
    TestEqual(TEXT("player_share 0.5"), Crew.PlayerShare(), 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistShareFullEmptyTest,
    "GTC.Systems.Economy.HeistCrew.PlayerShareFullWhenEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistShareFullEmptyTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    TestEqual(TEXT("share 1.0"), Crew.PlayerShare(), 1.0, Eps);
    TestEqual(TEXT("payout 8000"), Crew.Payout(8000, true), 8000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistCrewSkillAvgTest,
    "GTC.Systems.Economy.HeistCrew.CrewSkillIsAverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistCrewSkillAvgTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    Crew.AddMember(TEXT("driver"), 0.4, 0.1);
    Crew.AddMember(TEXT("hacker"), 0.8, 0.1);
    TestEqual(TEXT("crew_skill 0.6"), Crew.CrewSkill(), 0.6, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistCrewSkillZeroTest,
    "GTC.Systems.Economy.HeistCrew.CrewSkillZeroWhenEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistCrewSkillZeroTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    TestEqual(TEXT("crew_skill 0"), Crew.CrewSkill(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistSuccessProVsEmptyTest,
    "GTC.Systems.Economy.HeistCrew.SuccessChanceProVsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistSuccessProVsEmptyTest::RunTest(const FString& Parameters)
{
    HeistCrew Pro(1);
    Pro.AddMember(TEXT("ace"), 1.0, 0.2);
    HeistCrew Empty(1);
    TestEqual(TEXT("pro 0.85"), Pro.SuccessChance(0.5), 0.85, Eps);
    TestEqual(TEXT("empty 0.25"), Empty.SuccessChance(0.5), 0.25, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistSuccessRisesTest,
    "GTC.Systems.Economy.HeistCrew.SuccessChanceRisesWithSkill",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistSuccessRisesTest::RunTest(const FString& Parameters)
{
    HeistCrew Weak(1);
    Weak.AddMember(TEXT("rookie"), 0.2, 0.1);
    HeistCrew Strong(1);
    Strong.AddMember(TEXT("ace"), 0.9, 0.1);
    TestTrue(TEXT("strong > weak"), Strong.SuccessChance(0.5) > Weak.SuccessChance(0.5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistSuccessFallsTest,
    "GTC.Systems.Economy.HeistCrew.SuccessChanceFallsWithDifficulty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistSuccessFallsTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(1);
    Crew.AddMember(TEXT("ace"), 0.6, 0.1);
    TestTrue(TEXT("0.3 < 0.8"), Crew.SuccessChance(0.3) < Crew.SuccessChance(0.8));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistSuccessClampedTest,
    "GTC.Systems.Economy.HeistCrew.SuccessChanceClamped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistSuccessClampedTest::RunTest(const FString& Parameters)
{
    HeistCrew Pro(1);
    Pro.AddMember(TEXT("ace"), 1.0, 0.1);
    HeistCrew Empty(1);
    TestEqual(TEXT("pro 1.0"), Pro.SuccessChance(1.0), 1.0, Eps);
    TestEqual(TEXT("empty 0.0"), Empty.SuccessChance(0.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistAttemptDetTest,
    "GTC.Systems.Economy.HeistCrew.AttemptDeterministicSameSeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistAttemptDetTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(1);
    Crew.AddMember(TEXT("ace"), 0.5, 0.2);
    FRandomStream RngA = HeistCrew::MakeRng(42);
    FRandomStream RngB = HeistCrew::MakeRng(42);
    const bool A = Crew.Attempt(0.5, RngA);
    const bool B = Crew.Attempt(0.5, RngB);
    TestEqual(TEXT("a == b"), A, B);
    TestFalse(TEXT("no-rng never succeeds"), Crew.AttemptNoRng(0.5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistAttemptCertainTest,
    "GTC.Systems.Economy.HeistCrew.AttemptNearCertainSucceeds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistAttemptCertainTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(1);
    Crew.AddMember(TEXT("ace"), 1.0, 0.1);
    FRandomStream Rng = HeistCrew::MakeRng(7);
    TestTrue(TEXT("succeeds"), Crew.Attempt(1.0, Rng));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistAttemptHopelessTest,
    "GTC.Systems.Economy.HeistCrew.AttemptHopelessFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistAttemptHopelessTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(1);
    FRandomStream Rng = HeistCrew::MakeRng(7);
    TestFalse(TEXT("fails"), Crew.Attempt(0.0, Rng));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistPayoutSuccessTest,
    "GTC.Systems.Economy.HeistCrew.PayoutOnSuccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistPayoutSuccessTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(2);
    Crew.AddMember(TEXT("driver"), 0.5, 0.2);
    Crew.AddMember(TEXT("hacker"), 0.5, 0.3);
    TestEqual(TEXT("payout 5000"), Crew.Payout(10000, true), 5000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistPayoutFailTest,
    "GTC.Systems.Economy.HeistCrew.PayoutZeroOnFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistPayoutFailTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(2);
    Crew.AddMember(TEXT("driver"), 0.5, 0.2);
    TestEqual(TEXT("payout 0"), Crew.Payout(10000, false), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistPayoutZeroTakeTest,
    "GTC.Systems.Economy.HeistCrew.PayoutNonNegativeAndZeroTake",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistPayoutZeroTakeTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(1);
    Crew.AddMember(TEXT("driver"), 0.5, 0.2);
    TestEqual(TEXT("zero take 0"), Crew.Payout(0, true), 0);
    TestEqual(TEXT("neg take 0"), Crew.Payout(-100, true), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistExpectedPayoutTest,
    "GTC.Systems.Economy.HeistCrew.ExpectedPayoutMatchesHandCalc",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistExpectedPayoutTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(1);
    Crew.AddMember(TEXT("ace"), 1.0, 0.2);
    TestEqual(TEXT("expected 6800"), Crew.ExpectedPayout(10000, 0.5), 6800.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistExpectedZeroTakeTest,
    "GTC.Systems.Economy.HeistCrew.ExpectedPayoutZeroTake",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistExpectedZeroTakeTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(1);
    Crew.AddMember(TEXT("ace"), 1.0, 0.2);
    TestEqual(TEXT("expected 0"), Crew.ExpectedPayout(0, 0.5), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistRemoveTest,
    "GTC.Systems.Economy.HeistCrew.RemoveMember",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistRemoveTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    Crew.AddMember(TEXT("driver"), 0.5, 0.2);
    Crew.AddMember(TEXT("hacker"), 0.5, 0.3);
    const bool Removed = Crew.RemoveMember(TEXT("driver"));
    TestTrue(TEXT("removed"), Removed);
    TestEqual(TEXT("count 1"), Crew.MemberCount(), 1);
    const TArray<FString> Roles = Crew.Roles();
    TestEqual(TEXT("roles size"), Roles.Num(), 1);
    TestEqual(TEXT("hacker"), Roles[0], FString(TEXT("hacker")));
    TestEqual(TEXT("total_cut 0.3"), Crew.TotalCut(), 0.3, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHeistRemoveMissingTest,
    "GTC.Systems.Economy.HeistCrew.RemoveMissingMemberReturnsFalse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeistRemoveMissingTest::RunTest(const FString& Parameters)
{
    HeistCrew Crew(3);
    Crew.AddMember(TEXT("driver"), 0.5, 0.2);
    TestFalse(TEXT("remove ghost false"), Crew.RemoveMember(TEXT("ghost")));
    TestEqual(TEXT("count 1"), Crew.MemberCount(), 1);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
