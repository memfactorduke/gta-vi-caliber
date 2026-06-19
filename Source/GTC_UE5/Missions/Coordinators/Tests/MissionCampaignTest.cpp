// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MissionCampaign.h"
#include "../MissionController.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// BEHAVIOR tests (Wave 2 rule) — UMissionCampaign has NO the reference reference behavior; these
// assert ownership / lifecycle: it sequences one controller through the merged,
// parity-tested MissionChain, loads each mission's defs into the controller + the
// owned sibling FMissionObjectiveDriver, advances on pass, and replays on fail
// (gated on the player being alive). Scene wiring (group resolution, _process kick,
// player_health poll) is Wave 3; the live controller and bPlayerDead are passed in.

namespace MissionCampaignTestNS
{
    struct FCampaignRig
    {
        UGameInstance* GameInstance = nullptr;
        UMissionCampaign* Campaign = nullptr;
        UMissionController* Controller = nullptr;
    };

    FCampaignRig MakeCampaignRig()
    {
        FCampaignRig Rig;
        Rig.GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        Rig.Campaign = NewObject<UMissionCampaign>(Rig.GameInstance);
        Rig.Controller = NewObject<UMissionController>(Rig.GameInstance);
        return Rig;
    }
} // namespace MissionCampaignTestNS
using namespace MissionCampaignTestNS;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionCampaignOpeningArcShapeTest,
    "GTC.Missions.Campaign.OpeningArcMatchesGodotTable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionCampaignOpeningArcShapeTest::RunTest(const FString& Parameters)
{
    const TArray<UMissionCampaign::FCampaignMission> Arc = UMissionCampaign::OpeningArc();
    TestEqual(TEXT("five missions"), Arc.Num(), 5);
    TestEqual(TEXT("m1 id"), Arc[0].Id, FString(TEXT("intro")));
    TestEqual(TEXT("m1 title"), Arc[0].Title, FString(TEXT("WELCOME TO THE COAST")));
    TestEqual(TEXT("m1 untimed"), Arc[0].TimeLimit, 0.0, Eps);
    TestEqual(TEXT("m1 drive_strip radius == 7"), Arc[0].Objectives[1].Radius, 7.0, Eps);

    // heat is the timed mission (150s); kingpin is timed (180s) with 4 objectives.
    TestEqual(TEXT("heat time_limit == 150"), Arc[2].TimeLimit, 150.0, Eps);
    TestEqual(TEXT("kingpin time_limit == 180"), Arc[4].TimeLimit, 180.0, Eps);
    TestEqual(TEXT("kingpin has 4 objectives"), Arc[4].Objectives.Num(), 4);

    // A hold beat carries kind + duration (pickup's docks contact, 2.5s, radius 8).
    const UMissionCampaign::FCampaignObjective& Contact = Arc[1].Objectives[1];
    TestEqual(TEXT("m2_contact kind"), Contact.Kind, FString(TEXT("hold")));
    TestEqual(TEXT("m2_contact duration == 2.5"), Contact.Duration, 2.5, Eps);
    TestEqual(TEXT("m2_contact radius == 8"), Contact.Radius, 8.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionCampaignLoadsFirstMissionTest,
    "GTC.Missions.Campaign.BindLoadsFirstMissionIntoControllerAndDriver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionCampaignLoadsFirstMissionTest::RunTest(const FString& Parameters)
{
    FCampaignRig Rig = MakeCampaignRig();
    Rig.Campaign->BindController(Rig.Controller);

    TestEqual(TEXT("mission total == 5"), Rig.Campaign->MissionTotal(), 5);
    TestEqual(TEXT("none done yet"), Rig.Campaign->MissionsDone(), 0);

    // Controller got the first mission's title + objectives, and auto-began (Reset).
    TestEqual(TEXT("controller title"), Rig.Controller->Title, FString(TEXT("WELCOME TO THE COAST")));
    TestTrue(TEXT("controller active"), Rig.Controller->IsActive());
    TestEqual(TEXT("controller first objective"), Rig.Controller->CurrentObjectiveId(), FString(TEXT("reach_car")));

    // Driver got matching defs for every objective id.
    const FMissionObjectiveDriver& Driver = Rig.Campaign->GetDriver();
    TestTrue(TEXT("driver has reach_car def"), Driver.Defs.Contains(TEXT("reach_car")));
    TestEqual(TEXT("driver drive_strip radius == 7"), Driver.Defs[TEXT("drive_strip")].Radius, 7.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionCampaignAdvancesOnPassTest,
    "GTC.Missions.Campaign.AdvancesToNextMissionOnPass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionCampaignAdvancesOnPassTest::RunTest(const FString& Parameters)
{
    FCampaignRig Rig = MakeCampaignRig();
    Rig.Campaign->BindController(Rig.Controller);

    // Complete mission 1's three objectives -> controller emits mission_completed
    // -> campaign advances and loads mission 2.
    Rig.Controller->Complete(TEXT("reach_car"));
    Rig.Controller->Complete(TEXT("drive_strip"));
    Rig.Controller->Complete(TEXT("return_home"));

    TestEqual(TEXT("one mission done"), Rig.Campaign->MissionsDone(), 1);
    TestFalse(TEXT("campaign not complete"), Rig.Campaign->IsCampaignComplete());
    TestEqual(TEXT("controller loaded mission 2"), Rig.Controller->Title, FString(TEXT("THE PICKUP")));
    TestEqual(TEXT("mission 2 first objective"), Rig.Controller->CurrentObjectiveId(), FString(TEXT("m2_stash")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionCampaignCompletesWholeArcTest,
    "GTC.Missions.Campaign.CompletesWholeArc",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionCampaignCompletesWholeArcTest::RunTest(const FString& Parameters)
{
    FCampaignRig Rig = MakeCampaignRig();
    Rig.Campaign->BindController(Rig.Controller);

    // Drive every mission to completion by completing whatever the controller's
    // current objective is, until the campaign reports complete.
    int32 Guard = 0;
    while (!Rig.Campaign->IsCampaignComplete() && Guard < 100)
    {
        const FString Current = Rig.Controller->CurrentObjectiveId();
        if (Current.IsEmpty())
        {
            break;
        }
        Rig.Controller->Complete(Current);
        ++Guard;
    }

    TestTrue(TEXT("campaign complete"), Rig.Campaign->IsCampaignComplete());
    TestEqual(TEXT("all five missions done"), Rig.Campaign->MissionsDone(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMissionCampaignRetriesOnFailWhenAliveTest,
    "GTC.Missions.Campaign.ReplaysSameMissionOnFailOnceAlive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionCampaignRetriesOnFailWhenAliveTest::RunTest(const FString& Parameters)
{
    FCampaignRig Rig = MakeCampaignRig();
    Rig.Campaign->BindController(Rig.Controller);

    // Make progress on mission 1, then fail it (player death).
    Rig.Controller->Complete(TEXT("reach_car"));
    TestEqual(TEXT("progressed before fail"), Rig.Controller->CurrentObjectiveId(), FString(TEXT("drive_strip")));
    Rig.Controller->Tick(/*bPlayerDead*/ true, 0.016); // controller fails -> campaign queues retry
    TestTrue(TEXT("retry pending after fail"), Rig.Campaign->IsRetryPending());

    // While the player is still dead, the retry holds (no thrash).
    TestFalse(TEXT("no retry while dead"), Rig.Campaign->AdvanceRetryIfPending(/*bPlayerDead*/ true));
    TestTrue(TEXT("still pending"), Rig.Campaign->IsRetryPending());

    // Once the player is back up, the retry reloads the SAME mission from the top.
    TestTrue(TEXT("retry applies when alive"), Rig.Campaign->AdvanceRetryIfPending(/*bPlayerDead*/ false));
    TestFalse(TEXT("retry consumed"), Rig.Campaign->IsRetryPending());
    TestEqual(TEXT("same mission reloaded"), Rig.Controller->Title, FString(TEXT("WELCOME TO THE COAST")));
    TestEqual(TEXT("back to first objective"), Rig.Controller->CurrentObjectiveId(), FString(TEXT("reach_car")));
    TestEqual(TEXT("still mission 1 (none completed)"), Rig.Campaign->MissionsDone(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
