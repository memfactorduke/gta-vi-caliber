// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleStunt.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FVehicleStuntDetector — wheelie/stoppie/jump detection and completion.
 * Prefix GTC.Vehicles.Stunt.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleStuntTest,
    "GTC.Vehicles.Stunt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleStuntTest::RunTest(const FString& Parameters)
{
    const double Up = FVehicleStuntDetector::WheelieMinPitchRad + 0.1;
    const double Down = -(FVehicleStuntDetector::StoppieMinPitchRad + 0.1);

    // --- A sustained wheelie completes when the nose drops ---
    {
        FVehicleStuntDetector D;
        for (int32 i = 0; i < 8; ++i) // 0.8s of wheelie
        {
            const FStuntDetection R = D.Update(0.1, Up, /*airborne*/ false, 0.0);
            TestFalse(TEXT("no completion while the wheelie holds"), R.bCompleted);
        }
        TestTrue(TEXT("wheelie is active"), D.IsActive());
        const FStuntDetection End = D.Update(0.1, 0.0, false, 0.0); // level out
        TestTrue(TEXT("wheelie completes on level-out"), End.bCompleted);
        TestTrue(TEXT("completed trick is a wheelie"), End.Kind == EVehicleStunt::Wheelie);
        TestTrue(TEXT("wheelie magnitude is the seconds held"), End.Magnitude >= 0.8 - GtcTest::Eps);
    }

    // --- A momentary blip is not a trick ---
    {
        FVehicleStuntDetector D;
        D.Update(0.1, Up, false, 0.0);
        D.Update(0.1, Up, false, 0.0); // only 0.2s, under MinTrickSeconds
        const FStuntDetection End = D.Update(0.1, 0.0, false, 0.0);
        TestFalse(TEXT("a sub-threshold wheelie is discarded"), End.bCompleted);
    }

    // --- Stoppie ---
    {
        FVehicleStuntDetector D;
        for (int32 i = 0; i < 6; ++i) { D.Update(0.1, Down, false, 0.0); }
        const FStuntDetection End = D.Update(0.1, 0.0, false, 0.0);
        TestTrue(TEXT("stoppie completes"), End.bCompleted);
        TestTrue(TEXT("completed trick is a stoppie"), End.Kind == EVehicleStunt::Stoppie);
    }

    // --- A jump reports its peak height on landing ---
    {
        FVehicleStuntDetector D;
        D.Update(0.1, 0.0, /*airborne*/ true, 1.0);
        D.Update(0.1, 0.0, true, 3.0); // peak
        D.Update(0.1, 0.0, true, 2.0);
        const FStuntDetection Land = D.Update(0.1, 0.0, /*airborne*/ false, 0.0);
        TestTrue(TEXT("jump completes on landing"), Land.bCompleted);
        TestTrue(TEXT("completed trick is a jump"), Land.Kind == EVehicleStunt::Jump);
        TestTrue(TEXT("jump magnitude is the peak height"),
            FMath::IsNearlyEqual(Land.Magnitude, 3.0, GtcTest::Eps));
    }

    // --- A tiny hop (below MinJumpHeight) is not a jump ---
    {
        FVehicleStuntDetector D;
        D.Update(0.1, 0.0, /*airborne*/ true, 0.1);
        D.Update(0.1, 0.0, true, 0.2); // never gets real air
        const FStuntDetection Land = D.Update(0.1, 0.0, false, 0.0);
        TestFalse(TEXT("a low hop is not banked as a jump"), Land.bCompleted);
    }

    // --- Transitioning wheelie -> jump banks the wheelie first ---
    {
        FVehicleStuntDetector D;
        for (int32 i = 0; i < 6; ++i) { D.Update(0.1, Up, false, 0.0); }
        const FStuntDetection OnLaunch = D.Update(0.1, Up, /*airborne*/ true, 1.0);
        TestTrue(TEXT("launching mid-wheelie banks the wheelie"), OnLaunch.bCompleted);
        TestTrue(TEXT("the banked trick was the wheelie"), OnLaunch.Kind == EVehicleStunt::Wheelie);
        TestTrue(TEXT("now mid-jump"), D.Current == EVehicleStunt::Jump);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
