// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Ballistics.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

// Each test maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_ballistics.gd. Float/vector compares use Eps (1e-4),
// mirroring is_equal_approx. The model stays in the the reference frame (forward = -Z).
namespace
{
    using GtcTest::Eps;

    const FVector Fwd(0, 0, -1);
    const FVector Right(1, 0, 0);
    const FVector Up(0, 1, 0);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsZeroSpreadTest,
    "GTC.Weapons.Ballistics.ZeroSpreadReturnsForward",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsZeroSpreadTest::RunTest(const FString& Parameters)
{
    const FVector Dir = FBallistics::SpreadDirection(Fwd, Right, Up, FVector2D(1, 1), 0.0);
    TestTrue(TEXT("zero spread returns forward"), Dir.Equals(Fwd, Eps));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsZeroSampleTest,
    "GTC.Weapons.Ballistics.ZeroSampleReturnsForward",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsZeroSampleTest::RunTest(const FString& Parameters)
{
    const FVector Dir = FBallistics::SpreadDirection(Fwd, Right, Up, FVector2D::ZeroVector, 0.1);
    TestTrue(TEXT("zero sample returns forward"), Dir.Equals(Fwd, Eps));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsNormalizedTest,
    "GTC.Weapons.Ballistics.SpreadResultIsNormalized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsNormalizedTest::RunTest(const FString& Parameters)
{
    const FVector Dir = FBallistics::SpreadDirection(Fwd, Right, Up, FVector2D(0.7, -0.3), 0.15);
    TestEqual(TEXT("result is unit length"), Dir.Length(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsPushesTowardAxisTest,
    "GTC.Weapons.Ballistics.SpreadPushesTowardSampleAxis",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsPushesTowardAxisTest::RunTest(const FString& Parameters)
{
    // A positive x sample should tilt the shot to +x.
    const FVector Dir = FBallistics::SpreadDirection(Fwd, Right, Up, FVector2D(1, 0), 0.1);
    TestTrue(TEXT("dir.x > 0"), Dir.X > 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsWiderTiltsFurtherTest,
    "GTC.Weapons.Ballistics.WiderSpreadTiltsFurther",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsWiderTiltsFurtherTest::RunTest(const FString& Parameters)
{
    const FVector Narrow = FBallistics::SpreadDirection(Fwd, Right, Up, FVector2D(1, 0), 0.05);
    const FVector Wide = FBallistics::SpreadDirection(Fwd, Right, Up, FVector2D(1, 0), 0.15);
    TestTrue(TEXT("wide.x > narrow.x"), Wide.X > Narrow.X);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsFullDamageInsideTest,
    "GTC.Weapons.Ballistics.FullDamageInsideFalloff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsFullDamageInsideTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("full damage inside"), FBallistics::DamageAtRange(20.0, 10.0, 25.0, 90.0, 0.5), 20.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsMinDamageBeyondTest,
    "GTC.Weapons.Ballistics.MinDamageBeyondFalloff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsMinDamageBeyondTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("min damage beyond"), FBallistics::DamageAtRange(20.0, 100.0, 25.0, 90.0, 0.5), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsLerpsInBandTest,
    "GTC.Weapons.Ballistics.DamageLerpsInBand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsLerpsInBandTest::RunTest(const FString& Parameters)
{
    // Midpoint of [25, 90] → halfway between full (20) and min (10) = 15.
    const double Mid = FBallistics::DamageAtRange(20.0, 57.5, 25.0, 90.0, 0.5);
    TestEqual(TEXT("midpoint lerp == 15"), Mid, 15.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsDegenerateBandTest,
    "GTC.Weapons.Ballistics.DegenerateBandIsSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsDegenerateBandTest::RunTest(const FString& Parameters)
{
    // end <= start must not divide by zero.
    TestEqual(TEXT("degenerate band == 10"), FBallistics::DamageAtRange(20.0, 50.0, 30.0, 30.0, 0.5), 10.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsDiskWithinCircleTest,
    "GTC.Weapons.Ballistics.DiskSampleWithinUnitCircle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsDiskWithinCircleTest::RunTest(const FString& Parameters)
{
    for (int32 i = 0; i <= 10; ++i)
    {
        for (int32 j = 0; j <= 10; ++j)
        {
            const FVector2D P = FBallistics::DiskSample(static_cast<double>(i) / 10.0, static_cast<double>(j) / 10.0);
            TestTrue(TEXT("disk sample within unit circle"), P.Size() <= 1.0001);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsDiskZeroRadiusTest,
    "GTC.Weapons.Ballistics.DiskSampleZeroRadiusIsCentre",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsDiskZeroRadiusTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("zero radius is centre"), FBallistics::DiskSample(0.0, 0.5).IsNearlyZero(Eps));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsZoneHeadshotTest,
    "GTC.Weapons.Ballistics.ZoneMultiplierHeadshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsZoneHeadshotTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("headshot mult == 2"), FBallistics::ZoneMultiplier(1.7, 1.5, 2.0), 2.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsZoneBodyTest,
    "GTC.Weapons.Ballistics.ZoneMultiplierBody",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsZoneBodyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("body mult == 1"), FBallistics::ZoneMultiplier(1.0, 1.5, 2.0), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBallisticsZoneBoundaryTest,
    "GTC.Weapons.Ballistics.ZoneMultiplierBoundaryIsHeadshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBallisticsZoneBoundaryTest::RunTest(const FString& Parameters)
{
    // Exactly at the head line counts as a headshot.
    TestEqual(TEXT("boundary mult == 2"), FBallistics::ZoneMultiplier(1.5, 1.5, 2.0), 2.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
