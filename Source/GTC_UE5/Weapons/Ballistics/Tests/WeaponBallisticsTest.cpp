// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WeaponBallistics.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

// Each test maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_weapon_ballistics.gd. Float/vector compares use Eps,
// mirroring is_equal_approx. The model stays in the Godot frame.
//
// RNG note: SpreadDirection uses FRandomStream (seed-reproducible WITHIN UE5,
// not byte-identical to Godot). The oracle never pins a seed to an exact spread
// vector — it only asserts statistical/range properties (normalized, within the
// cone, actually perturbed), so FRandomStream satisfies them directly. The
// oracle's seeds are kept (test-local), as in the LootTable pattern.
namespace
{
    using GtcTest::Eps;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBDamageFullInsideTest,
    "GTC.Weapons.WeaponBallistics.DamageFullInsideAndAtStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBDamageFullInsideTest::RunTest(const FString& Parameters)
{
    // Full damage anywhere inside falloff_start, including the boundary.
    TestEqual(TEXT("inside"), FWeaponBallistics::DamageAtRange(40.0, 10.0, 25.0, 90.0, 0.45), 40.0, Eps);
    TestEqual(TEXT("at start"), FWeaponBallistics::DamageAtRange(40.0, 25.0, 25.0, 90.0, 0.45), 40.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBDamageReducedPastStartTest,
    "GTC.Weapons.WeaponBallistics.DamageReducedPastStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBDamageReducedPastStartTest::RunTest(const FString& Parameters)
{
    // Midpoint of the 25..90 band lerps 1.0 -> 0.4: factor 0.7 -> 28.0.
    TestEqual(TEXT("midpoint == 28"), FWeaponBallistics::DamageAtRange(40.0, 57.5, 25.0, 90.0, 0.4), 28.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBDamageFlooredTest,
    "GTC.Weapons.WeaponBallistics.DamageFlooredAtAndBeyondEnd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBDamageFlooredTest::RunTest(const FString& Parameters)
{
    // Floor (base * 0.45 = 18) reached at falloff_end and held flat past it.
    TestEqual(TEXT("at end"), FWeaponBallistics::DamageAtRange(40.0, 90.0, 25.0, 90.0, 0.45), 18.0, Eps);
    TestEqual(TEXT("beyond end"), FWeaponBallistics::DamageAtRange(40.0, 200.0, 25.0, 90.0, 0.45), 18.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBDamageMinFactorClampedTest,
    "GTC.Weapons.WeaponBallistics.DamageMinFactorClamped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBDamageMinFactorClampedTest::RunTest(const FString& Parameters)
{
    // min_factor > 1 clamps to 1: full damage past end, never amplified.
    TestEqual(TEXT("clamped"), FWeaponBallistics::DamageAtRange(40.0, 300.0, 25.0, 90.0, 2.0), 40.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBDamageDegenerateBandTest,
    "GTC.Weapons.WeaponBallistics.DamageDegenerateBandSnapsToFloor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBDamageDegenerateBandTest::RunTest(const FString& Parameters)
{
    // end <= start (collapsed band): a hard step at start — past it snaps to floor.
    TestEqual(TEXT("floor"), FWeaponBallistics::DamageAtRange(40.0, 60.0, 50.0, 50.0, 0.5), 20.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBDamageNegativeDistanceTest,
    "GTC.Weapons.WeaponBallistics.DamageNegativeDistanceGuarded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBDamageNegativeDistanceTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("neg dist"), FWeaponBallistics::DamageAtRange(40.0, -5.0, 25.0, 90.0, 0.45), 40.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBMultiplierOrderingTest,
    "GTC.Weapons.WeaponBallistics.MultiplierOrderingHeadTorsoLimb",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBMultiplierOrderingTest::RunTest(const FString& Parameters)
{
    // Head rewards precision, limbs punish it: head > torso > limb.
    TestTrue(TEXT("head > torso"),
        FWeaponBallistics::HitMultiplier(TEXT("head")) > FWeaponBallistics::HitMultiplier(TEXT("torso")));
    TestTrue(TEXT("torso > limb"),
        FWeaponBallistics::HitMultiplier(TEXT("torso")) > FWeaponBallistics::HitMultiplier(TEXT("limb")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBHeadMultiplierCaseTest,
    "GTC.Weapons.WeaponBallistics.HeadMultiplierValueAndCaseInsensitive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBHeadMultiplierCaseTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("head == 2"), FWeaponBallistics::HitMultiplier(TEXT("head")), 2.0, Eps);
    TestEqual(TEXT("HEAD == 2"), FWeaponBallistics::HitMultiplier(TEXT("HEAD")), 2.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBUnknownPartTest,
    "GTC.Weapons.WeaponBallistics.UnknownPartIsOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBUnknownPartTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("wing == 1"), FWeaponBallistics::HitMultiplier(TEXT("wing")), 1.0, Eps);
    TestEqual(TEXT("empty == 1"), FWeaponBallistics::HitMultiplier(TEXT("")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBSpreadZeroUnchangedTest,
    "GTC.Weapons.WeaponBallistics.SpreadZeroReturnsAimUnchanged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBSpreadZeroUnchangedTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(12345);
    const FVector Aim(0.0, 0.0, -1.0);
    const FVector Out = FWeaponBallistics::SpreadDirection(Aim, 0.0, Rng);
    TestTrue(TEXT("zero spread unchanged"), Out.Equals(Aim, Eps));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBSpreadNormalizedTest,
    "GTC.Weapons.WeaponBallistics.SpreadResultStaysNormalized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBSpreadNormalizedTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(999);
    const FVector Aim(0.0, 0.0, -1.0);
    const FVector Out = FWeaponBallistics::SpreadDirection(Aim, 0.1, Rng);
    TestEqual(TEXT("unit length"), Out.Length(), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBSpreadWithinConeTest,
    "GTC.Weapons.WeaponBallistics.SpreadWithinConeAngle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBSpreadWithinConeTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(7);
    const FVector Aim = FVector(0.0, 1.0, 0.0).GetSafeNormal();
    const double Spread = 0.15;
    const double CosLimit = FMath::Cos(Spread);
    for (int32 i = 0; i < 64; ++i)
    {
        const FVector Out = FWeaponBallistics::SpreadDirection(Aim, Spread, Rng);
        // Float slack so a shot exactly on the cone edge doesn't false-fail.
        TestTrue(TEXT("within cone"), FVector::DotProduct(Out, Aim) >= CosLimit - 0.0001);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBSpreadPerturbsTest,
    "GTC.Weapons.WeaponBallistics.SpreadActuallyPerturbs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBSpreadPerturbsTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(42);
    const FVector Aim(0.0, 0.0, -1.0);
    const FVector Out = FWeaponBallistics::SpreadDirection(Aim, 0.1, Rng);
    TestFalse(TEXT("perturbed"), Out.Equals(Aim, Eps));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBSpreadNormalizesAimTest,
    "GTC.Weapons.WeaponBallistics.SpreadNormalizesAimInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBSpreadNormalizesAimTest::RunTest(const FString& Parameters)
{
    FRandomStream Rng(3);
    // Non-unit, zero-spread aim should come back unit length along the same axis.
    const FVector Out = FWeaponBallistics::SpreadDirection(FVector(0.0, 0.0, -5.0), 0.0, Rng);
    TestTrue(TEXT("normalized aim"), Out.Equals(FVector(0.0, 0.0, -1.0), Eps));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBBloomStartsAtMinTest,
    "GTC.Weapons.WeaponBallistics.BloomStartsAtMin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBBloomStartsAtMinTest::RunTest(const FString& Parameters)
{
    FWeaponBallistics::FBloom B(0.01, 0.16, 0.02, 0.22);
    TestEqual(TEXT("starts at min"), B.CurrentSpread(), 0.01, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBBloomGrowsTest,
    "GTC.Weapons.WeaponBallistics.BloomGrowsWithShots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBBloomGrowsTest::RunTest(const FString& Parameters)
{
    FWeaponBallistics::FBloom B(0.01, 0.16, 0.02, 0.22);
    B.AddShot();
    B.AddShot();
    TestEqual(TEXT("grows to 0.05"), B.CurrentSpread(), 0.05, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBBloomCappedTest,
    "GTC.Weapons.WeaponBallistics.BloomCappedAtMax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBBloomCappedTest::RunTest(const FString& Parameters)
{
    FWeaponBallistics::FBloom B(0.01, 0.16, 0.05, 0.22);
    for (int32 i = 0; i < 50; ++i)
    {
        B.AddShot();
    }
    TestEqual(TEXT("capped at 0.16"), B.CurrentSpread(), 0.16, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBBloomRecoversTest,
    "GTC.Weapons.WeaponBallistics.BloomRecoversTowardMin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBBloomRecoversTest::RunTest(const FString& Parameters)
{
    FWeaponBallistics::FBloom B(0.01, 0.16, 0.05, 0.10);
    B.AddShot();      // 0.06
    B.Recover(0.2);   // -0.02 -> 0.04
    TestEqual(TEXT("recovers to 0.04"), B.CurrentSpread(), 0.04, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBBloomRecoverFloorsTest,
    "GTC.Weapons.WeaponBallistics.BloomRecoverFloorsAtMin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBBloomRecoverFloorsTest::RunTest(const FString& Parameters)
{
    FWeaponBallistics::FBloom B(0.01, 0.16, 0.05, 0.10);
    B.AddShot();
    B.Recover(100.0);
    TestEqual(TEXT("floors at min"), B.CurrentSpread(), 0.01, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBBloomResetTest,
    "GTC.Weapons.WeaponBallistics.BloomResetSnapsToMin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBBloomResetTest::RunTest(const FString& Parameters)
{
    FWeaponBallistics::FBloom B(0.02, 0.16, 0.04, 0.22);
    B.AddShot();
    B.AddShot();
    B.Reset();
    TestEqual(TEXT("reset to min"), B.CurrentSpread(), 0.02, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBEffectiveHeadshotTest,
    "GTC.Weapons.WeaponBallistics.EffectiveDamageHeadshotUpClose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBEffectiveHeadshotTest::RunTest(const FString& Parameters)
{
    // Inside falloff_start: full base, doubled for a headshot.
    const double Dmg = FWeaponBallistics::EffectiveDamage(30.0, 5.0, TEXT("head"), 25.0, 90.0, 0.45);
    TestEqual(TEXT("headshot == 60"), Dmg, 60.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBEffectiveLimbTest,
    "GTC.Weapons.WeaponBallistics.EffectiveDamageLimbAtRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBEffectiveLimbTest::RunTest(const FString& Parameters)
{
    // Midpoint of band: factor 0.7 -> 21.0, limb 0.7 -> 14.7.
    const double Dmg = FWeaponBallistics::EffectiveDamage(30.0, 57.5, TEXT("limb"), 25.0, 90.0, 0.4);
    TestEqual(TEXT("limb == 14.7"), Dmg, 14.7, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBTimeToKillBasicTest,
    "GTC.Weapons.WeaponBallistics.TimeToKillBasic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBTimeToKillBasicTest::RunTest(const FString& Parameters)
{
    // 100 hp, 25 dmg/shot -> 4 shots; at 10/s the 4th lands at 0.3s.
    TestEqual(TEXT("ttk == 0.3"), FWeaponBallistics::TimeToKill(25.0, 10.0, 100.0), 0.3, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBTimeToKillOneShotTest,
    "GTC.Weapons.WeaponBallistics.TimeToKillOneShotIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBTimeToKillOneShotTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("one shot == 0"), FWeaponBallistics::TimeToKill(100.0, 5.0, 80.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWBTimeToKillInfTest,
    "GTC.Weapons.WeaponBallistics.TimeToKillNoDamageIsInf",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWBTimeToKillInfTest::RunTest(const FString& Parameters)
{
    // No damage → true +infinity (guard asserted via !IsFinite).
    const double Ttk = FWeaponBallistics::TimeToKill(0.0, 10.0, 100.0);
    TestFalse(TEXT("ttk is +inf"), FMath::IsFinite(Ttk));
    TestTrue(TEXT("ttk is positive inf"), Ttk > 0.0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
