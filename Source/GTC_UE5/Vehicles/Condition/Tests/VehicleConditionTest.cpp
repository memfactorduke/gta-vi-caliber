// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleCondition.h"
#include "../ChopShop.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_vehicle_condition.gd (23 tests). sedan: tank 60L, economy 0.0009 L/m.
// One test (ConditionBlendsWearAndFeedsChopShop) composes FChopShop, mirroring the cross-ref oracle.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDefaultVehiclesLoadedTest,
    "GTC.Vehicles.Condition.DefaultVehiclesLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDefaultVehiclesLoadedTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    TestTrue(TEXT("vehicle_count >= 3"), VC.VehicleCount() >= 3);
    TestTrue(TEXT("has sedan"), VC.HasVehicle(TEXT("sedan")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionMalformedVehiclesDroppedTest,
    "GTC.Vehicles.Condition.MalformedVehiclesDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionMalformedVehiclesDroppedTest::RunTest(const FString& Parameters)
{
    TArray<FVehicleCondition::FVehicleDef> Defs;
    Defs.Add({TEXT("ok"), 50.0});
    Defs.Add({TEXT(""), 50.0});       // empty id
    Defs.Add({TEXT(""), 40.0});       // no id (modeled as empty)
    Defs.Add({TEXT("zero"), 0.0});    // non-positive tank
    Defs.Add({TEXT("ok"), 99.0});     // duplicate id
    FVehicleCondition VC(Defs);
    TestEqual(TEXT("vehicle_count == 1"), VC.VehicleCount(), 1);
    TestTrue(TEXT("has ok"), VC.HasVehicle(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionNewVehicleStartsFullAndUnwornTest,
    "GTC.Vehicles.Condition.NewVehicleStartsFullAndUnworn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionNewVehicleStartsFullAndUnwornTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    TestEqual(TEXT("fuel_fraction == 1.0"), VC.FuelFraction(TEXT("sedan")), 1.0, GtcTest::Eps);
    TestEqual(TEXT("engine_wear == 0.0"), VC.EngineWearOf(TEXT("sedan")), 0.0);
    TestEqual(TEXT("tire_wear == 0.0"), VC.TireWearOf(TEXT("sedan")), 0.0);
    TestEqual(TEXT("condition == 1.0"), VC.Condition(TEXT("sedan")), 1.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionUnknownVehicleIsNeutralTest,
    "GTC.Vehicles.Condition.UnknownVehicleIsNeutral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionUnknownVehicleIsNeutralTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    TestEqual(TEXT("condition == 1.0"), VC.Condition(TEXT("nope")), 1.0);
    TestEqual(TEXT("fuel_of == 0.0"), VC.FuelOf(TEXT("nope")), 0.0);
    TestEqual(TEXT("top_speed_factor == 1.0"), VC.TopSpeedFactor(TEXT("nope")), 1.0);
    TestEqual(TEXT("grip_factor == 1.0"), VC.GripFactor(TEXT("nope")), 1.0);
    TestEqual(TEXT("refuel == 0.0"), VC.Refuel(TEXT("nope")), 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveBurnsFuelByEconomyTest,
    "GTC.Vehicles.Condition.DriveBurnsFuelByEconomy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveBurnsFuelByEconomyTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    const double Before = VC.FuelOf(TEXT("sedan"));
    const FVehicleCondition::FDriveResult R = VC.Drive(TEXT("sedan"), 1000.0); // 0.0009 * 1000 = 0.9 L
    const double Burned = Before - VC.FuelOf(TEXT("sedan"));
    TestEqual(TEXT("burned == 0.9"), Burned, 0.9, GtcTest::Eps);
    TestEqual(TEXT("fuel_used == 0.9"), R.FuelUsed, 0.9, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveAccruesEngineAndTireWearTest,
    "GTC.Vehicles.Condition.DriveAccruesEngineAndTireWear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveAccruesEngineAndTireWearTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.Drive(TEXT("sedan"), 1000.0, 1.0);
    VC.Drive(TEXT("sports"), 1000.0, 2.0); // harder driving over the same distance
    TestTrue(TEXT("sedan engine wear > 0"), VC.EngineWearOf(TEXT("sedan")) > 0.0);
    TestTrue(TEXT("sedan tire wear > 0"), VC.TireWearOf(TEXT("sedan")) > 0.0);
    TestTrue(TEXT("sports engine > sedan engine"),
        VC.EngineWearOf(TEXT("sports")) > VC.EngineWearOf(TEXT("sedan")));
    TestTrue(TEXT("sports tire > sedan tire"),
        VC.TireWearOf(TEXT("sports")) > VC.TireWearOf(TEXT("sedan")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveStopsAtEmptyAndStallsTest,
    "GTC.Vehicles.Condition.DriveStopsAtEmptyAndStalls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveStopsAtEmptyAndStallsTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    const FVehicleCondition::FDriveResult R = VC.Drive(TEXT("sedan"), 9999999.0); // far beyond tank
    TestEqual(TEXT("fuel == 0.0"), VC.FuelOf(TEXT("sedan")), 0.0, GtcTest::Eps);
    TestTrue(TEXT("is out of fuel"), VC.IsOutOfFuel(TEXT("sedan")));
    TestTrue(TEXT("stalled"), R.bStalled);
    TestTrue(TEXT("fuel >= 0"), VC.FuelOf(TEXT("sedan")) >= 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionTopSpeedFactorDropsWithEngineWearTest,
    "GTC.Vehicles.Condition.TopSpeedFactorDropsWithEngineWear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionTopSpeedFactorDropsWithEngineWearTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.ApplyCrash(TEXT("sedan"), 1.0);
    VC.ApplyCrash(TEXT("sedan"), 1.0); // engine_wear ~0.8
    const double WornFactor = VC.TopSpeedFactor(TEXT("sedan"));
    VC.Drive(TEXT("sedan"), 9999999.0); // drain the tank
    const double EmptyFactor = VC.TopSpeedFactor(TEXT("sedan"));
    TestTrue(TEXT("worn < 1.0"), WornFactor < 1.0);
    TestTrue(TEXT("worn >= ENGINE_FLOOR"), WornFactor >= FVehicleCondition::EngineFloor);
    TestEqual(TEXT("empty == 0.0"), EmptyFactor, 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionGripFactorDropsWithTireWearTest,
    "GTC.Vehicles.Condition.GripFactorDropsWithTireWear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionGripFactorDropsWithTireWearTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    const double Fresh = VC.GripFactor(TEXT("sedan"));
    VC.Drive(TEXT("sedan"), 30000.0, 1.0); // tire wear ~0.9
    const double Worn = VC.GripFactor(TEXT("sedan"));
    TestEqual(TEXT("fresh == 1.0"), Fresh, 1.0, GtcTest::Eps);
    TestTrue(TEXT("worn < 1.0"), Worn < 1.0);
    TestTrue(TEXT("worn >= TIRE_FLOOR"), Worn >= FVehicleCondition::TireFloor);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionApplyCrashIncreasesEngineWearClampedTest,
    "GTC.Vehicles.Condition.ApplyCrashIncreasesEngineWearClamped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionApplyCrashIncreasesEngineWearClampedTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    const double Before = VC.EngineWearOf(TEXT("sedan"));
    VC.ApplyCrash(TEXT("sedan"), 0.5);
    const double After = VC.EngineWearOf(TEXT("sedan"));
    for (int32 i = 0; i < 10; ++i)
    {
        VC.ApplyCrash(TEXT("sedan"), 1.0);
    }
    const double Maxed = VC.EngineWearOf(TEXT("sedan"));
    const double CondFloor = VC.Condition(TEXT("sedan"));
    VC.ApplyCrash(TEXT("sedan"), 1.0); // already maxed: no further change
    const double CondAfter = VC.Condition(TEXT("sedan"));
    TestTrue(TEXT("after > before"), After > Before);
    TestEqual(TEXT("maxed == 1.0"), Maxed, 1.0, GtcTest::Eps);
    TestEqual(TEXT("cond_floor == cond_after"), CondFloor, CondAfter, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionConditionBlendsWearAndFeedsChopShopTest,
    "GTC.Vehicles.Condition.ConditionBlendsWearAndFeedsChopShop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionConditionBlendsWearAndFeedsChopShopTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    const double Pristine = VC.Condition(TEXT("sedan"));
    VC.Drive(TEXT("sedan"), 50000.0, 2.0);
    VC.ApplyCrash(TEXT("sedan"), 0.5);
    const double Worn = VC.Condition(TEXT("sedan"));
    FChopShop Shop;
    // A worn car fences for less — the named FChopShop::Value() composition.
    TestTrue(TEXT("worn < pristine"), Worn < Pristine);
    TestTrue(TEXT("worn value < pristine value"),
        Shop.Value(TEXT("sedan"), Worn) < Shop.Value(TEXT("sedan"), Pristine));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionRefuelCapsAtTankAndReturnsAddedTest,
    "GTC.Vehicles.Condition.RefuelCapsAtTankAndReturnsAdded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionRefuelCapsAtTankAndReturnsAddedTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.Drive(TEXT("sedan"), 20000.0); // burn 18 L, leaving 42
    const double Added = VC.Refuel(TEXT("sedan"), -1.0); // fill to full -> add exactly 18
    const double Second = VC.Refuel(TEXT("sedan"), -1.0); // already full -> 0
    TestEqual(TEXT("added == 18.0"), Added, 18.0, GtcTest::Eps);
    TestEqual(TEXT("fuel_fraction == 1.0"), VC.FuelFraction(TEXT("sedan")), 1.0, GtcTest::Eps);
    TestEqual(TEXT("second == 0.0"), Second, 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionServiceResetsSelectedChannelsTest,
    "GTC.Vehicles.Condition.ServiceResetsSelectedChannels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionServiceResetsSelectedChannelsTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.Drive(TEXT("sedan"), 30000.0, 2.0); // both channels wear
    VC.Service(TEXT("sedan"), true, false); // engine only
    const bool bEngineZeroed = VC.EngineWearOf(TEXT("sedan")) == 0.0;
    const bool bTireKept = VC.TireWearOf(TEXT("sedan")) > 0.0;
    VC.Service(TEXT("sedan")); // defaults: both
    TestTrue(TEXT("engine zeroed"), bEngineZeroed);
    TestTrue(TEXT("tire kept"), bTireKept);
    TestEqual(TEXT("tire now 0"), VC.TireWearOf(TEXT("sedan")), 0.0);
    TestEqual(TEXT("engine still 0"), VC.EngineWearOf(TEXT("sedan")), 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionSaveRoundtripTest,
    "GTC.Vehicles.Condition.SaveRoundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionSaveRoundtripTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.Drive(TEXT("sedan"), 15000.0, 1.5);
    VC.ApplyCrash(TEXT("sedan"), 0.3);
    const TArray<FVehicleCondition::FVehicleSave> Snap = VC.ToState();
    FVehicleCondition Fresh;
    Fresh.LoadState(Snap);
    TestEqual(TEXT("fuel match"), Fresh.FuelOf(TEXT("sedan")), VC.FuelOf(TEXT("sedan")), GtcTest::Eps);
    TestEqual(TEXT("engine match"),
        Fresh.EngineWearOf(TEXT("sedan")), VC.EngineWearOf(TEXT("sedan")), GtcTest::Eps);
    TestEqual(TEXT("tire match"),
        Fresh.TireWearOf(TEXT("sedan")), VC.TireWearOf(TEXT("sedan")), GtcTest::Eps);

    // Unknown ids and non-numeric values are ignored (sedan stays pristine).
    FVehicleCondition Bad;
    TArray<FVehicleCondition::FVehicleSave> BadData;
    FVehicleCondition::FVehicleSave Ghost;
    Ghost.Id = TEXT("ghost"); // unknown id -> ignored
    Ghost.Fuel = 10.0;
    Ghost.bHasEngineWear = false;
    Ghost.bHasTireWear = false;
    BadData.Add(Ghost);
    FVehicleCondition::FVehicleSave SedanBad;
    SedanBad.Id = TEXT("sedan");
    SedanBad.bHasFuel = false;        // "lots" non-numeric -> ignored
    SedanBad.bHasEngineWear = false;
    SedanBad.bHasTireWear = false;
    BadData.Add(SedanBad);
    Bad.LoadState(BadData);
    TestEqual(TEXT("bad engine still 0"), Bad.EngineWearOf(TEXT("sedan")), 0.0);
    TestEqual(TEXT("bad fuel_fraction 1.0"), Bad.FuelFraction(TEXT("sedan")), 1.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveIntensityZeroStillCostsTest,
    "GTC.Vehicles.Condition.DriveIntensityZeroStillCosts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveIntensityZeroStillCostsTest::RunTest(const FString& Parameters)
{
    // Movement always costs: intensity 0 is floored to MIN_INTENSITY (no free driving).
    FVehicleCondition VC;
    const double Before = VC.FuelOf(TEXT("sedan"));
    VC.Drive(TEXT("sedan"), 5000.0, 0.0);
    TestTrue(TEXT("fuel < before"), VC.FuelOf(TEXT("sedan")) < Before);
    TestTrue(TEXT("engine wear > 0"), VC.EngineWearOf(TEXT("sedan")) > 0.0);
    TestTrue(TEXT("tire wear > 0"), VC.TireWearOf(TEXT("sedan")) > 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveOnStalledVehicleIsNoopTest,
    "GTC.Vehicles.Condition.DriveOnStalledVehicleIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveOnStalledVehicleIsNoopTest::RunTest(const FString& Parameters)
{
    // Tiny tank so it empties with negligible wear, making the no-op assertion meaningful.
    TArray<FVehicleCondition::FVehicleDef> Defs;
    Defs.Add({TEXT("test"), 5.0, 1.0});
    FVehicleCondition VC(Defs);
    VC.Drive(TEXT("test"), 5.0, 1.0); // burns all 5 L; now stalled
    const double Ew = VC.EngineWearOf(TEXT("test"));
    const double Tw = VC.TireWearOf(TEXT("test"));
    const FVehicleCondition::FDriveResult R = VC.Drive(TEXT("test"), 1000.0, 1.0); // no move -> no wear
    TestTrue(TEXT("is out of fuel"), VC.IsOutOfFuel(TEXT("test")));
    TestEqual(TEXT("engine unchanged"), VC.EngineWearOf(TEXT("test")), Ew, GtcTest::Eps);
    TestEqual(TEXT("tire unchanged"), VC.TireWearOf(TEXT("test")), Tw, GtcTest::Eps);
    TestEqual(TEXT("fuel_used == 0.0"), R.FuelUsed, 0.0);
    TestTrue(TEXT("stalled"), R.bStalled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveFuelUsedReportsActualDrainTest,
    "GTC.Vehicles.Condition.DriveFuelUsedReportsActualDrain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveFuelUsedReportsActualDrainTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.Drive(TEXT("sedan"), 60000.0); // burn 54 L, leaving 6
    const FVehicleCondition::FDriveResult R = VC.Drive(TEXT("sedan"), 100000.0); // want 90, only 6 left
    TestEqual(TEXT("fuel_used == 6.0"), R.FuelUsed, 6.0, GtcTest::Eps);
    TestEqual(TEXT("fuel == 0.0"), VC.FuelOf(TEXT("sedan")), 0.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveUnknownVehicleNeutralDictTest,
    "GTC.Vehicles.Condition.DriveUnknownVehicleNeutralDict",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveUnknownVehicleNeutralDictTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    const int32 Count = VC.VehicleCount();
    const FVehicleCondition::FDriveResult R = VC.Drive(TEXT("nope"), 1000.0);
    TestEqual(TEXT("fuel_used == 0.0"), R.FuelUsed, 0.0);
    TestFalse(TEXT("stalled == false"), R.bStalled);
    TestEqual(TEXT("count unchanged"), VC.VehicleCount(), Count);
    TestFalse(TEXT("not has nope"), VC.HasVehicle(TEXT("nope")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionDriveNonpositiveDistanceNoopTest,
    "GTC.Vehicles.Condition.DriveNonpositiveDistanceNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionDriveNonpositiveDistanceNoopTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    const double Before = VC.FuelOf(TEXT("sedan"));
    const FVehicleCondition::FDriveResult R = VC.Drive(TEXT("sedan"), 0.0);
    VC.Drive(TEXT("sedan"), -50.0);
    TestEqual(TEXT("fuel unchanged"), VC.FuelOf(TEXT("sedan")), Before, GtcTest::Eps);
    TestEqual(TEXT("fuel_used == 0.0"), R.FuelUsed, 0.0);
    TestEqual(TEXT("engine wear == 0"), VC.EngineWearOf(TEXT("sedan")), 0.0);
    TestEqual(TEXT("tire wear == 0"), VC.TireWearOf(TEXT("sedan")), 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionRefuelPartialAmountTest,
    "GTC.Vehicles.Condition.RefuelPartialAmount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionRefuelPartialAmountTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.Drive(TEXT("sedan"), 20000.0); // burn 18 L, leaving 42
    const double Added = VC.Refuel(TEXT("sedan"), 10.0);
    TestEqual(TEXT("added == 10.0"), Added, 10.0, GtcTest::Eps);
    TestEqual(TEXT("fuel == 52.0"), VC.FuelOf(TEXT("sedan")), 52.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionRefuelClampsTooLargeTest,
    "GTC.Vehicles.Condition.RefuelClampsTooLarge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionRefuelClampsTooLargeTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.Drive(TEXT("sedan"), 20000.0); // 42 L left, 18 L of space
    const double Added = VC.Refuel(TEXT("sedan"), 100.0);
    TestEqual(TEXT("added == 18.0"), Added, 18.0, GtcTest::Eps);
    TestEqual(TEXT("fuel_fraction == 1.0"), VC.FuelFraction(TEXT("sedan")), 1.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionLoadStateClampsOutOfRangeTest,
    "GTC.Vehicles.Condition.LoadStateClampsOutOfRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionLoadStateClampsOutOfRangeTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    TArray<FVehicleCondition::FVehicleSave> Data;
    FVehicleCondition::FVehicleSave Row;
    Row.Id = TEXT("sedan");
    Row.Fuel = 999.0;
    Row.EngineWear = 5.0;
    Row.TireWear = -2.0;
    Data.Add(Row);
    VC.LoadState(Data);
    TestEqual(TEXT("fuel clamped to 60"), VC.FuelOf(TEXT("sedan")), 60.0, GtcTest::Eps);
    TestEqual(TEXT("engine clamped to 1.0"), VC.EngineWearOf(TEXT("sedan")), 1.0, GtcTest::Eps);
    TestEqual(TEXT("tire clamped to 0.0"), VC.TireWearOf(TEXT("sedan")), 0.0, GtcTest::Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleConditionApplyCrashUnknownAndNonpositiveNoopTest,
    "GTC.Vehicles.Condition.ApplyCrashUnknownAndNonpositiveNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FVehicleConditionApplyCrashUnknownAndNonpositiveNoopTest::RunTest(const FString& Parameters)
{
    FVehicleCondition VC;
    VC.ApplyCrash(TEXT("nope"), 1.0); // must not crash or register a vehicle
    const double Before = VC.EngineWearOf(TEXT("sedan"));
    VC.ApplyCrash(TEXT("sedan"), 0.0);
    VC.ApplyCrash(TEXT("sedan"), -0.5);
    TestFalse(TEXT("not has nope"), VC.HasVehicle(TEXT("nope")));
    TestEqual(TEXT("engine unchanged"), VC.EngineWearOf(TEXT("sedan")), Before);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
