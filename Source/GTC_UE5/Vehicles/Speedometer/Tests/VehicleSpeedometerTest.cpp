// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleSpeedometer.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Unit tests for FVehicleSpeedometer — the pure-core speedo HUD metrics. Pins the
 * km/h + mph unit conversions, needle and RPM fractions (clamping + degenerate-max
 * guards), the redline-flash boundary tie, and the P/R/N/forward-gear label.
 * Prefix GTC.Vehicles.Speedometer.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleSpeedometerTest,
    "GTC.Vehicles.Speedometer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleSpeedometerTest::RunTest(const FString& Parameters)
{
    using EGear = FVehicleSpeedometer::EGear;

    // --- Unit conversions (input is cm/s) -------------------------------------
    // 1000 cm/s == 10 m/s == 36 km/h.
    TestTrue(TEXT("1000 cm/s is 36 km/h"),
        FMath::Abs(FVehicleSpeedometer::SpeedKmPerHour(1000.0) - 36.0) <= Eps);
    // 36 km/h == 36 / 1.609344 mph ~= 22.3694 mph.
    TestTrue(TEXT("1000 cm/s is ~22.369 mph"),
        FMath::Abs(FVehicleSpeedometer::SpeedMilesPerHour(1000.0) - (36.0 / 1.609344)) <= Eps);
    // Zero speed reads zero on both dials; both conversions are linear through origin.
    TestTrue(TEXT("0 cm/s is 0 km/h"), FMath::Abs(FVehicleSpeedometer::SpeedKmPerHour(0.0)) <= Eps);
    TestTrue(TEXT("0 cm/s is 0 mph"), FMath::Abs(FVehicleSpeedometer::SpeedMilesPerHour(0.0)) <= Eps);
    // A reversing (negative) speed converts to a negative reading (sign preserved here;
    // the needle takes the magnitude separately, below).
    TestTrue(TEXT("negative speed converts negative"),
        FVehicleSpeedometer::SpeedKmPerHour(-1000.0) < 0.0);

    // --- Needle fraction (magnitude / max, clamped) ---------------------------
    TestTrue(TEXT("half-max needle is 0.5"),
        FMath::Abs(FVehicleSpeedometer::NeedleFraction(50.0, 100.0) - 0.5) <= Eps);
    TestTrue(TEXT("at-max needle saturates to 1"),
        FMath::Abs(FVehicleSpeedometer::NeedleFraction(100.0, 100.0) - 1.0) <= Eps);
    TestTrue(TEXT("over-max needle clamps to 1"),
        FMath::Abs(FVehicleSpeedometer::NeedleFraction(250.0, 100.0) - 1.0) <= Eps);
    TestTrue(TEXT("zero speed needle is 0"),
        FMath::Abs(FVehicleSpeedometer::NeedleFraction(0.0, 100.0)) <= Eps);
    // Reversing still sweeps the needle (magnitude).
    TestTrue(TEXT("reversing sweeps the needle by magnitude"),
        FMath::Abs(FVehicleSpeedometer::NeedleFraction(-50.0, 100.0) - 0.5) <= Eps);
    // Degenerate max => flat needle, no divide blow-up.
    TestTrue(TEXT("zero max needle is 0, not NaN"),
        FMath::Abs(FVehicleSpeedometer::NeedleFraction(50.0, 0.0)) <= Eps);
    TestTrue(TEXT("negative max needle is 0"),
        FMath::Abs(FVehicleSpeedometer::NeedleFraction(50.0, -100.0)) <= Eps);

    // --- RPM fraction + redline flash -----------------------------------------
    TestTrue(TEXT("half-redline rpm is 0.5"),
        FMath::Abs(FVehicleSpeedometer::RpmFraction(3500.0, 7000.0) - 0.5) <= Eps);
    TestTrue(TEXT("over-redline rpm clamps to 1"),
        FMath::Abs(FVehicleSpeedometer::RpmFraction(9000.0, 7000.0) - 1.0) <= Eps);
    TestTrue(TEXT("negative rpm clamps to 0"),
        FMath::Abs(FVehicleSpeedometer::RpmFraction(-500.0, 7000.0)) <= Eps);
    TestTrue(TEXT("zero redline rpm fraction is 0"),
        FMath::Abs(FVehicleSpeedometer::RpmFraction(3500.0, 0.0)) <= Eps);

    // Redline flash: the boundary tie (rpm == redline) latches ON.
    TestFalse(TEXT("below redline does not flash"),
        FVehicleSpeedometer::IsRedlineFlashing(6999.0, 7000.0));
    TestTrue(TEXT("exactly at redline flashes (tie included)"),
        FVehicleSpeedometer::IsRedlineFlashing(7000.0, 7000.0));
    TestTrue(TEXT("past redline flashes"),
        FVehicleSpeedometer::IsRedlineFlashing(8000.0, 7000.0));
    // A degenerate redline never latches the warning on, even though rpm >= 0.
    TestFalse(TEXT("zero redline never flashes"),
        FVehicleSpeedometer::IsRedlineFlashing(8000.0, 0.0));
    TestFalse(TEXT("negative redline never flashes"),
        FVehicleSpeedometer::IsRedlineFlashing(8000.0, -1.0));

    // --- Gear label -----------------------------------------------------------
    TestEqual(TEXT("park label"), FVehicleSpeedometer::GearLabel(EGear::Park, 0), FString(TEXT("P")));
    TestEqual(TEXT("reverse label"), FVehicleSpeedometer::GearLabel(EGear::Reverse, 0), FString(TEXT("R")));
    TestEqual(TEXT("neutral label"), FVehicleSpeedometer::GearLabel(EGear::Neutral, 0), FString(TEXT("N")));
    // Drive shows the forward-gear number.
    TestEqual(TEXT("drive gear 3 label"), FVehicleSpeedometer::GearLabel(EGear::Drive, 3), FString(TEXT("3")));
    // A not-yet-engaged forward "0" still reads as "1".
    TestEqual(TEXT("drive gear 0 reads as 1"), FVehicleSpeedometer::GearLabel(EGear::Drive, 0), FString(TEXT("1")));
    // A negative forward number is also floored to "1".
    TestEqual(TEXT("drive negative gear floors to 1"), FVehicleSpeedometer::GearLabel(EGear::Drive, -2), FString(TEXT("1")));
    // P/R/N ignore the forward-gear number entirely.
    TestEqual(TEXT("park ignores forward number"), FVehicleSpeedometer::GearLabel(EGear::Park, 5), FString(TEXT("P")));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
