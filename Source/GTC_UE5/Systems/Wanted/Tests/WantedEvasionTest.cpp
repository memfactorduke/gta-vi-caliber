// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WantedEvasion.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_wanted_evasion.gd (19 funcs). State checks are exact; the
// timer/progress are float curves checked with Eps. Compound `a and b` oracle returns
// are split into separate Test* calls.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionStartsVisibleTest,
    "GTC.Systems.Wanted.WantedEvasion.StartsVisibleFullTimer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionStartsVisibleTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(12.0);
    TestTrue(TEXT("is visible"), E.IsVisible());
    TestTrue(TEXT("state == Visible"), E.State() == FWantedEvasion::EState::Visible);
    TestEqual(TEXT("time_left == 12"), E.TimeLeft(), 12.0, Eps);
    TestEqual(TEXT("progress == 0"), E.SearchProgress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionFirstUnseenSearchingTest,
    "GTC.Systems.Wanted.WantedEvasion.FirstUnseenEntersSearching",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionFirstUnseenSearchingTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 1.0);
    TestTrue(TEXT("is searching"), E.IsSearching());
    TestFalse(TEXT("not visible"), E.IsVisible());
    TestFalse(TEXT("not cold"), E.IsCold());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionCountdownDecrementsTest,
    "GTC.Systems.Wanted.WantedEvasion.CountdownDecrementsWithDelta",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionCountdownDecrementsTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 2.0);
    TestEqual(TEXT("time_left == 8"), E.TimeLeft(), 8.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionReachesColdTest,
    "GTC.Systems.Wanted.WantedEvasion.ReachesColdAfterFullDuration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionReachesColdTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    for (int32 i = 0; i < 5; ++i)
    {
        E.Update(false, 2.0);
    }
    TestTrue(TEXT("is cold"), E.IsCold());
    TestEqual(TEXT("time_left == 0"), E.TimeLeft(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionColdProgressOneTest,
    "GTC.Systems.Wanted.WantedEvasion.ColdProgressIsOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionColdProgressOneTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    for (int32 i = 0; i < 5; ++i)
    {
        E.Update(false, 2.0);
    }
    TestEqual(TEXT("progress == 1"), E.SearchProgress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionResightingResetsTest,
    "GTC.Systems.Wanted.WantedEvasion.ResightingResetsToVisibleAndRefills",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionResightingResetsTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 6.0);
    E.Update(true, 0.0);
    TestTrue(TEXT("is visible"), E.IsVisible());
    TestEqual(TEXT("time_left == 10"), E.TimeLeft(), 10.0, Eps);
    TestEqual(TEXT("progress == 0"), E.SearchProgress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionResightThenSearchTest,
    "GTC.Systems.Wanted.WantedEvasion.ResightingThenSearchingStartsFromFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionResightThenSearchTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 7.0);
    E.Update(true, 0.0);
    E.Update(false, 2.0);
    TestTrue(TEXT("is searching"), E.IsSearching());
    TestEqual(TEXT("time_left == 8"), E.TimeLeft(), 8.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionNotifyCrimeVisibleTest,
    "GTC.Systems.Wanted.WantedEvasion.NotifyCrimeForcesVisible",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionNotifyCrimeVisibleTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 5.0);
    E.NotifyCrime();
    TestTrue(TEXT("is visible"), E.IsVisible());
    TestEqual(TEXT("time_left == 10"), E.TimeLeft(), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionNotifyCrimeFromColdTest,
    "GTC.Systems.Wanted.WantedEvasion.NotifyCrimeFromColdReheats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionNotifyCrimeFromColdTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    for (int32 i = 0; i < 5; ++i)
    {
        E.Update(false, 2.0);
    }
    E.NotifyCrime();
    TestTrue(TEXT("is visible"), E.IsVisible());
    TestEqual(TEXT("time_left == 10"), E.TimeLeft(), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionResetVisibleTest,
    "GTC.Systems.Wanted.WantedEvasion.ResetForcesVisible",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionResetVisibleTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 4.0);
    E.Reset();
    TestTrue(TEXT("is visible"), E.IsVisible());
    TestEqual(TEXT("time_left == 10"), E.TimeLeft(), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionProgressZeroVisibleTest,
    "GTC.Systems.Wanted.WantedEvasion.ProgressZeroWhileVisible",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionProgressZeroVisibleTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    TestEqual(TEXT("progress == 0"), E.SearchProgress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionProgressRampsTest,
    "GTC.Systems.Wanted.WantedEvasion.ProgressRampsDuringSearch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionProgressRampsTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 5.0);
    TestEqual(TEXT("progress == 0.5"), E.SearchProgress(), 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionProgressMonotonicTest,
    "GTC.Systems.Wanted.WantedEvasion.ProgressMonotonicWithinSearch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionProgressMonotonicTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    double Last = 0.0;
    for (int32 i = 0; i < 4; ++i)
    {
        E.Update(false, 2.0);
        const double P = E.SearchProgress();
        TestTrue(TEXT("monotonic non-decreasing"), P >= Last);
        Last = P;
    }
    TestEqual(TEXT("last == 0.8"), Last, 0.8, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionTimeLeftClampLowTest,
    "GTC.Systems.Wanted.WantedEvasion.TimeLeftClampedLow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionTimeLeftClampLowTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 100.0);
    TestTrue(TEXT("is cold"), E.IsCold());
    TestEqual(TEXT("time_left == 0"), E.TimeLeft(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionTimeLeftClampHighTest,
    "GTC.Systems.Wanted.WantedEvasion.TimeLeftClampedHigh",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionTimeLeftClampHighTest::RunTest(const FString& Parameters)
{
    // A re-sight refill never exceeds the duration.
    FWantedEvasion E(10.0);
    E.Update(true, 5.0);
    TestEqual(TEXT("time_left == 10"), E.TimeLeft(), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionColdOnlyAtEndTest,
    "GTC.Systems.Wanted.WantedEvasion.IsColdOnlyAtEnd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionColdOnlyAtEndTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 9.0);
    const bool bNotYet = !E.IsCold();
    E.Update(false, 1.0);
    TestTrue(TEXT("not cold at 9"), bNotYet);
    TestTrue(TEXT("cold at 10"), E.IsCold());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionColdStaysColdTest,
    "GTC.Systems.Wanted.WantedEvasion.ColdStaysColdUntilReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionColdStaysColdTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    for (int32 i = 0; i < 5; ++i)
    {
        E.Update(false, 2.0);
    }
    E.Update(false, 2.0);
    E.Update(false, 2.0);
    TestTrue(TEXT("is cold"), E.IsCold());
    TestEqual(TEXT("time_left == 0"), E.TimeLeft(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionZeroNegativeDeltaTest,
    "GTC.Systems.Wanted.WantedEvasion.ZeroAndNegativeDeltaSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionZeroNegativeDeltaTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E(10.0);
    E.Update(false, 3.0);
    E.Update(false, 0.0);
    E.Update(false, -5.0);
    TestTrue(TEXT("is searching"), E.IsSearching());
    TestEqual(TEXT("time_left == 7"), E.TimeLeft(), 7.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedEvasionDefaultDurationTest,
    "GTC.Systems.Wanted.WantedEvasion.DefaultDurationIsTwelve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedEvasionDefaultDurationTest::RunTest(const FString& Parameters)
{
    FWantedEvasion E;
    TestEqual(TEXT("search_duration == 12"), E.SearchDuration, 12.0, Eps);
    TestEqual(TEXT("time_left == 12"), E.TimeLeft(), 12.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
