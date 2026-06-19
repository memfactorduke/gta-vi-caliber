// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StatsCoordinator.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// Subsystem LOGIC-PARITY tests (Wave 2). A Godot oracle DOES exist —
// game/tests/unit/test_stats_coordinator_fix.gd — and these align 1:1 with its
// assertions using its own constants: death-while-wanted is NOT an evasion
// (busts_evaded == 0.0), a genuine escape still counts (== 1.0), death-while-clean does
// not suppress a later escape, and the save round-trip preserves missions_passed == 4.0
// / busts_evaded == 2.0. The oracle drives the model through the Node's _process / signal
// stubs; only that scene-graph wiring (resolving the "mission"/"player_health"/"wanted"
// groups, signal binding, per-frame polling) is deferred to Wave 3 — here the same ported
// logic is exercised through the explicit driver methods (UpdateWanted/OnPlayerDied/
// OnMissionPassed). UStatsCoordinator is created with a transient UGameInstance outer
// (ClassWithin); the owned model is lazily ensured. RestoreClearsWantedLatch is an extra
// behavior assertion beyond the oracle (no oracle func — marked inline).

namespace
{
    UStatsCoordinator* MakeStatsCoordinatorForTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UStatsCoordinator>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsCoordMissionTest,
    "GTC.Systems.StatsCoordinator.MissionPassedIncrementsStat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatsCoordMissionTest::RunTest(const FString& Parameters)
{
    UStatsCoordinator* Coordinator = MakeStatsCoordinatorForTest();
    Coordinator->OnMissionPassed();
    Coordinator->OnMissionPassed();
    TestEqual(TEXT("missions_passed == 2"), Coordinator->Stat(TEXT("missions_passed")), 2.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsCoordEvasionTest,
    "GTC.Systems.StatsCoordinator.WantedRiseThenClearCountsEvasion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatsCoordEvasionTest::RunTest(const FString& Parameters)
{
    UStatsCoordinator* Coordinator = MakeStatsCoordinatorForTest();
    // Wanted rises then clears: one busts_evaded.
    Coordinator->UpdateWanted(true);
    Coordinator->UpdateWanted(false);
    TestEqual(TEXT("busts_evaded == 1"), Coordinator->Stat(TEXT("busts_evaded")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsCoordDeathNotEvasionTest,
    "GTC.Systems.StatsCoordinator.DeathClearedWantedIsNotEvasion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatsCoordDeathNotEvasionTest::RunTest(const FString& Parameters)
{
    UStatsCoordinator* Coordinator = MakeStatsCoordinatorForTest();
    // Wanted, then die while wanted, then the wanted level clears (death/respawn): NOT an
    // evasion. Mirrors the _death_cleared_wanted consume path.
    Coordinator->UpdateWanted(true);
    Coordinator->OnPlayerDied();
    Coordinator->UpdateWanted(false);
    TestEqual(TEXT("busts_evaded == 0 after death-clear"), Coordinator->Stat(TEXT("busts_evaded")), 0.0, Eps);
    // A subsequent genuine rise-then-clear still counts (latch was consumed, not stuck).
    Coordinator->UpdateWanted(true);
    Coordinator->UpdateWanted(false);
    TestEqual(TEXT("busts_evaded == 1 after genuine escape"), Coordinator->Stat(TEXT("busts_evaded")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsCoordSaveRoundTripTest,
    "GTC.Systems.StatsCoordinator.SaveRoundTripPreservesStats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatsCoordSaveRoundTripTest::RunTest(const FString& Parameters)
{
    UStatsCoordinator* Coordinator = MakeStatsCoordinatorForTest();
    Coordinator->Restore({
        TPair<FString, double>(TEXT("kills"), 100.0),
        TPair<FString, double>(TEXT("missions_passed"), 4.0),
    });
    const TArray<TPair<FString, double>> Snapshot = Coordinator->SerializeState();

    UStatsCoordinator* Restored = MakeStatsCoordinatorForTest();
    Restored->Restore(Snapshot);
    TestEqual(TEXT("kills == 100"), Restored->Stat(TEXT("kills")), 100.0, Eps);
    TestEqual(TEXT("missions == 4"), Restored->Stat(TEXT("missions_passed")), 4.0, Eps);
    // Completion derives from restored stats (centurion earned -> 20%).
    TestEqual(TEXT("completion == 20"), Restored->CompletionPercent(), 20.0, Eps);
    return true;
}

// NO-ORACLE-FUNC: extra behavior assertion beyond test_stats_coordinator_fix.gd —
// guards that Restore() resets the _was_wanted/_death latch so a load can't bleed a
// spurious evasion into the loaded game.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatsCoordRestoreClearsLatchTest,
    "GTC.Systems.StatsCoordinator.RestoreClearsWantedLatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatsCoordRestoreClearsLatchTest::RunTest(const FString& Parameters)
{
    UStatsCoordinator* Coordinator = MakeStatsCoordinatorForTest();
    // Become wanted, then restore: the _was_wanted/_death latch resets, so the next clear
    // is not spuriously counted as an evasion against the loaded game.
    Coordinator->UpdateWanted(true);
    Coordinator->Restore(TArray<TPair<FString, double>>());
    Coordinator->UpdateWanted(false);
    TestEqual(TEXT("busts_evaded == 0 after restore"), Coordinator->Stat(TEXT("busts_evaded")), 0.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
