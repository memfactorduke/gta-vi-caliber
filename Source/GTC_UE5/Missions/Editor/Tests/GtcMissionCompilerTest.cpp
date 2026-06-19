// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GtcMissionCompiler.h"
#include "../GtcMissionDefinition.h"
#include "../../Core/MissionChain.h"
#include "../../Coordinators/MissionObjectiveDriver.h"
#include "../../../Tests/GtcTestTolerances.h"

namespace
{
    FGtcMissionDefinition MakeNarrative(const TCHAR* Id, const TCHAR* Title, int32 NumObjectives)
    {
        FGtcMissionDefinition D;
        D.Id = Id;
        D.Title = Title;
        D.ContentType = EGtcMissionContentType::MainMission;
        for (int32 i = 0; i < NumObjectives; ++i)
        {
            FGtcObjectiveDefinition O;
            O.Id = FString::Printf(TEXT("%s_o%d"), Id, i);
            O.Text = FText::FromString(FString::Printf(TEXT("Objective %d"), i));
            O.bHasWaypoint = true;
            O.Waypoint = FVector(100.0 * i, 0.0, 0.0);
            D.Objectives.Add(O);
        }
        return D;
    }
}

// 1) CompileMission carries the controller-relevant fields and gates bValid on objectives.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionCompilerCompileMissionTest,
    "GTC.Missions.Editor.Compiler.CompileMission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionCompilerCompileMissionTest::RunTest(const FString& Parameters)
{
    FGtcMissionDefinition D = MakeNarrative(TEXT("m1"), TEXT("First Blood"), 2);
    D.TimeLimit = 180.0;
    D.Objectives[1].bHasWaypoint = false;

    const FGtcCompiledMission C = GtcMissionCompiler::CompileMission(D);
    TestTrue(TEXT("valid"), C.bValid);
    TestEqual(TEXT("title"), C.Title, FString(TEXT("First Blood")));
    TestEqual(TEXT("time"), C.TimeLimit, 180.0, GtcTest::Eps);
    TestEqual(TEXT("objective count"), C.Objectives.Num(), 2);
    if (C.Objectives.Num() == 2)
    {
        TestEqual(TEXT("o0 id"), C.Objectives[0].Id, FString(TEXT("m1_o0")));
        TestEqual(TEXT("o0 text"), C.Objectives[0].Text, FString(TEXT("Objective 0")));
        TestTrue(TEXT("o0 has waypoint"), C.Objectives[0].bHasWaypoint);
        TestTrue(TEXT("o0 waypoint"), C.Objectives[0].Waypoint.Equals(FVector(0, 0, 0), GtcTest::Eps));
        TestFalse(TEXT("o1 no waypoint"), C.Objectives[1].bHasWaypoint);
    }

    // No objectives -> not runnable.
    FGtcMissionDefinition Empty;
    Empty.Title = TEXT("Empty");
    const FGtcCompiledMission CE = GtcMissionCompiler::CompileMission(Empty);
    TestFalse(TEXT("empty not runnable"), CE.bValid);

    return true;
}

// 2) CompileCampaign feeds the REAL pure MissionChain and a campaign progresses end-to-end.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionCompilerCampaignDrivesChainTest,
    "GTC.Missions.Editor.Compiler.CampaignDrivesChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionCompilerCampaignDrivesChainTest::RunTest(const FString& Parameters)
{
    TArray<FGtcMissionDefinition> Authored;
    Authored.Add(MakeNarrative(TEXT("intro"), TEXT("Intro"), 1));
    Authored.Add(MakeNarrative(TEXT("heist"), TEXT("The Heist"), 3));

    const TArray<MissionChain::FMissionDef> Compiled = GtcMissionCompiler::CompileCampaign(Authored);
    TestEqual(TEXT("campaign size"), Compiled.Num(), 2);

    MissionChain Chain(Compiled);
    TestEqual(TEXT("count"), Chain.Count(), 2);
    TestEqual(TEXT("starts at intro"), Chain.CurrentId(), FString(TEXT("intro")));

    // Authored objective ids and waypoints survived into the chain's carried data.
    const MissionChain::FMissionDef Cur = Chain.Current();
    TestEqual(TEXT("intro objective ids"), Cur.ObjectiveDefs.Num(), 1);
    TestTrue(TEXT("intro waypoint carried"), Cur.Waypoints.Contains(TEXT("intro_o0")));

    Chain.CompleteCurrent();
    TestEqual(TEXT("advances to heist"), Chain.CurrentId(), FString(TEXT("heist")));
    TestEqual(TEXT("heist objective ids"), Chain.Current().ObjectiveDefs.Num(), 3);

    Chain.CompleteCurrent();
    TestTrue(TEXT("campaign complete"), Chain.IsCampaignComplete());

    return true;
}

// 3) Objective drivers compile from authored data and drive reach/hold to satisfaction —
//    the hands-off auto-completion the subsystem runs each frame, proven headlessly.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionCompilerObjectiveDriversTest,
    "GTC.Missions.Editor.Compiler.ObjectiveDrivers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionCompilerObjectiveDriversTest::RunTest(const FString& Parameters)
{
    FGtcMissionDefinition D;
    D.Title = TEXT("Drive");

    FGtcObjectiveDefinition R;
    R.Id = TEXT("reach1");
    R.DriverKind = TEXT("reach");
    R.Radius = 600.0;
    R.bHasWaypoint = true;
    R.Waypoint = FVector(1000.0, 0.0, 0.0);
    D.Objectives.Add(R);

    FGtcObjectiveDefinition H;
    H.Id = TEXT("hold1");
    H.DriverKind = TEXT("hold");
    H.Radius = 600.0;
    H.Duration = 2.0;
    H.bHasWaypoint = true;
    H.Waypoint = FVector::ZeroVector;
    D.Objectives.Add(H);

    FGtcObjectiveDefinition N; // no waypoint -> excluded
    N.Id = TEXT("nowp");
    N.bHasWaypoint = false;
    D.Objectives.Add(N);

    FGtcObjectiveDefinition S; // Survive: excluded from proximity driver even with a waypoint
    S.Id = TEXT("survive1");
    S.Kind = EGtcObjectiveKind::Survive;
    S.bHasWaypoint = true;
    S.Waypoint = FVector(500.0, 0.0, 0.0);
    S.Duration = 10.0;
    D.Objectives.Add(S);

    const TMap<FString, FMissionObjectiveDriver::FObjectiveDef> Defs = GtcMissionCompiler::CompileObjectiveDrivers(D);
    TestEqual(TEXT("two driver defs"), Defs.Num(), 2);
    TestTrue(TEXT("reach present"), Defs.Contains(TEXT("reach1")));
    TestTrue(TEXT("hold present"), Defs.Contains(TEXT("hold1")));
    TestFalse(TEXT("no-waypoint excluded"), Defs.Contains(TEXT("nowp")));
    TestFalse(TEXT("survive excluded from proximity"), Defs.Contains(TEXT("survive1")));
    if (const FMissionObjectiveDriver::FObjectiveDef* Rd = Defs.Find(TEXT("reach1")))
    {
        TestEqual(TEXT("reach radius"), Rd->Radius, 600.0, GtcTest::Eps);
        TestEqual(TEXT("reach kind"), Rd->Kind, FString(TEXT("reach")));
    }

    // Reach: outside the radius is not satisfied; inside is.
    FMissionObjectiveDriver Reacher;
    Reacher.Defs = Defs;
    Reacher.Bind();
    const FVector ReachTarget(1000.0, 0.0, 0.0);
    TestFalse(TEXT("far not satisfied"), Reacher.TickObjective(TEXT("reach1"), FVector(5000.0, 0.0, 0.0), ReachTarget, 0.1));
    TestTrue(TEXT("near satisfied"), Reacher.TickObjective(TEXT("reach1"), FVector(1100.0, 0.0, 0.0), ReachTarget, 0.1));

    // Hold: must stay in range for Duration (2s) before it satisfies.
    FMissionObjectiveDriver Holder;
    Holder.Defs = Defs;
    Holder.Bind();
    bool bSatisfied = false;
    for (int32 i = 0; i < 30 && !bSatisfied; ++i)
    {
        bSatisfied = Holder.TickObjective(TEXT("hold1"), FVector(100.0, 0.0, 0.0), FVector::ZeroVector, 0.1);
    }
    TestTrue(TEXT("hold satisfied after duration"), bSatisfied);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
