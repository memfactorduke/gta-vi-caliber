// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GtcMissionBank.h"
#include "../GtcMissionDefinition.h"
#include "../GtcMissionJson.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

// Pure, headless coverage of the authoring logic (GtcMissionBank) and the file IO that makes
// missions openable. No subsystem / world — the bank is plain C++ over a local TArray.

// 1) Create / find / remove a mission.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionBankCreateFindRemoveTest,
    "GTC.Missions.Editor.Bank.CreateFindRemove",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionBankCreateFindRemoveTest::RunTest(const FString& Parameters)
{
    TArray<FGtcMissionDefinition> Bank;
    const FGuid Id = GtcMissionBank::CreateMission(Bank, EGtcMissionContentType::SideQuest);

    TestEqual(TEXT("count after create"), Bank.Num(), 1);
    TestTrue(TEXT("id valid"), Id.IsValid());

    const FGtcMissionDefinition* Found = GtcMissionBank::Find(Bank, Id);
    TestNotNull(TEXT("found"), Found);
    if (Found)
    {
        TestTrue(TEXT("content type"), Found->ContentType == EGtcMissionContentType::SideQuest);
        TestFalse(TEXT("default title not empty"), Found->Title.IsEmpty());
        TestFalse(TEXT("default id not empty"), Found->Id.IsEmpty());
    }

    const FGuid Activity = GtcMissionBank::CreateMission(Bank, EGtcMissionContentType::Activity);
    const FGtcMissionDefinition* Act = GtcMissionBank::Find(Bank, Activity);
    TestNotNull(TEXT("activity found"), Act);
    if (Act)
    {
        TestTrue(TEXT("activity defaults to side job"), Act->ActivityKind == EGtcActivityKind::SideJob);
    }

    TestTrue(TEXT("remove ok"), GtcMissionBank::RemoveMission(Bank, Id));
    TestEqual(TEXT("count after remove"), Bank.Num(), 1);
    TestNull(TEXT("gone"), GtcMissionBank::Find(Bank, Id));
    TestFalse(TEXT("remove missing"), GtcMissionBank::RemoveMission(Bank, Id));

    return true;
}

// 2) Objective add / remove / reorder.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionBankObjectiveOpsTest,
    "GTC.Missions.Editor.Bank.ObjectiveOps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionBankObjectiveOpsTest::RunTest(const FString& Parameters)
{
    TArray<FGtcMissionDefinition> Bank;
    const FGuid Id = GtcMissionBank::CreateMission(Bank, EGtcMissionContentType::MainMission);

    auto MakeObj = [](const TCHAR* ObjId)
    {
        FGtcObjectiveDefinition O;
        O.Id = ObjId;
        return O;
    };

    TestTrue(TEXT("add a"), GtcMissionBank::AddObjective(Bank, Id, MakeObj(TEXT("a"))));
    TestTrue(TEXT("add b"), GtcMissionBank::AddObjective(Bank, Id, MakeObj(TEXT("b"))));
    TestTrue(TEXT("add c"), GtcMissionBank::AddObjective(Bank, Id, MakeObj(TEXT("c"))));
    TestFalse(TEXT("add to missing mission"), GtcMissionBank::AddObjective(Bank, FGuid::NewGuid(), MakeObj(TEXT("x"))));

    const FGtcMissionDefinition* M = GtcMissionBank::Find(Bank, Id);
    TestEqual(TEXT("3 objectives"), M->Objectives.Num(), 3);

    // Move "c" (index 2) to the front.
    TestTrue(TEXT("reorder"), GtcMissionBank::ReorderObjective(Bank, Id, 2, 0));
    TestEqual(TEXT("c now first"), M->Objectives[0].Id, FString(TEXT("c")));
    TestEqual(TEXT("a now second"), M->Objectives[1].Id, FString(TEXT("a")));
    TestFalse(TEXT("reorder bad index"), GtcMissionBank::ReorderObjective(Bank, Id, 9, 0));

    TestTrue(TEXT("remove b"), GtcMissionBank::RemoveObjective(Bank, Id, TEXT("b")));
    TestEqual(TEXT("2 objectives"), M->Objectives.Num(), 2);
    TestFalse(TEXT("remove missing objective"), GtcMissionBank::RemoveObjective(Bank, Id, TEXT("zzz")));

    return true;
}

// 2b) God-mode "place here": setting an objective's waypoint by id.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionBankSetWaypointTest,
    "GTC.Missions.Editor.Bank.SetWaypoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionBankSetWaypointTest::RunTest(const FString& Parameters)
{
    TArray<FGtcMissionDefinition> Bank;
    const FGuid Id = GtcMissionBank::CreateMission(Bank, EGtcMissionContentType::MainMission);

    FGtcObjectiveDefinition O;
    O.Id = TEXT("go");
    O.bHasWaypoint = false;
    GtcMissionBank::AddObjective(Bank, Id, O);

    const FVector Where(1500.0, -250.0, 90.0);
    TestTrue(TEXT("set ok"), GtcMissionBank::SetObjectiveWaypoint(Bank, Id, TEXT("go"), Where));

    const FGtcMissionDefinition* M = GtcMissionBank::Find(Bank, Id);
    TestTrue(TEXT("waypoint stored"), M->Objectives[0].Waypoint.Equals(Where));
    TestTrue(TEXT("now has waypoint"), M->Objectives[0].bHasWaypoint);

    TestFalse(TEXT("missing objective"), GtcMissionBank::SetObjectiveWaypoint(Bank, Id, TEXT("nope"), Where));
    TestFalse(TEXT("missing mission"), GtcMissionBank::SetObjectiveWaypoint(Bank, FGuid::NewGuid(), TEXT("go"), Where));

    return true;
}

// 2c) World-availability gating: opt-in + prerequisites + completion/repeatable.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionBankAvailabilityTest,
    "GTC.Missions.Editor.Bank.Availability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionBankAvailabilityTest::RunTest(const FString& Parameters)
{
    const FGuid PreId = FGuid::NewGuid();

    FGtcMissionDefinition Gated;
    Gated.MissionId = FGuid::NewGuid();
    Gated.bAvailableInWorld = true;
    Gated.Prerequisites.Add(PreId);

    TSet<FGuid> Completed;

    // Opt-in but prerequisite not met -> unavailable.
    TestFalse(TEXT("locked by prereq"), GtcMissionBank::IsMissionAvailable(Gated, Completed));

    // Prerequisite completed -> available.
    Completed.Add(PreId);
    TestTrue(TEXT("unlocked"), GtcMissionBank::IsMissionAvailable(Gated, Completed));

    // Not opted in -> never available.
    FGtcMissionDefinition NotInWorld;
    NotInWorld.MissionId = FGuid::NewGuid();
    NotInWorld.bAvailableInWorld = false;
    TestFalse(TEXT("not opted in"), GtcMissionBank::IsMissionAvailable(NotInWorld, Completed));

    // Completed + non-repeatable -> drops out; repeatable -> stays.
    FGtcMissionDefinition Done;
    Done.MissionId = FGuid::NewGuid();
    Done.bAvailableInWorld = true;
    Completed.Add(Done.MissionId);
    TestFalse(TEXT("done non-repeatable hidden"), GtcMissionBank::IsMissionAvailable(Done, Completed));
    Done.bRepeatable = true;
    TestTrue(TEXT("done repeatable stays"), GtcMissionBank::IsMissionAvailable(Done, Completed));

    return true;
}

// 2d) Count-objective classifier (which kinds NotifyObjectiveProgress advances).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionBankCountObjectiveTest,
    "GTC.Missions.Editor.Bank.CountObjective",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionBankCountObjectiveTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("collect counts"), GtcMissionBank::IsCountObjective(EGtcObjectiveKind::Collect));
    TestTrue(TEXT("eliminate counts"), GtcMissionBank::IsCountObjective(EGtcObjectiveKind::Eliminate));
    TestFalse(TEXT("reach not count"), GtcMissionBank::IsCountObjective(EGtcObjectiveKind::Reach));
    TestFalse(TEXT("survive not count"), GtcMissionBank::IsCountObjective(EGtcObjectiveKind::Survive));
    TestFalse(TEXT("interact not count"), GtcMissionBank::IsCountObjective(EGtcObjectiveKind::Interact));
    return true;
}

// 3) Validation catches the load-bearing authoring mistakes.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionBankValidationTest,
    "GTC.Missions.Editor.Bank.Validation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionBankValidationTest::RunTest(const FString& Parameters)
{
    // A clean narrative mission validates.
    {
        FGtcMissionDefinition D;
        D.Title = TEXT("Good");
        D.ContentType = EGtcMissionContentType::MainMission;
        FGtcObjectiveDefinition O;
        O.Id = TEXT("go");
        O.Text = FText::FromString(TEXT("Go there"));
        O.Waypoint = FVector(100.0, 0.0, 0.0);
        D.Objectives.Add(O);
        const FGtcMissionValidation V = GtcMissionBank::Validate(D);
        TestTrue(TEXT("clean mission valid"), V.IsValid());
    }

    // Empty title + no objectives -> two errors.
    {
        FGtcMissionDefinition D;
        D.ContentType = EGtcMissionContentType::MainMission;
        const FGtcMissionValidation V = GtcMissionBank::Validate(D);
        TestFalse(TEXT("invalid"), V.IsValid());
        TestTrue(TEXT(">=2 errors"), V.Errors.Num() >= 2);
    }

    // Duplicate objective ids -> error.
    {
        FGtcMissionDefinition D;
        D.Title = TEXT("Dup");
        FGtcObjectiveDefinition A; A.Id = TEXT("x");
        FGtcObjectiveDefinition B; B.Id = TEXT("x");
        D.Objectives.Add(A);
        D.Objectives.Add(B);
        const FGtcMissionValidation V = GtcMissionBank::Validate(D);
        TestFalse(TEXT("dup invalid"), V.IsValid());
    }

    // Collect with zero target -> error; bad driver kind -> warning.
    {
        FGtcMissionDefinition D;
        D.Title = TEXT("Counts");
        FGtcObjectiveDefinition O;
        O.Id = TEXT("collect");
        O.Kind = EGtcObjectiveKind::Collect;
        O.TargetCount = 0;
        O.DriverKind = TEXT("teleport");
        D.Objectives.Add(O);
        const FGtcMissionValidation V = GtcMissionBank::Validate(D);
        TestFalse(TEXT("zero count invalid"), V.IsValid());
        TestTrue(TEXT("warns about driver"), V.Warnings.Num() >= 1);
    }

    // Negative time -> error.
    {
        FGtcMissionDefinition D;
        D.Title = TEXT("Timed");
        FGtcObjectiveDefinition O; O.Id = TEXT("o");
        D.Objectives.Add(O);
        D.TimeLimit = -5.0;
        const FGtcMissionValidation V = GtcMissionBank::Validate(D);
        TestFalse(TEXT("negative time invalid"), V.IsValid());
    }

    // Activity street race with no checkpoints -> error.
    {
        FGtcMissionDefinition D;
        D.Title = TEXT("Race");
        D.ContentType = EGtcMissionContentType::Activity;
        D.ActivityKind = EGtcActivityKind::StreetRace;
        D.Race.Laps = 1;
        const FGtcMissionValidation V = GtcMissionBank::Validate(D);
        TestFalse(TEXT("empty race invalid"), V.IsValid());
    }

    return true;
}

// 4) File save -> load round-trip preserves the mission (the on-disk "openable" path).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcMissionBankFileRoundTripTest,
    "GTC.Missions.Editor.Bank.FileRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcMissionBankFileRoundTripTest::RunTest(const FString& Parameters)
{
    FGtcMissionDefinition D;
    D.MissionId = FGuid::NewGuid();
    D.Id = TEXT("file_test");
    D.Title = TEXT("On Disk");
    D.ContentType = EGtcMissionContentType::SideQuest;
    D.TimeLimit = 42.0;
    FGtcObjectiveDefinition O;
    O.Id = TEXT("o1");
    O.Text = FText::FromString(TEXT("Do it"));
    O.Waypoint = FVector(7.0, 8.0, 9.0);
    D.Objectives.Add(O);

    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MissionEditorTest"), TEXT("rt.mission.json"));

    TestTrue(TEXT("save ok"), GtcMissionJson::SaveMissionToFile(D, Path));
    TestTrue(TEXT("file exists"), IFileManager::Get().FileExists(*Path));

    FGtcMissionDefinition Loaded;
    TestTrue(TEXT("load ok"), GtcMissionJson::LoadMissionFromFile(Path, Loaded));
    TestTrue(TEXT("loaded valid"), Loaded.bValid);
    TestEqual(TEXT("id"), Loaded.Id, D.Id);
    TestEqual(TEXT("title"), Loaded.Title, D.Title);
    TestEqual(TEXT("mission_id"), Loaded.MissionId, D.MissionId);
    TestEqual(TEXT("time"), Loaded.TimeLimit, 42.0, GtcTest::Eps);
    TestEqual(TEXT("objective count"), Loaded.Objectives.Num(), 1);
    if (Loaded.Objectives.Num() == 1)
    {
        TestTrue(TEXT("waypoint"), Loaded.Objectives[0].Waypoint.Equals(O.Waypoint, GtcTest::Eps));
    }

    // Missing file -> safe failure.
    FGtcMissionDefinition Missing;
    TestFalse(TEXT("missing file fails"),
        GtcMissionJson::LoadMissionFromFile(Path + TEXT(".nope"), Missing));
    TestFalse(TEXT("missing invalid"), Missing.bValid);

    IFileManager::Get().Delete(*Path);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
