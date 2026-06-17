// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ExplosionMath.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_explosion_math.gd. Float compares use Eps (1e-4).
namespace
{
    using GtcTest::Eps;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExMathFullAtCentreTest,
    "GTC.Weapons.ExplosionMath.FullDamageAtCentre",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExMathFullAtCentreTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("centre == 120"), FExplosionMath::RadialDamage(0.0, 2.5, 7.5, 120.0), 120.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExMathFullInsideInnerTest,
    "GTC.Weapons.ExplosionMath.FullDamageInsideInner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExMathFullInsideInnerTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("inner == 120"), FExplosionMath::RadialDamage(2.5, 2.5, 7.5, 120.0), 120.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExMathZeroAtOuterTest,
    "GTC.Weapons.ExplosionMath.ZeroAtOuter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExMathZeroAtOuterTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("outer == 0"), FExplosionMath::RadialDamage(7.5, 2.5, 7.5, 120.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExMathZeroBeyondOuterTest,
    "GTC.Weapons.ExplosionMath.ZeroBeyondOuter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExMathZeroBeyondOuterTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("beyond == 0"), FExplosionMath::RadialDamage(20.0, 2.5, 7.5, 120.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExMathLinearMidpointTest,
    "GTC.Weapons.ExplosionMath.LinearMidpoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExMathLinearMidpointTest::RunTest(const FString& Parameters)
{
    // Midpoint of [2.5, 7.5] is 5.0 → half damage.
    TestEqual(TEXT("midpoint == 60"), FExplosionMath::RadialDamage(5.0, 2.5, 7.5, 120.0), 60.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExMathDegenerateRadiiTest,
    "GTC.Weapons.ExplosionMath.DegenerateRadiiSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExMathDegenerateRadiiTest::RunTest(const FString& Parameters)
{
    // Zero-width / inverted band → no damage anywhere, no divide-by-zero.
    TestEqual(TEXT("centre, degenerate == 0"), FExplosionMath::RadialDamage(0.0, 5.0, 5.0, 120.0), 0.0, Eps);
    TestEqual(TEXT("mid, degenerate == 0"), FExplosionMath::RadialDamage(3.0, 5.0, 5.0, 120.0), 0.0, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
