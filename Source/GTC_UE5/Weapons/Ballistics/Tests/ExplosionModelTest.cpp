// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ExplosionModel.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_explosion_model.gd. Curve under test is LINEAR falloff.
// Float/vector compares use Eps (1e-4). The model stays in the Godot frame, so
// the upward knockback bias is along +Y (Godot's Vector3.UP).
namespace
{
    using GtcTest::Eps;
}

// --- falloff ---------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelFalloffFullCentreTest,
    "GTC.Weapons.ExplosionModel.FalloffFullAtCenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelFalloffFullCentreTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("centre == 1"), FExplosionModel::Falloff(0.0, 10.0), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelFalloffZeroAtBeyondTest,
    "GTC.Weapons.ExplosionModel.FalloffZeroAtAndBeyondRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelFalloffZeroAtBeyondTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("at radius == 0"), FExplosionModel::Falloff(10.0, 10.0), 0.0, Eps);
    TestEqual(TEXT("beyond == 0"), FExplosionModel::Falloff(25.0, 10.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelFalloffHalfMidTest,
    "GTC.Weapons.ExplosionModel.FalloffHalfAtMidRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelFalloffHalfMidTest::RunTest(const FString& Parameters)
{
    // Linear: at half the radius the curve reads 0.5.
    TestEqual(TEXT("mid == 0.5"), FExplosionModel::Falloff(5.0, 10.0), 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelFalloffZeroRadiusTest,
    "GTC.Weapons.ExplosionModel.FalloffZeroRadiusIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelFalloffZeroRadiusTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("zero radius == 0"), FExplosionModel::Falloff(0.0, 0.0), 0.0, Eps);
    return true;
}

// --- damage_at -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelDamageFullCentreTest,
    "GTC.Weapons.ExplosionModel.DamageFullAtCenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelDamageFullCentreTest::RunTest(const FString& Parameters)
{
    const FVector C(4.0, 1.0, -2.0);
    TestEqual(TEXT("centre == 100"), FExplosionModel::DamageAt(C, C, 100.0, 8.0), 100.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelDamageHalfMidTest,
    "GTC.Weapons.ExplosionModel.DamageHalfAtMidRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelDamageHalfMidTest::RunTest(const FString& Parameters)
{
    // Target 5 units up from centre, radius 10 → linear 0.5 → 50 damage.
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(0.0, 5.0, 0.0);
    TestEqual(TEXT("mid == 50"), FExplosionModel::DamageAt(C, T, 100.0, 10.0), 50.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelDamageZeroAtRadiusTest,
    "GTC.Weapons.ExplosionModel.DamageZeroAtRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelDamageZeroAtRadiusTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(10.0, 0.0, 0.0);
    TestEqual(TEXT("at radius == 0"), FExplosionModel::DamageAt(C, T, 100.0, 10.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelDamageZeroBeyondTest,
    "GTC.Weapons.ExplosionModel.DamageZeroBeyondRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelDamageZeroBeyondTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(0.0, 0.0, 30.0);
    TestEqual(TEXT("beyond == 0"), FExplosionModel::DamageAt(C, T, 100.0, 10.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelDamageNeverNegativeTest,
    "GTC.Weapons.ExplosionModel.DamageNeverNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelDamageNeverNegativeTest::RunTest(const FString& Parameters)
{
    // Negative max_damage is floored to 0, never produces healing.
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(3.0, 0.0, 0.0);
    TestTrue(TEXT("never negative"), FExplosionModel::DamageAt(C, T, -50.0, 10.0) >= 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelDamage3dDistanceTest,
    "GTC.Weapons.ExplosionModel.DamageUsesFull3dDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelDamage3dDistanceTest::RunTest(const FString& Parameters)
{
    // Diagonal (3,4,0) is 5 units out; radius 10 → 0.5 → 50.
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(3.0, 4.0, 0.0);
    TestEqual(TEXT("3d == 50"), FExplosionModel::DamageAt(C, T, 100.0, 10.0), 50.0, Eps);
    return true;
}

// --- knockback -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelKnockbackAwayTest,
    "GTC.Weapons.ExplosionModel.KnockbackPointsAwayFromCenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelKnockbackAwayTest::RunTest(const FString& Parameters)
{
    // Target at half radius on +x: pushed outward in +x with magnitude
    // strength*1 (0.5*100 = 50), no z drift.
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(5.0, 0.0, 0.0);
    const FVector K = FExplosionModel::Knockback(C, T, 100.0, 10.0);
    TestTrue(TEXT("k.x > 0"), K.X > 0.0);
    TestEqual(TEXT("k.x == 50"), K.X, 50.0, Eps);
    TestEqual(TEXT("k.z == 0"), K.Z, 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelKnockbackUpwardTest,
    "GTC.Weapons.ExplosionModel.KnockbackHasUpwardComponent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelKnockbackUpwardTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(0.0, 0.0, 4.0);
    const FVector K = FExplosionModel::Knockback(C, T, 100.0, 10.0);
    TestTrue(TEXT("k.y > 0"), K.Y > 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelKnockbackShrinksTest,
    "GTC.Weapons.ExplosionModel.KnockbackShrinksWithDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelKnockbackShrinksTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector Near = FExplosionModel::Knockback(C, FVector(2.0, 0.0, 0.0), 100.0, 10.0);
    const FVector Far = FExplosionModel::Knockback(C, FVector(8.0, 0.0, 0.0), 100.0, 10.0);
    TestTrue(TEXT("near > far"), Near.Length() > Far.Length());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelKnockbackZeroBeyondTest,
    "GTC.Weapons.ExplosionModel.KnockbackZeroBeyondRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelKnockbackZeroBeyondTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(20.0, 0.0, 0.0);
    TestTrue(TEXT("beyond == ZERO"), FExplosionModel::Knockback(C, T, 100.0, 10.0) == FVector::ZeroVector);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelKnockbackZeroAtRadiusTest,
    "GTC.Weapons.ExplosionModel.KnockbackZeroAtRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelKnockbackZeroAtRadiusTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector T(10.0, 0.0, 0.0);
    TestTrue(TEXT("at radius == ZERO"), FExplosionModel::Knockback(C, T, 100.0, 10.0) == FVector::ZeroVector);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelKnockbackCentreNoNanTest,
    "GTC.Weapons.ExplosionModel.KnockbackCenterNoNanPureVertical",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelKnockbackCentreNoNanTest::RunTest(const FString& Parameters)
{
    // Exact centre: no outward direction, still lifts straight up, never NaN.
    const FVector C(7.0, 2.0, 3.0);
    const FVector K = FExplosionModel::Knockback(C, C, 100.0, 10.0);
    const bool bFinite = !(FMath::IsNaN(K.X) || FMath::IsNaN(K.Y) || FMath::IsNaN(K.Z));
    TestTrue(TEXT("finite"), bFinite);
    TestEqual(TEXT("k.x == 0"), K.X, 0.0, Eps);
    TestEqual(TEXT("k.z == 0"), K.Z, 0.0, Eps);
    TestTrue(TEXT("k.y > 0"), K.Y > 0.0);
    return true;
}

// --- is_in_blast -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelInBlastInsideTest,
    "GTC.Weapons.ExplosionModel.InBlastInside",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelInBlastInsideTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("inside"), FExplosionModel::IsInBlast(FVector::ZeroVector, FVector(0.0, 0.0, 9.99), 10.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelInBlastBoundaryTest,
    "GTC.Weapons.ExplosionModel.InBlastBoundaryExcluded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelInBlastBoundaryTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("boundary excluded"), FExplosionModel::IsInBlast(FVector::ZeroVector, FVector(10.0, 0.0, 0.0), 10.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelInBlastOutsideTest,
    "GTC.Weapons.ExplosionModel.InBlastOutside",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelInBlastOutsideTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("outside"), FExplosionModel::IsInBlast(FVector::ZeroVector, FVector(11.0, 0.0, 0.0), 10.0));
    return true;
}

// --- should_chain ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelChainWithinTest,
    "GTC.Weapons.ExplosionModel.ShouldChainWithinTrigger",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelChainWithinTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector Car(3.0, 0.0, 0.0);
    TestTrue(TEXT("within trigger"), FExplosionModel::ShouldChain(C, Car, 5.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelChainInclusiveTest,
    "GTC.Weapons.ExplosionModel.ShouldChainAtTriggerInclusive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelChainInclusiveTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector Barrel(5.0, 0.0, 0.0);
    TestTrue(TEXT("at trigger inclusive"), FExplosionModel::ShouldChain(C, Barrel, 5.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelChainBeyondTest,
    "GTC.Weapons.ExplosionModel.ShouldChainFalseBeyondTrigger",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelChainBeyondTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const FVector Car(0.0, 0.0, 9.0);
    TestFalse(TEXT("beyond trigger"), FExplosionModel::ShouldChain(C, Car, 5.0));
    return true;
}

// --- apply_to_many ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelApplyOnlyInBlastTest,
    "GTC.Weapons.ExplosionModel.ApplyToManyOnlyInBlast",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelApplyOnlyInBlastTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const TArray<FVector> Targets{
        FVector(0.0, 0.0, 0.0),   // index 0: centre, in blast
        FVector(5.0, 0.0, 0.0),   // index 1: mid, in blast
        FVector(20.0, 0.0, 0.0),  // index 2: outside
    };
    const TArray<FExplosionHit> Hits = FExplosionModel::ApplyToMany(C, Targets, 100.0, 10.0);
    TestEqual(TEXT("two hits"), Hits.Num(), 2);
    TestEqual(TEXT("hit0 index 0"), Hits[0].Index, 0);
    TestEqual(TEXT("hit1 index 1"), Hits[1].Index, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelApplyDamageValuesTest,
    "GTC.Weapons.ExplosionModel.ApplyToManyDamageValues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelApplyDamageValuesTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const TArray<FVector> Targets{ FVector(0.0, 0.0, 0.0), FVector(5.0, 0.0, 0.0) };
    const TArray<FExplosionHit> Hits = FExplosionModel::ApplyToMany(C, Targets, 100.0, 10.0);
    TestEqual(TEXT("hit0 dmg 100"), Hits[0].Damage, 100.0, Eps);
    TestEqual(TEXT("hit1 dmg 50"), Hits[1].Damage, 50.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExModelApplyEmptyTest,
    "GTC.Weapons.ExplosionModel.ApplyToManyEmptyWhenNoneInBlast",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExModelApplyEmptyTest::RunTest(const FString& Parameters)
{
    const FVector C(0.0, 0.0, 0.0);
    const TArray<FVector> Targets{ FVector(50.0, 0.0, 0.0), FVector(0.0, 60.0, 0.0) };
    TestEqual(TEXT("empty"), FExplosionModel::ApplyToMany(C, Targets, 100.0, 10.0).Num(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
