// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ProgressionTracker.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// Subsystem BEHAVIOR / lifecycle tests for UProgressionTracker. The Godot
// progression_tracker.gd has NO parity oracle (test_progression_tracker.gd has 0
// funcs), so these are fresh tests of the Wave 2 ownership model: the subsystem owns a
// single FPlayerProgression and drives it via the objective/mission award methods that
// replace the Godot MissionController signal wiring (Wave 3).
//
// UGameInstanceSubsystem has ClassWithin = UGameInstance, so it must be created with a
// GameInstance outer (a transient one suffices for headless ownership/driving tests).

namespace
{
    UProgressionTracker* MakeProgressionTrackerForTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UProgressionTracker>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgTrackerOwnsFreshModelTest,
    "GTC.Systems.Progression.ProgressionTracker.OwnsFreshModelAtStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgTrackerOwnsFreshModelTest::RunTest(const FString& Parameters)
{
    UProgressionTracker* Tracker = MakeProgressionTrackerForTest();
    // The owned model starts at level 1 / 0 XP without any explicit Initialize call.
    TestEqual(TEXT("level == 1"), Tracker->Level(), 1);
    TestEqual(TEXT("total_xp == 0"), Tracker->TotalXp(), 0);
    TestEqual(TEXT("progress == 0"), Tracker->LevelProgress(), 0.0, Eps);
    // Owned-model accessor exposes the same state (ownership stays with the subsystem).
    TestEqual(TEXT("owned level == 1"), Tracker->GetProgression().Level(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgTrackerAwardObjectiveTest,
    "GTC.Systems.Progression.ProgressionTracker.AwardObjectiveDrivesModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgTrackerAwardObjectiveTest::RunTest(const FString& Parameters)
{
    UProgressionTracker* Tracker = MakeProgressionTrackerForTest();
    Tracker->ObjectiveXp = 120;  // default; assert the driver feeds the model.
    Tracker->AwardObjective();
    // 120 XP at level 1 (leave costs 100) -> level 2 with 20 carried in.
    TestEqual(TEXT("total_xp == 120"), Tracker->TotalXp(), 120);
    TestEqual(TEXT("level == 2"), Tracker->Level(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgTrackerAwardMissionTest,
    "GTC.Systems.Progression.ProgressionTracker.AwardMissionDrivesModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgTrackerAwardMissionTest::RunTest(const FString& Parameters)
{
    UProgressionTracker* Tracker = MakeProgressionTrackerForTest();
    Tracker->MissionXp = 600;
    Tracker->AwardMission();
    TestEqual(TEXT("total_xp == 600"), Tracker->TotalXp(), 600);
    // 600 XP: -100 (L2) -200 (L3) -300 (L4) leaves 0 -> level 4.
    TestEqual(TEXT("level == 4"), Tracker->Level(), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProgTrackerDeinitializeTest,
    "GTC.Systems.Progression.ProgressionTracker.DeinitializeIsClean",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgTrackerDeinitializeTest::RunTest(const FString& Parameters)
{
    // Lifecycle smoke test: a tracker that earned XP can be torn down without state
    // leaking across the owned-model boundary (value member, no external resource).
    UProgressionTracker* Tracker = MakeProgressionTrackerForTest();
    Tracker->AwardMission();
    TestTrue(TEXT("earned xp"), Tracker->TotalXp() > 0);
    Tracker->Deinitialize();
    // After Deinitialize the UObject is still valid until GC; the owned model is a value
    // member so the subsystem remains queryable and consistent.
    TestEqual(TEXT("still consistent"), Tracker->TotalXp(), 600);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
