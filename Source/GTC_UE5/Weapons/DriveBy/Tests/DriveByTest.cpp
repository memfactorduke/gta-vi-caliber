// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../DriveBy.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FDriveBy — forward firing block, firing side, and speed spread.
 * Prefix GTC.Weapons.DriveBy.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDriveByTest,
    "GTC.Weapons.DriveBy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDriveByTest::RunTest(const FString& Parameters)
{
    const FVector FwdDir(1, 0, 0);
    const FVector RightDir(0, 1, 0);

    // --- AimAngleFromForward ---
    TestTrue(TEXT("aiming ahead -> ~0"),
        FMath::IsNearlyEqual(FDriveBy::AimAngleFromForward(FwdDir, FVector(1, 0, 0)), 0.0, GtcTest::Eps));
    TestTrue(TEXT("aiming to the side -> ~pi/2"),
        FMath::IsNearlyEqual(FDriveBy::AimAngleFromForward(FwdDir, FVector(0, 1, 0)), PI / 2.0, GtcTest::Eps));
    TestTrue(TEXT("aiming back -> ~pi"),
        FMath::IsNearlyEqual(FDriveBy::AimAngleFromForward(FwdDir, FVector(-1, 0, 0)), PI, GtcTest::Eps));

    // --- CanFire: forward blocked, sides/back allowed ---
    TestFalse(TEXT("can't fire forward through your own car"),
        FDriveBy::CanFire(FwdDir, FVector(1, 0, 0)));
    TestTrue(TEXT("can fire to the side"), FDriveBy::CanFire(FwdDir, FVector(0, 1, 0)));
    TestTrue(TEXT("can fire to the rear"), FDriveBy::CanFire(FwdDir, FVector(-1, 0, 0)));
    TestFalse(TEXT("a degenerate aim can't fire"), FDriveBy::CanFire(FwdDir, FVector::ZeroVector));

    // --- FiringSide ---
    TestTrue(TEXT("aim right -> +1"),
        FMath::IsNearlyEqual(FDriveBy::FiringSide(RightDir, FVector(0, 1, 0)), 1.0, GtcTest::Eps));
    TestTrue(TEXT("aim left -> -1"),
        FMath::IsNearlyEqual(FDriveBy::FiringSide(RightDir, FVector(0, -1, 0)), -1.0, GtcTest::Eps));
    TestTrue(TEXT("aim straight ahead -> 0"),
        FMath::IsNearlyEqual(FDriveBy::FiringSide(RightDir, FVector(1, 0, 0)), 0.0, GtcTest::Eps));

    // --- SpeedSpread / EffectiveSpread ---
    TestEqual(TEXT("no speed spread at a standstill"), FDriveBy::SpeedSpread(0.0), 0.0);
    TestTrue(TEXT("full speed gives the max spread"),
        FMath::IsNearlyEqual(FDriveBy::SpeedSpread(1.0), FDriveBy::MaxSpeedSpreadRad, GtcTest::Eps));
    TestTrue(TEXT("speed spread grows with speed"),
        FDriveBy::SpeedSpread(0.3) < FDriveBy::SpeedSpread(0.8));
    TestTrue(TEXT("over-speed clamps"),
        FMath::IsNearlyEqual(FDriveBy::SpeedSpread(5.0), FDriveBy::MaxSpeedSpreadRad, GtcTest::Eps));

    const double Base = 0.02;
    TestTrue(TEXT("effective spread equals base at rest"),
        FMath::IsNearlyEqual(FDriveBy::EffectiveSpread(Base, 0.0), Base, GtcTest::Eps));
    TestTrue(TEXT("driving widens the spread"),
        FDriveBy::EffectiveSpread(Base, 1.0) > Base);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
