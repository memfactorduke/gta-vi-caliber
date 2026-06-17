// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionChain.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_mission_chain.gd.

using GtcTest::Eps;

namespace MissionChainTestNS
{
// Mirrors the oracle's _sample(): three missions with id+title.
TArray<MissionChain::FMissionDef> Sample()
{
    return {
        MissionChain::FMissionDef(TEXT("intro"), TEXT("Welcome")),
        MissionChain::FMissionDef(TEXT("heist"), TEXT("The Job")),
        MissionChain::FMissionDef(TEXT("escape"), TEXT("Get Out")),
    };
}
} // namespace MissionChainTestNS
using namespace MissionChainTestNS;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainCountMatchesTest,
    "GTC.Missions.Core.MissionChain.CountMatchesDefinitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainCountMatchesTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("count == 3"), MissionChain(Sample()).Count(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainEmptyIsCompleteTest,
    "GTC.Missions.Core.MissionChain.EmptyChainIsComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainEmptyIsCompleteTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(TArray<MissionChain::FMissionDef>{});
    TestTrue(TEXT("is_campaign_complete"), Chain.IsCampaignComplete());
    TestEqual(TEXT("progress == 1.0"), Chain.Progress(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainStartsOnFirstTest,
    "GTC.Missions.Core.MissionChain.StartsOnFirstMission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainStartsOnFirstTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    TestEqual(TEXT("active_index == 0"), Chain.ActiveIndex(), 0);
    TestEqual(TEXT("current_id == intro"), Chain.CurrentId(), FString(TEXT("intro")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainCompleteCurrentAdvancesTest,
    "GTC.Missions.Core.MissionChain.CompleteCurrentAdvances",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainCompleteCurrentAdvancesTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    Chain.CompleteCurrent();
    TestEqual(TEXT("active_index == 1"), Chain.ActiveIndex(), 1);
    TestEqual(TEXT("current_id == heist"), Chain.CurrentId(), FString(TEXT("heist")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainCompletingAllFinishesTest,
    "GTC.Missions.Core.MissionChain.CompletingAllFinishesCampaign",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainCompletingAllFinishesTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    Chain.CompleteCurrent();
    Chain.CompleteCurrent();
    Chain.CompleteCurrent();
    TestTrue(TEXT("is_campaign_complete"), Chain.IsCampaignComplete());
    TestTrue(TEXT("current().is_empty()"), Chain.Current().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainOvercompletingClampsTest,
    "GTC.Missions.Core.MissionChain.OvercompletingDoesNotRunPastEnd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainOvercompletingClampsTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    for (int32 i = 0; i < 10; ++i)
    {
        Chain.CompleteCurrent();
    }
    TestEqual(TEXT("active_index == 3"), Chain.ActiveIndex(), 3);
    TestTrue(TEXT("is_campaign_complete"), Chain.IsCampaignComplete());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainProgressRampsTest,
    "GTC.Missions.Core.MissionChain.ProgressRamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainProgressRampsTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    const double P0 = Chain.Progress();
    Chain.CompleteCurrent();
    const double P1 = Chain.Progress();
    TestEqual(TEXT("p0 == 0.0"), P0, 0.0, Eps);
    TestEqual(TEXT("p1 == 1/3"), P1, 1.0 / 3.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainRemainingCompletedTest,
    "GTC.Missions.Core.MissionChain.RemainingAndCompleted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainRemainingCompletedTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    Chain.CompleteCurrent();
    TestEqual(TEXT("remaining == 2"), Chain.Remaining(), 2);
    TestEqual(TEXT("completed == 1"), Chain.Completed(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainCurrentIdEmptyWhenDoneTest,
    "GTC.Missions.Core.MissionChain.CurrentIdEmptyWhenDone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainCurrentIdEmptyWhenDoneTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(TArray<MissionChain::FMissionDef>{});
    TestEqual(TEXT("current_id == empty"), Chain.CurrentId(), FString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainResetReturnsToFirstTest,
    "GTC.Missions.Core.MissionChain.ResetReturnsToFirst",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainResetReturnsToFirstTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    Chain.CompleteCurrent();
    Chain.CompleteCurrent();
    Chain.Reset();
    TestEqual(TEXT("active_index == 0"), Chain.ActiveIndex(), 0);
    TestEqual(TEXT("current_id == intro"), Chain.CurrentId(), FString(TEXT("intro")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainCurrentCarriesFieldsTest,
    "GTC.Missions.Core.MissionChain.CurrentCarriesDefinitionFields",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainCurrentCarriesFieldsTest::RunTest(const FString& Parameters)
{
    MissionChain Chain(Sample());
    const MissionChain::FMissionDef Mission = Chain.Current();
    TestEqual(TEXT("title == Welcome"), Mission.Title, FString(TEXT("Welcome")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionChainInitCopiesInputTest,
    "GTC.Missions.Core.MissionChain.InitCopiesInputArray",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionChainInitCopiesInputTest::RunTest(const FString& Parameters)
{
    TArray<MissionChain::FMissionDef> Defs = Sample();
    MissionChain Chain(Defs);
    Defs.Empty();
    TestEqual(TEXT("count == 3 after source cleared"), Chain.Count(), 3);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
