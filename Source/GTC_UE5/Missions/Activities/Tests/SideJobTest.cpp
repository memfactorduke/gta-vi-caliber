// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SideJob.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to a test_* assertion in the Godot parity oracle
// game/tests/unit/test_side_job.gd.

using GtcTest::Eps;
using SJ = SideJob;

// --- fare -------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobFareScalesWithDistanceTest,
    "GTC.Missions.Activities.SideJob.FareScalesWithDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobFareScalesWithDistanceTest::RunTest(const FString& Parameters)
{
    // 100 base + 200m * 1.5 = 100 + 300 = 400.
    TestEqual(TEXT("fare == 400"), SJ::Fare(200.0, 100, 1.5), 400);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobFareFloorsAtBaseTest,
    "GTC.Missions.Activities.SideJob.FareFloorsAtBaseWithZeroDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobFareFloorsAtBaseTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("fare == 50"), SJ::Fare(0.0, 50, 1.5), 50);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobFareClampsNegativeDistanceTest,
    "GTC.Missions.Activities.SideJob.FareClampsNegativeDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobFareClampsNegativeDistanceTest::RunTest(const FString& Parameters)
{
    // Negative distance treated as 0 -> just the base.
    TestEqual(TEXT("fare == 75"), SJ::Fare(-500.0, 75, 2.0), 75);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobFareNeverNegativeTest,
    "GTC.Missions.Activities.SideJob.FareNeverNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobFareNeverNegativeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("fare == 0"), SJ::Fare(-10.0, -10, -1.0), 0);
    return true;
}

// --- vigilante --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobVigilanteByTargetsTest,
    "GTC.Missions.Activities.SideJob.VigilanteRewardByTargets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobVigilanteByTargetsTest::RunTest(const FString& Parameters)
{
    // 200 base + 3 targets * 100 = 500.
    TestEqual(TEXT("reward == 500"), SJ::VigilanteReward(3, 200, 100), 500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobVigilanteZeroTargetsTest,
    "GTC.Missions.Activities.SideJob.VigilanteZeroTargetsIsBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobVigilanteZeroTargetsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("reward == 200"), SJ::VigilanteReward(0, 200, 100), 200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobVigilanteNegativeTargetsTest,
    "GTC.Missions.Activities.SideJob.VigilanteNegativeTargetsGuarded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobVigilanteNegativeTargetsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("reward == 150"), SJ::VigilanteReward(-4, 150, 100), 150);
    return true;
}

// --- time_bonus -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobTimeBonusUnderParTest,
    "GTC.Missions.Activities.SideJob.TimeBonusPositiveUnderPar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobTimeBonusUnderParTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("bonus == 500"), SJ::TimeBonus(20.0, 30.0, 500), 500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobTimeBonusAtParTest,
    "GTC.Missions.Activities.SideJob.TimeBonusFullAtExactlyPar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobTimeBonusAtParTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("bonus == 500"), SJ::TimeBonus(30.0, 30.0, 500), 500);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobTimeBonusLinearDecayTest,
    "GTC.Missions.Activities.SideJob.TimeBonusLinearDecayOverPar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobTimeBonusLinearDecayTest::RunTest(const FString& Parameters)
{
    // 1.5x par -> 50% of the bonus.
    TestEqual(TEXT("bonus == 250"), SJ::TimeBonus(45.0, 30.0, 500), 250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobTimeBonusClampedTest,
    "GTC.Missions.Activities.SideJob.TimeBonusClampedNonNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobTimeBonusClampedTest::RunTest(const FString& Parameters)
{
    // Negative bonus and a degenerate par both yield 0, never negative.
    TestEqual(TEXT("negative bonus == 0"), SJ::TimeBonus(10.0, 30.0, -100), 0);
    TestEqual(TEXT("degenerate par == 0"), SJ::TimeBonus(10.0, 0.0, 500), 0);
    return true;
}

// --- payout -----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobPayoutTaxiTest,
    "GTC.Missions.Activities.SideJob.PayoutTaxiCombinesFareAndBonus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobPayoutTaxiTest::RunTest(const FString& Parameters)
{
    // Taxi: 100 base + 100m*1.5 = 250 fare, + full 100 time bonus (under par) = 350.
    const SJ::FJob Job = SJ::MakeJob(
        static_cast<int32>(SJ::EKind::Taxi), FVector::ZeroVector, FVector(100, 0, 0), 100);
    TestEqual(TEXT("payout == 350"), SJ::Payout(Job, 100.0, 10.0, 30.0), 350);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobPayoutVigilanteTest,
    "GTC.Missions.Activities.SideJob.PayoutVigilanteUsesTargetCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobPayoutVigilanteTest::RunTest(const FString& Parameters)
{
    // Vigilante: distance arg carries kill count (2). 300 base + 2*150 = 600 core,
    // + 300 time bonus (under par) = 900.
    const SJ::FJob Job = SJ::MakeJob(
        static_cast<int32>(SJ::EKind::Vigilante), FVector::ZeroVector, FVector(5, 0, 0), 300);
    TestEqual(TEXT("payout == 900"), SJ::Payout(Job, 2.0, 5.0, 20.0), 900);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobPayoutOverParTest,
    "GTC.Missions.Activities.SideJob.PayoutOverParDropsBonus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobPayoutOverParTest::RunTest(const FString& Parameters)
{
    // Same taxi trip but over par -> only the 250 fare, no bonus.
    const SJ::FJob Job = SJ::MakeJob(
        static_cast<int32>(SJ::EKind::Taxi), FVector::ZeroVector, FVector(100, 0, 0), 100);
    TestEqual(TEXT("payout == 250"), SJ::Payout(Job, 100.0, 99.0, 30.0), 250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobPayoutNeverNegativeTest,
    "GTC.Missions.Activities.SideJob.PayoutNeverNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobPayoutNeverNegativeTest::RunTest(const FString& Parameters)
{
    const SJ::FJob Job = SJ::MakeJob(
        static_cast<int32>(SJ::EKind::Delivery), FVector::ZeroVector, FVector::ZeroVector, 0);
    TestEqual(TEXT("payout == 0"), SJ::Payout(Job, -50.0, 100.0, 30.0), 0);
    return true;
}

// --- chain_multiplier -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobChainBaseTest,
    "GTC.Missions.Activities.SideJob.ChainMultiplierBaseIsOneAndFloored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobChainBaseTest::RunTest(const FString& Parameters)
{
    // First job (0 streak) is exactly 1.0; a negative streak never drops below 1.0.
    TestEqual(TEXT("base == 1.0"), SJ::ChainMultiplier(0), 1.0, Eps);
    TestTrue(TEXT("negative floored"), SJ::ChainMultiplier(-5) >= 1.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobChainGrowsTest,
    "GTC.Missions.Activities.SideJob.ChainMultiplierGrowsWithStreak",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobChainGrowsTest::RunTest(const FString& Parameters)
{
    // 3 in a row -> 1.0 + 3*0.1 = 1.3.
    TestEqual(TEXT("streak == 1.3"), SJ::ChainMultiplier(3), 1.3, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobChainCapsTest,
    "GTC.Missions.Activities.SideJob.ChainMultiplierCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobChainCapsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("caps at 2.0"), SJ::ChainMultiplier(50), 2.0, Eps);
    return true;
}

// --- stateful active-job lifecycle ------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobStartsWithNoActiveTest,
    "GTC.Missions.Activities.SideJob.StartsWithNoActiveJob",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobStartsWithNoActiveTest::RunTest(const FString& Parameters)
{
    SJ S;
    TestFalse(TEXT("no active"), S.HasActive());
    TestEqual(TEXT("completed == 0"), S.CompletedCount(), 0);
    TestEqual(TEXT("active kind == -1"), S.ActiveKind(), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobLifecycleTest,
    "GTC.Missions.Activities.SideJob.LifecycleStartPickupDropoffComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobLifecycleTest::RunTest(const FString& Parameters)
{
    SJ S;
    const SJ::FJob Job = SJ::MakeJob(
        static_cast<int32>(SJ::EKind::Taxi), FVector::ZeroVector, FVector(10, 0, 0), 100);
    S.Start(Job);
    TestTrue(TEXT("has active"), S.HasActive());
    TestTrue(TEXT("at pickup"), S.Stage() == SJ::EStage::Pickup);
    TestFalse(TEXT("pickup not done"), S.IsPickupDone());
    S.AdvanceStage();
    TestTrue(TEXT("at dropoff"), S.Stage() == SJ::EStage::Dropoff);
    TestTrue(TEXT("pickup done"), S.IsPickupDone());
    S.Complete();
    TestFalse(TEXT("no active after complete"), S.HasActive());
    TestEqual(TEXT("completed == 1"), S.CompletedCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobActiveKindTest,
    "GTC.Missions.Activities.SideJob.ActiveKindReportsCurrentJob",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobActiveKindTest::RunTest(const FString& Parameters)
{
    SJ S;
    S.Start(SJ::MakeJob(
        static_cast<int32>(SJ::EKind::Towing), FVector::ZeroVector, FVector::OneVector, 80));
    TestEqual(TEXT("kind == Towing"), S.ActiveKind(), static_cast<int32>(SJ::EKind::Towing));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobCancelTest,
    "GTC.Missions.Activities.SideJob.CancelAbortsWithoutCrediting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobCancelTest::RunTest(const FString& Parameters)
{
    SJ S;
    S.Start(SJ::MakeJob(
        static_cast<int32>(SJ::EKind::Delivery), FVector::ZeroVector, FVector::OneVector, 50));
    S.Cancel();
    TestFalse(TEXT("no active"), S.HasActive());
    TestEqual(TEXT("completed == 0"), S.CompletedCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobCompletedCountTest,
    "GTC.Missions.Activities.SideJob.CompletedCountIncrementsAcrossJobs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobCompletedCountTest::RunTest(const FString& Parameters)
{
    SJ S;
    for (int32 i = 0; i < 3; ++i)
    {
        S.Start(SJ::MakeJob(
            static_cast<int32>(SJ::EKind::Taxi), FVector::ZeroVector, FVector::OneVector, 100));
        S.AdvanceStage();
        S.Complete();
    }
    TestEqual(TEXT("completed == 3"), S.CompletedCount(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobNoActiveNoopsTest,
    "GTC.Missions.Activities.SideJob.NoActiveAdvanceAndCompleteAreNoops",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobNoActiveNoopsTest::RunTest(const FString& Parameters)
{
    SJ S;
    S.AdvanceStage();
    S.Complete();
    S.Cancel();
    TestFalse(TEXT("no active"), S.HasActive());
    TestEqual(TEXT("completed == 0"), S.CompletedCount(), 0);
    TestTrue(TEXT("stage == Done"), S.Stage() == SJ::EStage::Done);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSideJobKindNameRoundTripTest,
    "GTC.Missions.Activities.SideJob.KindNameRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSideJobKindNameRoundTripTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("taxi"), SJ::KindName(static_cast<int32>(SJ::EKind::Taxi)), FString(TEXT("taxi")));
    TestEqual(
        TEXT("vigilante"),
        SJ::KindName(static_cast<int32>(SJ::EKind::Vigilante)),
        FString(TEXT("vigilante")));
    TestEqual(TEXT("unknown == empty"), SJ::KindName(999), FString());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
