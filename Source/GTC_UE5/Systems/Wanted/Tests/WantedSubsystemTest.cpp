// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WantedSubsystem.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// Subsystem BEHAVIOR tests (Wave 2 rule): these verify UWantedSubsystem OWNS its three
// pure models (FWantedSystem, the FCrimeWitness report timers, FWantedEvasion) and that
// crime/tick/evasion drivers wire them correctly — they do NOT re-test the pure
// heat/LOS/evasion math (that is the 11/25/19 parity oracles above). The Godot
// wanted_tracker.gd has NO parity oracle, so these are behavior assertions for the port,
// not 1:1 parity. Scene-graph wiring (WeaponController signal, observer gathering from
// "pedestrians"/"police" groups, witness-down liveness) is Wave 3 and DEFERRED.

namespace
{
    // UGameInstanceSubsystem requires a UGameInstance outer (ClassWithin); a transient one
    // suffices. Driving Initialize() needs a live FSubsystemCollectionBase, impractical to
    // spin up headless (same constraint SaveSubsystemTest documents), so the subsystem is
    // constructed directly and its public API driven. The owned models are configured by
    // member initializers whose values match the @export defaults Initialize would thread
    // in, so the construction-only path is equivalent for these defaults; Deinitialize is
    // still driven explicitly to exercise the lifecycle hook.
    UWantedSubsystem* MakeWantedSubsystemForTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UWantedSubsystem>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemInitDefaultsTest,
    "GTC.Systems.Wanted.WantedSubsystem.InitializesCleanWithTuning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemInitDefaultsTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    // Owns a clean heat model after Initialize, configured from the @export decay/cap.
    TestEqual(TEXT("stars == 0"), Subsystem->Stars(), 0);
    TestFalse(TEXT("not wanted"), Subsystem->IsWanted());
    TestEqual(TEXT("decay rate threaded into model"), Subsystem->GetWantedSystem().DecayRate, Subsystem->DecayRate, Eps);
    TestEqual(TEXT("heat cap threaded into model"), Subsystem->GetWantedSystem().HeatCap, Subsystem->HeatCap, Eps);
    TestTrue(TEXT("evasion starts visible"), Subsystem->GetEvasion().IsVisible());
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemReportCrimeHeatsTest,
    "GTC.Systems.Wanted.WantedSubsystem.ReportCrimeDrivesOwnedHeatModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemReportCrimeHeatsTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    // Unconditional (already-reported) kill: lands KillHeat directly into the owned model.
    Subsystem->ReportCrime(/*bKilled*/ true);
    TestTrue(TEXT("is wanted after kill"), Subsystem->IsWanted());
    TestEqual(TEXT("heat == KillHeat"), Subsystem->GetWantedSystem().Heat, Subsystem->KillHeat, Eps);
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemUnseenCrimeNoHeatTest,
    "GTC.Systems.Wanted.WantedSubsystem.UnseenWitnessedCrimeAddsNoHeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemUnseenCrimeNoHeatTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    // A lone kill in an empty alley: no observers -> perception gate drops it, no heat.
    Subsystem->ReportWitnessedCrime(/*bKilled*/ true, FVector::ZeroVector, TArray<FCrimeObserver>());
    TestFalse(TEXT("not wanted, unseen"), Subsystem->IsWanted());
    TestEqual(TEXT("no pending report"), Subsystem->PendingReportCount(), 0);
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemCopWitnessInstantTest,
    "GTC.Systems.Wanted.WantedSubsystem.CopWitnessReportsInstantly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemCopWitnessInstantTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    // A cop standing at -10 on X, facing +X toward the crime at the origin: within the
    // police cone, so the report is instant (no pending queue) and heat lands now.
    const TArray<FCrimeObserver> Observers = {
        FCrimeObserver(FVector(-10, 0, 0), FVector(1, 0, 0), /*bIsPolice*/ true),
    };
    Subsystem->ReportWitnessedCrime(/*bKilled*/ false, FVector::ZeroVector, Observers);
    TestTrue(TEXT("wanted instantly"), Subsystem->IsWanted());
    TestEqual(TEXT("no pending (instant)"), Subsystem->PendingReportCount(), 0);
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemCivilianReportDelayedTest,
    "GTC.Systems.Wanted.WantedSubsystem.CivilianWitnessQueuesDelayedReport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemCivilianReportDelayedTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    // A civilian witnesses it: queues a pending report, no heat yet.
    const TArray<FCrimeObserver> Observers = {
        FCrimeObserver(FVector(-10, 0, 0), FVector(1, 0, 0), /*bIsPolice*/ false),
    };
    Subsystem->ReportWitnessedCrime(/*bKilled*/ false, FVector::ZeroVector, Observers);
    TestFalse(TEXT("no heat before call lands"), Subsystem->IsWanted());
    TestEqual(TEXT("one pending report"), Subsystem->PendingReportCount(), 1);
    // Tick past the report delay (2.5s default): the call lands, heat applies, queue drains.
    Subsystem->TickFrame(3.0);
    TestTrue(TEXT("wanted after call lands"), Subsystem->IsWanted());
    TestEqual(TEXT("pending drained"), Subsystem->PendingReportCount(), 0);
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemHeatHoldsThenDecaysTest,
    "GTC.Systems.Wanted.WantedSubsystem.HeatHoldsDuringWindowThenDecays",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemHeatHoldsThenDecaysTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    Subsystem->ReportCrime(/*bKilled*/ true); // arms the crime-active window (0.6s)
    const double HeatAfterCrime = Subsystem->GetWantedSystem().Heat;
    // Within the active window heat HOLDS (committing flag true).
    Subsystem->TickFrame(0.3);
    TestEqual(TEXT("heat holds inside window"), Subsystem->GetWantedSystem().Heat, HeatAfterCrime, Eps);
    // After the window expires heat decays.
    Subsystem->TickFrame(1.0);
    TestTrue(TEXT("heat decays after window"), Subsystem->GetWantedSystem().Heat < HeatAfterCrime);
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemClearDropsReportsTest,
    "GTC.Systems.Wanted.WantedSubsystem.ClearWipesHeatAndPendingReports",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemClearDropsReportsTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    Subsystem->ReportCrime(/*bKilled*/ true);
    const TArray<FCrimeObserver> Observers = {
        FCrimeObserver(FVector(-10, 0, 0), FVector(1, 0, 0), /*bIsPolice*/ false),
    };
    Subsystem->ReportWitnessedCrime(/*bKilled*/ false, FVector::ZeroVector, Observers);
    TestEqual(TEXT("has pending"), Subsystem->PendingReportCount(), 1);
    Subsystem->Clear();
    TestFalse(TEXT("not wanted after clear"), Subsystem->IsWanted());
    TestEqual(TEXT("heat == 0"), Subsystem->GetWantedSystem().Heat, 0.0, Eps);
    TestEqual(TEXT("pending dropped"), Subsystem->PendingReportCount(), 0);
    // A queued report must not re-apply heat after a clear.
    Subsystem->TickFrame(5.0);
    TestFalse(TEXT("still not wanted"), Subsystem->IsWanted());
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemSerializeRoundTripTest,
    "GTC.Systems.Wanted.WantedSubsystem.HeatSerializeRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemSerializeRoundTripTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    Subsystem->ReportCrime(/*bKilled*/ true);
    const double Snapshot = Subsystem->SerializeHeat();

    UWantedSubsystem* Restored = MakeWantedSubsystemForTest();
    Restored->RestoreHeat(Snapshot);
    TestEqual(TEXT("heat restored"), Restored->GetWantedSystem().Heat, Snapshot, Eps);
    // Negative restore floors at 0 (Godot maxf guard).
    Restored->RestoreHeat(-5.0);
    TestEqual(TEXT("negative floors to 0"), Restored->GetWantedSystem().Heat, 0.0, Eps);
    Subsystem->Deinitialize();
    Restored->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSubsystemEvasionDriverTest,
    "GTC.Systems.Wanted.WantedSubsystem.EvasionDriverFeedsOwnedStateMachine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSubsystemEvasionDriverTest::RunTest(const FString& Parameters)
{
    UWantedSubsystem* Subsystem = MakeWantedSubsystemForTest();
    // Losing line-of-sight drives the owned evasion machine into SEARCHING.
    Subsystem->UpdateEvasion(/*bSeenByPolice*/ false, 1.0);
    TestTrue(TEXT("evasion searching"), Subsystem->GetEvasion().IsSearching());
    // Re-sighting snaps it back to VISIBLE.
    Subsystem->UpdateEvasion(/*bSeenByPolice*/ true, 0.0);
    TestTrue(TEXT("evasion visible after re-sight"), Subsystem->GetEvasion().IsVisible());
    Subsystem->Deinitialize();
    return true;
}

#endif // WITH_AUTOMATION_TESTS
