// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcVitals.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// ============================================================================
// FNpcVitals — pure pedestrian health math (prefix GTC.NPC.Vitals). The
// acting-on-the-result half (ragdoll, montages, heat) lives in the AGTCCitizen
// adapter and is exercised in-editor, not here.
// ============================================================================

// --- a fresh citizen starts at full health and alive ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsStartsFullTest,
    "GTC.NPC.Vitals.StartsFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsStartsFullTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(100.0);
    TestTrue(TEXT("health == 100"), FMath::Abs(V.Health - 100.0) < Eps);
    TestFalse(TEXT("not dead"), V.IsDead());
    TestTrue(TEXT("fraction == 1"), FMath::Abs(V.Fraction() - 1.0) < Eps);
    return true;
}

// --- a survivable hit lowers health and reports Wounded ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsWoundReducesTest,
    "GTC.NPC.Vitals.WoundReducesHealth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsWoundReducesTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(100.0);
    const ENpcHitResult R = V.Apply(30.0);
    TestTrue(TEXT("wounded"), R == ENpcHitResult::Wounded);
    TestTrue(TEXT("health == 70"), FMath::Abs(V.Health - 70.0) < Eps);
    TestFalse(TEXT("still up"), V.IsDead());
    return true;
}

// --- zero / negative damage is ignored and leaves health untouched ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsNonPositiveIgnoredTest,
    "GTC.NPC.Vitals.NonPositiveIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsNonPositiveIgnoredTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(100.0);
    TestTrue(TEXT("zero ignored"), V.Apply(0.0) == ENpcHitResult::Ignored);
    TestTrue(TEXT("negative ignored"), V.Apply(-25.0) == ENpcHitResult::Ignored);
    TestTrue(TEXT("health == 100"), FMath::Abs(V.Health - 100.0) < Eps);
    return true;
}

// --- a hit that empties the pool reports Killed and clamps at zero ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsLethalHitTest,
    "GTC.NPC.Vitals.LethalHitKills",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsLethalHitTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(100.0);
    const ENpcHitResult R = V.Apply(100.0);
    TestTrue(TEXT("killed"), R == ENpcHitResult::Killed);
    TestTrue(TEXT("dead"), V.IsDead());
    TestTrue(TEXT("health == 0"), FMath::Abs(V.Health - 0.0) < Eps);
    return true;
}

// --- overkill does not drive health negative ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsOverkillClampsTest,
    "GTC.NPC.Vitals.OverkillClampsAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsOverkillClampsTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(100.0);
    TestTrue(TEXT("killed"), V.Apply(5000.0) == ENpcHitResult::Killed);
    TestTrue(TEXT("health == 0, not negative"), FMath::Abs(V.Health - 0.0) < Eps);
    return true;
}

// --- shooting a corpse changes nothing (no double-kill) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsHitOnDeadIgnoredTest,
    "GTC.NPC.Vitals.HitOnDeadIsIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsHitOnDeadIgnoredTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(100.0);
    V.Apply(100.0);
    const ENpcHitResult R = V.Apply(10.0);
    TestTrue(TEXT("ignored once dead"), R == ENpcHitResult::Ignored);
    TestTrue(TEXT("still zero"), FMath::Abs(V.Health - 0.0) < Eps);
    return true;
}

// --- fraction tracks remaining health ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsFractionTracksTest,
    "GTC.NPC.Vitals.FractionTracks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsFractionTracksTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(200.0);
    V.Apply(50.0); // 150 / 200
    TestTrue(TEXT("fraction == 0.75"), FMath::Abs(V.Fraction() - 0.75) < Eps);
    return true;
}

// --- a tougher body soaks a hit that would drop a baseline citizen ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsToughnessTest,
    "GTC.NPC.Vitals.ToughBodyTakesMoreHits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsToughnessTest::RunTest(const FString& Parameters)
{
    FNpcVitals Tough(200.0);
    TestTrue(TEXT("first hit only wounds"), Tough.Apply(120.0) == ENpcHitResult::Wounded);
    TestTrue(TEXT("second hit kills"), Tough.Apply(120.0) == ENpcHitResult::Killed);
    return true;
}

// --- wound severity is the clamped damage-to-toughness ratio ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsWoundSeverityTest,
    "GTC.NPC.Vitals.WoundSeverityScales",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsWoundSeverityTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("quarter hit"), FMath::Abs(FNpcVitals::WoundSeverity(25.0, 100.0) - 0.25) < Eps);
    TestTrue(TEXT("overkill clamps to 1"), FMath::Abs(FNpcVitals::WoundSeverity(500.0, 100.0) - 1.0) < Eps);
    TestTrue(TEXT("negative clamps to 0"), FMath::Abs(FNpcVitals::WoundSeverity(-5.0, 100.0) - 0.0) < Eps);
    return true;
}

// --- a non-positive or zero toughness ceiling never divides by zero ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcVitalsMaxHealthFlooredTest,
    "GTC.NPC.Vitals.MaxHealthFloored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNpcVitalsMaxHealthFlooredTest::RunTest(const FString& Parameters)
{
    FNpcVitals V(0.0);
    TestTrue(TEXT("max floored above zero"), V.MaxHealth > 0.0);
    TestTrue(TEXT("fraction finite"), FMath::IsFinite(V.Fraction()));
    TestTrue(TEXT("any hit kills a zero-health body"), V.Apply(1.0) == ENpcHitResult::Killed);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
