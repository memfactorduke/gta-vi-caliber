// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WantedSystem.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_wanted_system.gd (11 funcs). Heat/decay are a float curve, so
// heat assertions use Eps; star/response-unit counts are exact integers. Compound
// `a and b` oracle returns are split into separate TestTrue/TestEqual calls.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemStartsCleanTest,
    "GTC.Systems.Wanted.WantedSystem.StartsClean",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemStartsCleanTest::RunTest(const FString& Parameters)
{
    FWantedSystem W;
    TestEqual(TEXT("heat == 0"), W.Heat, 0.0, Eps);
    TestEqual(TEXT("stars == 0"), W.Stars(), 0);
    TestFalse(TEXT("not wanted"), W.IsWanted());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemCrimeRaisesHeatAndStarsTest,
    "GTC.Systems.Wanted.WantedSystem.CrimeRaisesHeatAndStars",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemCrimeRaisesHeatAndStarsTest::RunTest(const FString& Parameters)
{
    FWantedSystem W;
    W.AddCrime(3.5);
    TestTrue(TEXT("is wanted"), W.IsWanted());
    TestEqual(TEXT("stars == 2"), W.Stars(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemNegativeCrimeIgnoredTest,
    "GTC.Systems.Wanted.WantedSystem.NegativeCrimeIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemNegativeCrimeIgnoredTest::RunTest(const FString& Parameters)
{
    FWantedSystem W;
    W.AddCrime(-5.0);
    TestEqual(TEXT("heat == 0"), W.Heat, 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemHeatCappedTest,
    "GTC.Systems.Wanted.WantedSystem.HeatCapped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemHeatCappedTest::RunTest(const FString& Parameters)
{
    FWantedSystem W(0.4, 8.0);
    W.AddCrime(100.0);
    TestEqual(TEXT("heat == 8"), W.Heat, 8.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemDecayWhenNotCommittingTest,
    "GTC.Systems.Wanted.WantedSystem.DecayWhenNotCommitting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemDecayWhenNotCommittingTest::RunTest(const FString& Parameters)
{
    FWantedSystem W(1.0, 20.0);
    W.AddCrime(5.0);
    W.Tick(2.0, false);
    TestEqual(TEXT("heat == 3"), W.Heat, 3.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemNoDecayWhileCommittingTest,
    "GTC.Systems.Wanted.WantedSystem.NoDecayWhileCommitting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemNoDecayWhileCommittingTest::RunTest(const FString& Parameters)
{
    FWantedSystem W(1.0, 20.0);
    W.AddCrime(5.0);
    W.Tick(2.0, true);
    TestEqual(TEXT("heat == 5"), W.Heat, 5.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemHeatFloorsAtZeroTest,
    "GTC.Systems.Wanted.WantedSystem.HeatFloorsAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemHeatFloorsAtZeroTest::RunTest(const FString& Parameters)
{
    FWantedSystem W(1.0, 20.0);
    W.AddCrime(1.0);
    W.Tick(10.0, false);
    TestEqual(TEXT("heat == 0"), W.Heat, 0.0, Eps);
    TestFalse(TEXT("not wanted"), W.IsWanted());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemStarsForThresholdsTest,
    "GTC.Systems.Wanted.WantedSystem.StarsForThresholds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemStarsForThresholdsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("0.5 -> 0"), FWantedSystem::StarsFor(0.5), 0);
    TestEqual(TEXT("1.0 -> 1"), FWantedSystem::StarsFor(1.0), 1);
    TestEqual(TEXT("3.0 -> 2"), FWantedSystem::StarsFor(3.0), 2);
    TestEqual(TEXT("6.0 -> 3"), FWantedSystem::StarsFor(6.0), 3);
    TestEqual(TEXT("10.0 -> 4"), FWantedSystem::StarsFor(10.0), 4);
    TestEqual(TEXT("16.0 -> 5"), FWantedSystem::StarsFor(16.0), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemStarsCappedAtFiveTest,
    "GTC.Systems.Wanted.WantedSystem.StarsCappedAtFive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemStarsCappedAtFiveTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("999 -> 5"), FWantedSystem::StarsFor(999.0), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemResponseUnitsScaleTest,
    "GTC.Systems.Wanted.WantedSystem.ResponseUnitsScaleWithStars",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemResponseUnitsScaleTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("0 -> 0"), FWantedSystem::ResponseUnits(0), 0);
    TestEqual(TEXT("3 -> 3"), FWantedSystem::ResponseUnits(3), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSystemResponseUnitsClampedTest,
    "GTC.Systems.Wanted.WantedSystem.ResponseUnitsClamped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSystemResponseUnitsClampedTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("9 -> 5"), FWantedSystem::ResponseUnits(9), 5);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
