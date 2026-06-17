// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleHealth.h"
#include "../../../Tests/GtcTestTolerances.h"

#include <limits>

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_vehicle_health.gd (24 tests). Concrete numbers: max 1000, fire at 0.2.

// the reference `== INF` (exact positive-infinity) check.
static bool VehicleHealthIsPosInf(double Value)
{
    return Value == std::numeric_limits<double>::infinity();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthStartsPristineTest,
    "GTC.Vehicles.Health.StartsPristine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthStartsPristineTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    TestEqual(TEXT("health == 1000"), V.Health(), 1000.0, GtcTest::Eps);
    TestTrue(TEXT("state PRISTINE"), V.State() == FVehicleHealth::EState::Pristine);
    TestEqual(TEXT("health_fraction == 1.0"), V.HealthFraction(), 1.0, GtcTest::Eps);
    TestFalse(TEXT("not on fire"), V.IsOnFire());
    TestFalse(TEXT("not wrecked"), V.IsWrecked());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthPristineAtTwoThirdsBoundaryTest,
    "GTC.Vehicles.Health.PristineAtTwoThirdsBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthPristineAtTwoThirdsBoundaryTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(340.0); // 0.66 fraction -> still PRISTINE
    TestTrue(TEXT("state PRISTINE"), V.State() == FVehicleHealth::EState::Pristine);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthDropsToDamagedTest,
    "GTC.Vehicles.Health.DropsToDamaged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthDropsToDamagedTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(400.0); // 0.60 fraction
    TestTrue(TEXT("state DAMAGED"), V.State() == FVehicleHealth::EState::Damaged);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthDropsToSmokingTest,
    "GTC.Vehicles.Health.DropsToSmoking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthDropsToSmokingTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(750.0); // 0.25 fraction, below 0.33, above 0.2
    TestTrue(TEXT("state SMOKING"), V.State() == FVehicleHealth::EState::Smoking);
    TestFalse(TEXT("not on fire"), V.IsOnFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthDropsToOnFireTest,
    "GTC.Vehicles.Health.DropsToOnFire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthDropsToOnFireTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(850.0); // 0.15 fraction, below fire threshold
    TestTrue(TEXT("is on fire"), V.IsOnFire());
    TestTrue(TEXT("state ON_FIRE"), V.State() == FVehicleHealth::EState::OnFire);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthOnFireArmsTheFuseTest,
    "GTC.Vehicles.Health.OnFireArmsTheFuse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthOnFireArmsTheFuseTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    TestEqual(TEXT("time_to_explosion == 5.0"), V.TimeToExplosion(), 5.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthFuseIsInfBeforeFireTest,
    "GTC.Vehicles.Health.FuseIsInfBeforeFire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthFuseIsInfBeforeFireTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(400.0); // DAMAGED, not on fire
    TestTrue(TEXT("time_to_explosion == INF"), VehicleHealthIsPosInf(V.TimeToExplosion()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthTickBeforeFireIsNoopTest,
    "GTC.Vehicles.Health.TickBeforeFireIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthTickBeforeFireIsNoopTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(400.0);
    V.Tick(3.0);
    TestTrue(TEXT("state DAMAGED"), V.State() == FVehicleHealth::EState::Damaged);
    TestTrue(TEXT("time_to_explosion == INF"), VehicleHealthIsPosInf(V.TimeToExplosion()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthTickBurnsFuseDownTest,
    "GTC.Vehicles.Health.TickBurnsFuseDown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthTickBurnsFuseDownTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    V.Tick(2.0);
    TestEqual(TEXT("time_to_explosion == 3.0"), V.TimeToExplosion(), 3.0, GtcTest::Eps);
    TestTrue(TEXT("is on fire"), V.IsOnFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthTimeToExplosionDecreasesWhileBurningTest,
    "GTC.Vehicles.Health.TimeToExplosionDecreasesWhileBurning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthTimeToExplosionDecreasesWhileBurningTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    const double Before = V.TimeToExplosion();
    V.Tick(1.0);
    const double After = V.TimeToExplosion();
    TestTrue(TEXT("after < before"), After < Before);
    TestEqual(TEXT("after == 4.0"), After, 4.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthFuseElapsedWrecksTest,
    "GTC.Vehicles.Health.FuseElapsedWrecks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthFuseElapsedWrecksTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    V.Tick(5.0);
    TestTrue(TEXT("is wrecked"), V.IsWrecked());
    TestTrue(TEXT("state WRECKED"), V.State() == FVehicleHealth::EState::Wrecked);
    TestEqual(TEXT("health == 0.0"), V.Health(), 0.0, GtcTest::Eps);
    TestEqual(TEXT("time_to_explosion == 0.0"), V.TimeToExplosion(), 0.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthFuseDoesNotRearmAcrossTicksTest,
    "GTC.Vehicles.Health.FuseDoesNotRearmAcrossTicks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthFuseDoesNotRearmAcrossTicksTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    V.Tick(1.0);
    V.ApplyDamage(10.0); // still on fire; fuse must keep counting from 4.0
    TestEqual(TEXT("time_to_explosion == 4.0"), V.TimeToExplosion(), 4.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthMassiveHitInstantWreckTest,
    "GTC.Vehicles.Health.MassiveHitInstantWreck",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthMassiveHitInstantWreckTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(5000.0);
    TestTrue(TEXT("is wrecked"), V.IsWrecked());
    TestEqual(TEXT("health == 0.0"), V.Health(), 0.0, GtcTest::Eps);
    TestEqual(TEXT("time_to_explosion == 0.0"), V.TimeToExplosion(), 0.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthHealthFloorsAtZeroTest,
    "GTC.Vehicles.Health.HealthFloorsAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthHealthFloorsAtZeroTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(2000.0);
    TestEqual(TEXT("health == 0.0"), V.Health(), 0.0, GtcTest::Eps);
    TestEqual(TEXT("health_fraction == 0.0"), V.HealthFraction(), 0.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthJustExplodedIsOneShotTest,
    "GTC.Vehicles.Health.JustExplodedIsOneShot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthJustExplodedIsOneShotTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(5000.0);
    TestTrue(TEXT("just_exploded true"), V.JustExploded());
    TestFalse(TEXT("just_exploded then false"), V.JustExploded());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthJustExplodedFalseBeforeWreckTest,
    "GTC.Vehicles.Health.JustExplodedFalseBeforeWreck",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthJustExplodedFalseBeforeWreckTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(850.0); // on fire, not yet exploded
    TestFalse(TEXT("not just exploded"), V.JustExploded());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthExplosionFiresOnceViaFuseTest,
    "GTC.Vehicles.Health.ExplosionFiresOnceViaFuse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthExplosionFiresOnceViaFuseTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    V.Tick(5.0);
    TestTrue(TEXT("just exploded true"), V.JustExploded());
    TestFalse(TEXT("then false"), V.JustExploded());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthNegativeDamageIgnoredTest,
    "GTC.Vehicles.Health.NegativeDamageIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthNegativeDamageIgnoredTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(-300.0);
    TestEqual(TEXT("health == 1000"), V.Health(), 1000.0, GtcTest::Eps);
    TestTrue(TEXT("state PRISTINE"), V.State() == FVehicleHealth::EState::Pristine);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthNegativeDeltaIgnoredTest,
    "GTC.Vehicles.Health.NegativeDeltaIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthNegativeDeltaIgnoredTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    V.Tick(-2.0);
    TestEqual(TEXT("time_to_explosion == 5.0"), V.TimeToExplosion(), 5.0, GtcTest::Eps);
    TestTrue(TEXT("is on fire"), V.IsOnFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthWreckedIsStickyToDamageTest,
    "GTC.Vehicles.Health.WreckedIsStickyToDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthWreckedIsStickyToDamageTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(5000.0);
    V.JustExploded();
    V.ApplyDamage(100.0); // ignored once wrecked
    TestTrue(TEXT("is wrecked"), V.IsWrecked());
    TestEqual(TEXT("health == 0.0"), V.Health(), 0.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthWreckedIsStickyToTickTest,
    "GTC.Vehicles.Health.WreckedIsStickyToTick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthWreckedIsStickyToTickTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2, 5.0);
    V.ApplyDamage(850.0);
    V.Tick(5.0);
    V.JustExploded();
    V.Tick(3.0); // no-op; stays wrecked, no second explosion
    TestTrue(TEXT("is wrecked"), V.IsWrecked());
    TestFalse(TEXT("not just exploded again"), V.JustExploded());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthRepairRestoresPristineTest,
    "GTC.Vehicles.Health.RepairRestoresPristine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthRepairRestoresPristineTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(850.0);
    V.Repair();
    TestEqual(TEXT("health == 1000"), V.Health(), 1000.0, GtcTest::Eps);
    TestTrue(TEXT("state PRISTINE"), V.State() == FVehicleHealth::EState::Pristine);
    TestTrue(TEXT("time_to_explosion == INF"), VehicleHealthIsPosInf(V.TimeToExplosion()));
    TestFalse(TEXT("not on fire"), V.IsOnFire());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthResetRestoresAfterWreckTest,
    "GTC.Vehicles.Health.ResetRestoresAfterWreck",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthResetRestoresAfterWreckTest::RunTest(const FString& Parameters)
{
    FVehicleHealth V(1000.0, 0.2);
    V.ApplyDamage(5000.0);
    V.JustExploded();
    V.Reset();
    TestEqual(TEXT("health == 1000"), V.Health(), 1000.0, GtcTest::Eps);
    TestTrue(TEXT("state PRISTINE"), V.State() == FVehicleHealth::EState::Pristine);
    TestFalse(TEXT("not wrecked"), V.IsWrecked());
    TestFalse(TEXT("not just exploded"), V.JustExploded());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHealthFullBurnDownTimelineTest,
    "GTC.Vehicles.Health.FullBurnDownTimeline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleHealthFullBurnDownTimelineTest::RunTest(const FString& Parameters)
{
    // PRISTINE -> DAMAGED -> SMOKING -> ON_FIRE -> (fuse) -> WRECKED
    FVehicleHealth V(1000.0, 0.2, 4.0);
    TestTrue(TEXT("pristine"), V.State() == FVehicleHealth::EState::Pristine);
    V.ApplyDamage(400.0);
    TestTrue(TEXT("damaged"), V.State() == FVehicleHealth::EState::Damaged);
    V.ApplyDamage(350.0); // 0.25 -> SMOKING
    TestTrue(TEXT("smoking"), V.State() == FVehicleHealth::EState::Smoking);
    V.ApplyDamage(100.0); // 0.15 -> ON_FIRE
    TestTrue(TEXT("on fire"), V.IsOnFire());
    V.Tick(4.0);
    TestTrue(TEXT("wrecked"), V.IsWrecked());
    TestTrue(TEXT("just exploded"), V.JustExploded());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
