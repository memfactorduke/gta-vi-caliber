// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GtcMissionDefinition.h"
#include "../GtcMissionJson.h"
#include "../../../Systems/Save/SaveJson.h"
#include "../../../Tests/GtcTestTolerances.h"

// These tests pin the "openable" mission-file format: a fully-populated definition must
// survive MissionToJson -> JsonToMission with structure/ints/strings/enum-names exact and
// floats within GtcTest::Eps (the save system's round-trip-tolerance contract), and any
// malformed/non-mission text must fail safely (bValid==false, no crash, no half-built
// mission). No UWorld / subsystem / actor — pure value layer, runs headless.

namespace
{
    // A rich sample exercising every field path: two narrative objectives (Reach +
    // Eliminate), a trigger, a full reward, an activity payload (Side Job AND a multi-
    // checkpoint Street Race) and prerequisite links.
    FGtcMissionDefinition MakeSampleMission()
    {
        FGtcMissionDefinition D;
        D.MissionId = FGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444);
        D.Id = TEXT("mission_001");
        D.Title = TEXT("Welcome to Vice");
        D.Description = FText::FromString(TEXT("Meet the contact at the marina."));
        D.ContentType = EGtcMissionContentType::SideQuest;
        D.Giver = TEXT("tommy");
        D.District = TEXT("ocean_beach");
        D.TimeLimit = 300.0;
        D.bRepeatable = true;

        FGtcObjectiveDefinition O1;
        O1.Id = TEXT("go_marina");
        O1.Text = FText::FromString(TEXT("Reach the marina"));
        O1.Kind = EGtcObjectiveKind::Reach;
        O1.Waypoint = FVector(1234.5, -678.25, 90.0);
        O1.bHasWaypoint = true;
        O1.Radius = 750.0;
        O1.DriverKind = TEXT("reach");
        D.Objectives.Add(O1);

        FGtcObjectiveDefinition O2;
        O2.Id = TEXT("clear_dock");
        O2.Text = FText::FromString(TEXT("Eliminate the dealers"));
        O2.Kind = EGtcObjectiveKind::Eliminate;
        O2.bHasWaypoint = false;
        O2.TargetCount = 4;
        O2.DriverKind = TEXT("hold");
        O2.bOptional = true;
        D.Objectives.Add(O2);

        FGtcTriggerDefinition T;
        T.ObjectiveId = TEXT("go_marina");
        T.Type = EGtcTriggerType::EnterRadius;
        T.Location = FVector(1234.5, -678.25, 90.0);
        T.Radius = 750.0;
        D.Triggers.Add(T);

        D.Reward.Money = 5000;
        D.Reward.ObjectiveReward = 300;
        D.Reward.MissionBonus = 2500;
        D.Reward.Xp = 120;
        D.Reward.Respect = 15;
        D.Reward.Unlocks.Add(TEXT("weapon_smg"));
        D.Reward.Unlocks.Add(TEXT("safehouse_marina"));

        D.ActivityKind = EGtcActivityKind::StreetRace;
        D.SideJob.Kind = 2;
        D.SideJob.Pickup = FVector(10.0, 20.0, 30.0);
        D.SideJob.Dropoff = FVector(-40.0, -50.0, 60.0);
        D.SideJob.BaseReward = 800;
        D.SideJob.ParTime = 95.5;

        D.Race.Checkpoints.Add(FVector(100.0, 0.0, 0.0));
        D.Race.Checkpoints.Add(FVector(200.0, 100.0, 0.0));
        D.Race.Checkpoints.Add(FVector(300.0, 0.0, 0.0));
        D.Race.Laps = 3;
        D.Race.BaseReward = 1500;

        D.Prerequisites.Add(FGuid(0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD));
        return D;
    }
}

// 1) Full round-trip preserves every value.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionJsonRoundTripPreservesValuesTest,
    "GTC.Missions.Editor.MissionJson.RoundTripPreservesValues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionJsonRoundTripPreservesValuesTest::RunTest(const FString& Parameters)
{
    const FGtcMissionDefinition Src = MakeSampleMission();
    FGtcMissionDefinition Dst;
    const bool bOk = GtcMissionJson::JsonToMission(GtcMissionJson::MissionToJson(Src), Dst);

    TestTrue(TEXT("parse ok"), bOk);
    TestTrue(TEXT("valid"), Dst.bValid);

    // Header / strings / enums — exact.
    TestEqual(TEXT("mission_id"), Dst.MissionId, Src.MissionId);
    TestEqual(TEXT("id"), Dst.Id, Src.Id);
    TestEqual(TEXT("title"), Dst.Title, Src.Title);
    TestEqual(TEXT("description"), Dst.Description.ToString(), Src.Description.ToString());
    TestEqual(TEXT("giver"), Dst.Giver, Src.Giver);
    TestEqual(TEXT("district"), Dst.District, Src.District);
    TestTrue(TEXT("content_type"), Dst.ContentType == Src.ContentType);
    TestTrue(TEXT("activity_kind"), Dst.ActivityKind == Src.ActivityKind);
    TestEqual(TEXT("repeatable"), Dst.bRepeatable, Src.bRepeatable);
    TestEqual(TEXT("time_limit"), Dst.TimeLimit, Src.TimeLimit, GtcTest::Eps);

    // Objectives — count, order, fields.
    TestEqual(TEXT("objective count"), Dst.Objectives.Num(), Src.Objectives.Num());
    if (Dst.Objectives.Num() == 2)
    {
        TestEqual(TEXT("obj0 id"), Dst.Objectives[0].Id, Src.Objectives[0].Id);
        TestEqual(TEXT("obj0 text"), Dst.Objectives[0].Text.ToString(), Src.Objectives[0].Text.ToString());
        TestTrue(TEXT("obj0 kind"), Dst.Objectives[0].Kind == EGtcObjectiveKind::Reach);
        TestTrue(TEXT("obj0 waypoint"), Dst.Objectives[0].Waypoint.Equals(Src.Objectives[0].Waypoint, GtcTest::Eps));
        TestEqual(TEXT("obj0 radius"), Dst.Objectives[0].Radius, 750.0, GtcTest::Eps);
        TestEqual(TEXT("obj0 driver"), Dst.Objectives[0].DriverKind, FString(TEXT("reach")));

        TestTrue(TEXT("obj1 kind"), Dst.Objectives[1].Kind == EGtcObjectiveKind::Eliminate);
        TestEqual(TEXT("obj1 target"), Dst.Objectives[1].TargetCount, 4);
        TestEqual(TEXT("obj1 optional"), Dst.Objectives[1].bOptional, true);
        TestEqual(TEXT("obj1 has_waypoint"), Dst.Objectives[1].bHasWaypoint, false);
        TestEqual(TEXT("obj1 driver"), Dst.Objectives[1].DriverKind, FString(TEXT("hold")));
    }

    // Triggers.
    TestEqual(TEXT("trigger count"), Dst.Triggers.Num(), 1);
    if (Dst.Triggers.Num() == 1)
    {
        TestEqual(TEXT("trig obj"), Dst.Triggers[0].ObjectiveId, FString(TEXT("go_marina")));
        TestTrue(TEXT("trig type"), Dst.Triggers[0].Type == EGtcTriggerType::EnterRadius);
        TestEqual(TEXT("trig radius"), Dst.Triggers[0].Radius, 750.0, GtcTest::Eps);
    }

    // Reward.
    TestEqual(TEXT("money"), Dst.Reward.Money, 5000);
    TestEqual(TEXT("obj reward"), Dst.Reward.ObjectiveReward, 300);
    TestEqual(TEXT("bonus"), Dst.Reward.MissionBonus, 2500);
    TestEqual(TEXT("xp"), Dst.Reward.Xp, 120);
    TestEqual(TEXT("respect"), Dst.Reward.Respect, 15);
    TestEqual(TEXT("unlock count"), Dst.Reward.Unlocks.Num(), 2);
    if (Dst.Reward.Unlocks.Num() == 2)
    {
        TestEqual(TEXT("unlock0"), Dst.Reward.Unlocks[0], FString(TEXT("weapon_smg")));
        TestEqual(TEXT("unlock1"), Dst.Reward.Unlocks[1], FString(TEXT("safehouse_marina")));
    }

    // Side Job.
    TestEqual(TEXT("sidejob kind"), Dst.SideJob.Kind, 2);
    TestTrue(TEXT("sidejob pickup"), Dst.SideJob.Pickup.Equals(Src.SideJob.Pickup, GtcTest::Eps));
    TestTrue(TEXT("sidejob dropoff"), Dst.SideJob.Dropoff.Equals(Src.SideJob.Dropoff, GtcTest::Eps));
    TestEqual(TEXT("sidejob base"), Dst.SideJob.BaseReward, 800);
    TestEqual(TEXT("sidejob par"), Dst.SideJob.ParTime, 95.5, GtcTest::Eps);

    // Street Race — checkpoint count + order.
    TestEqual(TEXT("checkpoint count"), Dst.Race.Checkpoints.Num(), 3);
    if (Dst.Race.Checkpoints.Num() == 3)
    {
        TestTrue(TEXT("cp0"), Dst.Race.Checkpoints[0].Equals(Src.Race.Checkpoints[0], GtcTest::Eps));
        TestTrue(TEXT("cp1"), Dst.Race.Checkpoints[1].Equals(Src.Race.Checkpoints[1], GtcTest::Eps));
        TestTrue(TEXT("cp2"), Dst.Race.Checkpoints[2].Equals(Src.Race.Checkpoints[2], GtcTest::Eps));
    }
    TestEqual(TEXT("laps"), Dst.Race.Laps, 3);
    TestEqual(TEXT("race base"), Dst.Race.BaseReward, 1500);

    // Prerequisites.
    TestEqual(TEXT("prereq count"), Dst.Prerequisites.Num(), 1);
    if (Dst.Prerequisites.Num() == 1)
    {
        TestEqual(TEXT("prereq0"), Dst.Prerequisites[0], Src.Prerequisites[0]);
    }

    return true;
}

// 2) Malformed / non-mission text fails safely (no crash, bValid==false, returns false).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionJsonRejectsBadInputTest,
    "GTC.Missions.Editor.MissionJson.RejectsBadInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionJsonRejectsBadInputTest::RunTest(const FString& Parameters)
{
    FGtcMissionDefinition Out;

    TestFalse(TEXT("empty string"), GtcMissionJson::JsonToMission(FString(), Out));
    TestFalse(TEXT("empty invalid"), Out.bValid);

    TestFalse(TEXT("garbage"), GtcMissionJson::JsonToMission(TEXT("not json {{{"), Out));
    TestFalse(TEXT("garbage invalid"), Out.bValid);

    // Valid JSON, but not a mission envelope.
    TestFalse(TEXT("wrong format"), GtcMissionJson::JsonToMission(TEXT("{\"format\":\"savegame\",\"x\":1}"), Out));
    TestFalse(TEXT("wrong format invalid"), Out.bValid);

    // Envelope present but mission object empty -> invalid.
    TestFalse(TEXT("empty mission"), GtcMissionJson::JsonToMission(
        TEXT("{\"format\":\"gtc_mission\",\"version\":1,\"mission\":{}}"), Out));
    TestFalse(TEXT("empty mission invalid"), Out.bValid);

    return true;
}

// 2b) A file missing mission_id gets a fresh unique GUID (no zero-GUID collisions).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionJsonAssignsMissionIdTest,
    "GTC.Missions.Editor.MissionJson.AssignsMissionIdWhenMissing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionJsonAssignsMissionIdTest::RunTest(const FString& Parameters)
{
    FGtcMissionDefinition A;
    FGtcMissionDefinition B;
    TestTrue(TEXT("A parses"), GtcMissionJson::JsonToMission(
        TEXT("{\"format\":\"gtc_mission\",\"mission\":{\"id\":\"a\",\"title\":\"A\"}}"), A));
    TestTrue(TEXT("B parses"), GtcMissionJson::JsonToMission(
        TEXT("{\"format\":\"gtc_mission\",\"mission\":{\"id\":\"b\",\"title\":\"B\"}}"), B));

    TestTrue(TEXT("A got a valid guid"), A.MissionId.IsValid());
    TestTrue(TEXT("B got a valid guid"), B.MissionId.IsValid());
    TestTrue(TEXT("guids are distinct"), A.MissionId != B.MissionId);
    return true;
}

// 3) An empty/default mission still round-trips structurally (no objectives is allowed at
//    the format layer; the editor's validation — not the codec — flags it later).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionJsonEmptyMissionRoundTripTest,
    "GTC.Missions.Editor.MissionJson.EmptyMissionRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionJsonEmptyMissionRoundTripTest::RunTest(const FString& Parameters)
{
    FGtcMissionDefinition Src;
    Src.Id = TEXT("blank");
    Src.Title = TEXT("Blank");

    FGtcMissionDefinition Dst;
    const bool bOk = GtcMissionJson::JsonToMission(GtcMissionJson::MissionToJson(Src), Dst);
    TestTrue(TEXT("parse ok"), bOk);
    TestTrue(TEXT("valid"), Dst.bValid);
    TestEqual(TEXT("id"), Dst.Id, FString(TEXT("blank")));
    TestEqual(TEXT("objective count"), Dst.Objectives.Num(), 0);
    TestEqual(TEXT("time_limit default"), Dst.TimeLimit, 0.0, GtcTest::Eps);
    return true;
}

// 4) The whole bank round-trips through a save section (savegame persistence).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionJsonBankRoundTripTest,
    "GTC.Missions.Editor.MissionJson.BankRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionJsonBankRoundTripTest::RunTest(const FString& Parameters)
{
    TArray<FGtcMissionDefinition> Bank;
    Bank.Add(MakeSampleMission());
    FGtcMissionDefinition Second;
    Second.Id = TEXT("m2");
    Second.Title = TEXT("Second");
    FGtcObjectiveDefinition O;
    O.Id = TEXT("o");
    Second.Objectives.Add(O);
    Bank.Add(Second);

    const TSharedRef<FGtcJsonObject> Section = MakeShared<FGtcJsonObject>();
    GtcMissionJson::WriteBank(Bank, Section);

    TArray<FGtcMissionDefinition> Restored;
    GtcMissionJson::ReadBank(Section, Restored);

    TestEqual(TEXT("bank size"), Restored.Num(), 2);
    if (Restored.Num() == 2)
    {
        TestEqual(TEXT("m0 id"), Restored[0].Id, Bank[0].Id);
        TestEqual(TEXT("m0 title"), Restored[0].Title, Bank[0].Title);
        TestEqual(TEXT("m0 objectives"), Restored[0].Objectives.Num(), Bank[0].Objectives.Num());
        TestEqual(TEXT("m1 id"), Restored[1].Id, FString(TEXT("m2")));
        TestEqual(TEXT("m1 objectives"), Restored[1].Objectives.Num(), 1);
    }

    // An empty section restores an empty bank (no crash).
    TArray<FGtcMissionDefinition> Empty;
    GtcMissionJson::ReadBank(MakeShared<FGtcJsonObject>(), Empty);
    TestEqual(TEXT("empty section -> empty bank"), Empty.Num(), 0);

    return true;
}

// 5) Completed-mission progress (GUID set) round-trips through a save section.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionJsonGuidSetRoundTripTest,
    "GTC.Missions.Editor.MissionJson.GuidSetRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionJsonGuidSetRoundTripTest::RunTest(const FString& Parameters)
{
    TSet<FGuid> In;
    In.Add(FGuid::NewGuid());
    In.Add(FGuid::NewGuid());
    In.Add(FGuid::NewGuid());

    const TSharedRef<FGtcJsonObject> Section = MakeShared<FGtcJsonObject>();
    GtcMissionJson::WriteGuidSet(Section, TEXT("completed"), In);

    TSet<FGuid> Out;
    GtcMissionJson::ReadGuidSet(Section, TEXT("completed"), Out);
    TestEqual(TEXT("set size"), Out.Num(), In.Num());
    for (const FGuid& G : In)
    {
        TestTrue(TEXT("contains id"), Out.Contains(G));
    }

    // Missing key -> empty set, no crash.
    TSet<FGuid> Missing;
    GtcMissionJson::ReadGuidSet(MakeShared<FGtcJsonObject>(), TEXT("completed"), Missing);
    TestEqual(TEXT("missing -> empty"), Missing.Num(), 0);

    // String set (granted unlocks) round-trips the same way.
    TSet<FString> Unlocks;
    Unlocks.Add(TEXT("weapon_smg"));
    Unlocks.Add(TEXT("safehouse_marina"));
    const TSharedRef<FGtcJsonObject> USection = MakeShared<FGtcJsonObject>();
    GtcMissionJson::WriteStringSet(USection, TEXT("unlocks"), Unlocks);
    TSet<FString> UnlocksOut;
    GtcMissionJson::ReadStringSet(USection, TEXT("unlocks"), UnlocksOut);
    TestEqual(TEXT("unlock set size"), UnlocksOut.Num(), 2);
    TestTrue(TEXT("has smg"), UnlocksOut.Contains(TEXT("weapon_smg")));
    TestTrue(TEXT("has safehouse"), UnlocksOut.Contains(TEXT("safehouse_marina")));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
