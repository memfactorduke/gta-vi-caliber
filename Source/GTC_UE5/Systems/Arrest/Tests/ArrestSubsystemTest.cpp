// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ArrestSubsystem.h"
#include "ArrestTestListener.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// Subsystem BEHAVIOR tests (Wave 2 rule). ArrestController has NO the reference reference behavior, so
// these are NOT parity tests: they assert the UE port's ownership / lifecycle and that
// driving the bust state machine through Tick() composes the (parity-tested) FArrestModel
// math correctly. The scene-graph wiring (player/police Node3D resolution, respawn) is
// Wave 3; per the DEFERRED-OWNERSHIP decision the stars/distance are passed into Tick() and
// the fee/heat-clear land on the subsystem's OWNED state (wallet snapshot + heat-clear
// intent), never on a real Wanted/PlayerStats type.

namespace
{
    // UGameInstanceSubsystem requires a UGameInstance outer (ClassWithin); a transient one
    // suffices for these owned-state behavior assertions.
    UArrestSubsystem* MakeArrestSubsystemForTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UArrestSubsystem>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestSubsystemDefaultsTest,
    "GTC.Systems.Arrest.ArrestSubsystem.DefaultsMatchGodotExports",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestSubsystemDefaultsTest::RunTest(const FString& Parameters)
{
    UArrestSubsystem* Arrest = MakeArrestSubsystemForTest();
    TestEqual(TEXT("catch_distance == 2.0"), Arrest->CatchDistance, 2.0, Eps);
    TestEqual(TEXT("grapple_time == 1.5"), Arrest->GrappleTime, FArrestModel::DefaultGrappleTime, Eps);
    TestEqual(TEXT("cash_penalty == 0.10"), Arrest->CashPenalty, FArrestModel::DefaultCashPenalty, Eps);
    TestEqual(TEXT("starts uncornered (time_held == 0)"), Arrest->GetTimeHeld(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestSubsystemGrappleBuildsAndDecaysTest,
    "GTC.Systems.Arrest.ArrestSubsystem.GrappleBuildsWhileCorneredAndDecaysFree",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestSubsystemGrappleBuildsAndDecaysTest::RunTest(const FString& Parameters)
{
    UArrestSubsystem* Arrest = MakeArrestSubsystemForTest();
    // Wanted (2 stars) and within catch range (1.0 <= 2.0): timer builds, no bust yet.
    bool bBust = Arrest->Tick(2, 1.0, 0.5);
    TestFalse(TEXT("no bust at 0.5s"), bBust);
    TestEqual(TEXT("time_held == 0.5"), Arrest->GetTimeHeld(), 0.5, Eps);

    // Break free (out of range): timer decays back down.
    Arrest->Tick(2, 5.0, 0.2);
    TestEqual(TEXT("time_held decays to 0.3"), Arrest->GetTimeHeld(), 0.3, Eps);

    // No heat (0 stars) is also a clean break even while close.
    Arrest->Tick(0, 0.5, 0.1);
    TestEqual(TEXT("time_held decays to 0.2 with no heat"), Arrest->GetTimeHeld(), 0.2, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestSubsystemBustLandsAndTakesFeeTest,
    "GTC.Systems.Arrest.ArrestSubsystem.BustLandsTakesFeeAndRequestsHeatClear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestSubsystemBustLandsAndTakesFeeTest::RunTest(const FString& Parameters)
{
    UArrestSubsystem* Arrest = MakeArrestSubsystemForTest();
    Arrest->SetWallet(1000);

    // Hold cornered for the full grapple window (default 1.5s) in one tick: bust lands.
    const bool bBust = Arrest->Tick(3, 0.5, 1.5);
    TestTrue(TEXT("bust lands at grapple_time"), bBust);
    TestEqual(TEXT("fee == 100 (10% of 1000)"), Arrest->GetLastFee(), 100);
    TestEqual(TEXT("wallet drops to 900"), Arrest->GetWallet(), 900);
    TestTrue(TEXT("heat-clear requested"), Arrest->IsClearHeatRequested());
    TestEqual(TEXT("grapple resets after bust"), Arrest->GetTimeHeld(), 0.0, Eps);

    // The Wave 3 adapter drains the heat-clear intent exactly once.
    TestTrue(TEXT("consume returns the pending request"), Arrest->ConsumeClearHeatRequest());
    TestFalse(TEXT("intent cleared after consume"), Arrest->IsClearHeatRequested());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestSubsystemBustBroadcastsDelegateTest,
    "GTC.Systems.Arrest.ArrestSubsystem.BustBroadcastsOnBustedWithFee",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestSubsystemBustBroadcastsDelegateTest::RunTest(const FString& Parameters)
{
    UArrestSubsystem* Arrest = MakeArrestSubsystemForTest();
    Arrest->SetWallet(500);

    UArrestTestListener* Listener = NewObject<UArrestTestListener>();
    Arrest->OnBusted.AddDynamic(Listener, &UArrestTestListener::HandleBusted);

    Arrest->Tick(1, 1.0, FArrestModel::DefaultGrappleTime);
    TestTrue(TEXT("OnBusted fired"), Listener->bFired);
    TestEqual(TEXT("broadcast fee == 50 (10% of 500)"), Listener->ReceivedFee, 50);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestSubsystemGrappleProgressTest,
    "GTC.Systems.Arrest.ArrestSubsystem.GrappleProgressIsNormalisedAndSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestSubsystemGrappleProgressTest::RunTest(const FString& Parameters)
{
    UArrestSubsystem* Arrest = MakeArrestSubsystemForTest();
    // Half the grapple window cornered: progress == 0.5.
    Arrest->Tick(2, 1.0, 0.75); // 0.75 / 1.5
    TestEqual(TEXT("progress == 0.5"), Arrest->GrappleProgress(), 0.5, Eps);

    // A zero grapple_time must not divide-by-zero (the reference guards this too).
    Arrest->GrappleTime = 0.0;
    TestEqual(TEXT("zero grapple_time -> progress 0"), Arrest->GrappleProgress(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArrestSubsystemOwnsStateNoCrossDepsTest,
    "GTC.Systems.Arrest.ArrestSubsystem.OwnsArrestStateWithoutWantedOrStatsDeps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArrestSubsystemOwnsStateNoCrossDepsTest::RunTest(const FString& Parameters)
{
    // DEFERRED-OWNERSHIP assertion: a bust resolves entirely against the subsystem's OWNED
    // wallet snapshot + heat-clear intent — no Wanted/PlayerStats type is constructed or
    // queried. Negative wallet snapshots are clamped to 0 (fee never negative).
    UArrestSubsystem* Arrest = MakeArrestSubsystemForTest();
    Arrest->SetWallet(-5);
    TestEqual(TEXT("wallet clamps at 0"), Arrest->GetWallet(), 0);

    Arrest->Tick(4, 0.0, FArrestModel::DefaultGrappleTime);
    TestEqual(TEXT("fee on empty wallet == 0"), Arrest->GetLastFee(), 0);
    TestEqual(TEXT("wallet stays 0"), Arrest->GetWallet(), 0);
    TestTrue(TEXT("heat-clear still requested on a busted player"), Arrest->IsClearHeatRequested());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
