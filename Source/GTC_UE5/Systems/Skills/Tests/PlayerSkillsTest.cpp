// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PlayerSkills.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_player_skills.gd. Gains use a diminishing-returns float curve,
// so level/gain assertions use Eps; tier/clamp/count assertions are exact.

namespace
{
    // Mirror of the oracle's TWO := [{"id":"a"},{"id":"b"}].
    TArray<FSkillDef> MakeTwoSkills()
    {
        return { FSkillDef(TEXT("a"), 1.0), FSkillDef(TEXT("b"), 1.0) };
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsDefaultLoadedTest,
    "GTC.Systems.Skills.PlayerSkills.DefaultSkillsLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsDefaultLoadedTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    TestEqual(TEXT("count == 7"), S.SkillCount(), 7);
    TestTrue(TEXT("has driving"), S.HasSkill(TEXT("driving")));
    TestTrue(TEXT("has shooting"), S.HasSkill(TEXT("shooting")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsListOrderTest,
    "GTC.Systems.Skills.PlayerSkills.SkillsListOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsListOrderTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    const TArray<FString> List = S.Skills();
    TestEqual(TEXT("first == driving"), List[0], FString(TEXT("driving")));
    TestTrue(TEXT("has flying"), List.Contains(TEXT("flying")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsFreshIsZeroTest,
    "GTC.Systems.Skills.PlayerSkills.FreshSkillIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsFreshIsZeroTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    TestEqual(TEXT("driving level == 0"), S.Level(TEXT("driving")), 0.0, Eps);
    TestEqual(TEXT("driving tier novice"), S.Tier(TEXT("driving")), FString(TEXT("novice")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsUnknownNeutralTest,
    "GTC.Systems.Skills.PlayerSkills.UnknownSkillIsNeutral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsUnknownNeutralTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    TestEqual(TEXT("nope level == 0"), S.Level(TEXT("nope")), 0.0, Eps);
    TestEqual(TEXT("nope tier empty"), S.Tier(TEXT("nope")), FString());
    TestEqual(TEXT("nope bonus == 0"), S.Bonus(TEXT("nope")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsMalformedDroppedTest,
    "GTC.Systems.Skills.PlayerSkills.MalformedSkillsDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsMalformedDroppedTest::RunTest(const FString& Parameters)
{
    // Typed-port note: the reference {"rate": 1.0} (missing id) row is unrepresentable with a
    // typed FSkillDef (id defaults to ""), so it folds into the empty-id drop path, which
    // is already covered. Empty-id, zero-rate and duplicate-id rows cover the drop path.
    const FPlayerSkills S({
        FSkillDef(TEXT("ok"), 1.0),
        FSkillDef(TEXT(""), 1.0),
        FSkillDef(TEXT("zero_rate"), 0.0),
        FSkillDef(TEXT("ok"), 2.0),  // duplicate id dropped
    });
    TestEqual(TEXT("count == 1"), S.SkillCount(), 1);
    TestTrue(TEXT("has ok"), S.HasSkill(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsTrainIncreasesTest,
    "GTC.Systems.Skills.PlayerSkills.TrainIncreasesLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsTrainIncreasesTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    const double Gain = S.Train(TEXT("driving"), 10.0);  // 10 * 1.0 * (1 - 0) = 10
    TestEqual(TEXT("gain == 10"), Gain, 10.0, Eps);
    TestEqual(TEXT("level == 10"), S.Level(TEXT("driving")), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsDiminishingTest,
    "GTC.Systems.Skills.PlayerSkills.TrainDiminishingReturns",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsDiminishingTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    const double First = S.Train(TEXT("driving"), 10.0);   // gain 10 -> value 10
    const double Second = S.Train(TEXT("driving"), 10.0);  // gain 10*0.9 = 9 -> value 19
    TestTrue(TEXT("first > second"), First > Second);
    TestEqual(TEXT("level == 19"), S.Level(TEXT("driving")), 19.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsRespectsRateTest,
    "GTC.Systems.Skills.PlayerSkills.TrainRespectsRate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsRespectsRateTest::RunTest(const FString& Parameters)
{
    // Same effort, lower-rate skill gains less.
    FPlayerSkills S;
    const double Driving = S.Train(TEXT("driving"), 10.0);  // rate 1.0
    const double Flying = S.Train(TEXT("flying"), 10.0);    // rate 0.5
    TestTrue(TEXT("driving > flying"), Driving > Flying);
    TestEqual(TEXT("flying == 5"), Flying, 5.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsCapsAtMaxTest,
    "GTC.Systems.Skills.PlayerSkills.TrainCapsAtMax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsCapsAtMaxTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    S.Train(TEXT("driving"), 100000.0);
    TestEqual(TEXT("level == MAX"), S.Level(TEXT("driving")), FPlayerSkills::MaxSkill, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsAtMaxGainsNothingTest,
    "GTC.Systems.Skills.PlayerSkills.TrainAtMaxGainsNothing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsAtMaxGainsNothingTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    S.SetLevel(TEXT("driving"), FPlayerSkills::MaxSkill);
    TestEqual(TEXT("gain == 0"), S.Train(TEXT("driving"), 50.0), 0.0, Eps);
    TestEqual(TEXT("level == MAX"), S.Level(TEXT("driving")), FPlayerSkills::MaxSkill, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsRejectsBadInputTest,
    "GTC.Systems.Skills.PlayerSkills.TrainRejectsBadInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsRejectsBadInputTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    TestEqual(TEXT("unknown gain 0"), S.Train(TEXT("nope"), 10.0), 0.0, Eps);
    TestEqual(TEXT("zero effort gain 0"), S.Train(TEXT("driving"), 0.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsTierBandsTest,
    "GTC.Systems.Skills.PlayerSkills.TierBands",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsTierBandsTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    S.SetLevel(TEXT("driving"), 25.0);
    const FString Competent = S.Tier(TEXT("driving"));
    S.SetLevel(TEXT("driving"), 70.0);
    const FString Expert = S.Tier(TEXT("driving"));
    S.SetLevel(TEXT("driving"), 90.0);
    const FString Master = S.Tier(TEXT("driving"));
    TestEqual(TEXT("competent"), Competent, FString(TEXT("competent")));
    TestEqual(TEXT("expert"), Expert, FString(TEXT("expert")));
    TestEqual(TEXT("master"), Master, FString(TEXT("master")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsBonusNormalisedTest,
    "GTC.Systems.Skills.PlayerSkills.BonusIsNormalised",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsBonusNormalisedTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    S.SetLevel(TEXT("shooting"), 50.0);
    TestEqual(TEXT("bonus == 0.5"), S.Bonus(TEXT("shooting")), 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsSetLevelClampsTest,
    "GTC.Systems.Skills.PlayerSkills.SetLevelClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsSetLevelClampsTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    S.SetLevel(TEXT("driving"), 150.0);
    const double High = S.Level(TEXT("driving"));
    S.SetLevel(TEXT("driving"), -20.0);
    const double Low = S.Level(TEXT("driving"));
    TestEqual(TEXT("high == MAX"), High, FPlayerSkills::MaxSkill, Eps);
    TestEqual(TEXT("low == 0"), Low, 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsSetLevelUnknownNoopTest,
    "GTC.Systems.Skills.PlayerSkills.SetLevelUnknownIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsSetLevelUnknownNoopTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    S.SetLevel(TEXT("nope"), 50.0);
    TestFalse(TEXT("still no nope"), S.HasSkill(TEXT("nope")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsOverallMasteryTest,
    "GTC.Systems.Skills.PlayerSkills.OverallMastery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsOverallMasteryTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S(MakeTwoSkills());
    S.SetLevel(TEXT("a"), 50.0);
    S.SetLevel(TEXT("b"), 50.0);
    TestEqual(TEXT("mastery == 0.5"), S.OverallMastery(), 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsOverallMasteryEmptyTest,
    "GTC.Systems.Skills.PlayerSkills.OverallMasteryEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsOverallMasteryEmptyTest::RunTest(const FString& Parameters)
{
    const FPlayerSkills S({ FSkillDef(TEXT("only"), 1.0) });
    TestEqual(TEXT("mastery == 0"), S.OverallMastery(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsRoundTripTest,
    "GTC.Systems.Skills.PlayerSkills.ToDictAndLoadRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsRoundTripTest::RunTest(const FString& Parameters)
{
    FPlayerSkills S;
    S.Train(TEXT("driving"), 10.0);
    S.SetLevel(TEXT("shooting"), 33.0);
    const TArray<TPair<FString, double>> Saved = S.ToDict();
    FPlayerSkills Restored;
    Restored.LoadDict(Saved);
    TestEqual(TEXT("driving matches"), Restored.Level(TEXT("driving")), S.Level(TEXT("driving")), Eps);
    TestEqual(TEXT("shooting == 33"), Restored.Level(TEXT("shooting")), 33.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillsLoadIgnoresBadTest,
    "GTC.Systems.Skills.PlayerSkills.LoadIgnoresUnknownAndBadValues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSkillsLoadIgnoresBadTest::RunTest(const FString& Parameters)
{
    // Typed-port note: the reference "shooting": "bad" (non-number) row is unrepresentable in a
    // typed {id->double} map; the load_dict number-type guard drops it so shooting stays 0.
    // We mirror that observable outcome by omitting the unrepresentable bad value, and keep
    // the unknown-id drop ("nope") which IS representable.
    FPlayerSkills S;
    S.LoadDict({
        TPair<FString, double>(TEXT("driving"), 40.0),
        TPair<FString, double>(TEXT("nope"), 99.0),
    });
    TestEqual(TEXT("driving == 40"), S.Level(TEXT("driving")), 40.0, Eps);
    TestEqual(TEXT("shooting == 0"), S.Level(TEXT("shooting")), 0.0, Eps);
    TestFalse(TEXT("no nope"), S.HasSkill(TEXT("nope")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
