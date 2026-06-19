// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BulletPenetration.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FBulletPenetration — surface cost, penetrate/stop, remaining power, and
 * exit damage. Prefix GTC.Weapons.Penetration.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBulletPenetrationTest,
    "GTC.Weapons.Penetration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBulletPenetrationTest::RunTest(const FString& Parameters)
{
    // --- SurfaceCost ---
    TestTrue(TEXT("thicker surface costs more"),
        FBulletPenetration::SurfaceCost(10.0, 1.0) > FBulletPenetration::SurfaceCost(5.0, 1.0));
    TestTrue(TEXT("denser surface costs more"),
        FBulletPenetration::SurfaceCost(5.0, 3.0) > FBulletPenetration::SurfaceCost(5.0, 0.5));

    // --- CanPenetrate ---
    TestTrue(TEXT("a powerful round punches through thin wood"),
        FBulletPenetration::CanPenetrate(20.0, 4.0, 0.5)); // cost 2
    TestFalse(TEXT("a weak round is stopped by thick concrete"),
        FBulletPenetration::CanPenetrate(5.0, 10.0, 3.0)); // cost 30
    TestFalse(TEXT("a no-power round penetrates nothing"),
        FBulletPenetration::CanPenetrate(0.0, 1.0, 0.1));

    // --- RemainingPower ---
    TestTrue(TEXT("penetrating bleeds power"),
        FBulletPenetration::RemainingPower(20.0, 4.0, 1.0) < 20.0);
    TestEqual(TEXT("a stopped round has no remaining power"),
        FBulletPenetration::RemainingPower(5.0, 10.0, 3.0), 0.0);
    // Chaining through two surfaces costs more than one.
    const double After1 = FBulletPenetration::RemainingPower(30.0, 4.0, 1.0);
    const double After2 = FBulletPenetration::RemainingPower(After1, 4.0, 1.0);
    TestTrue(TEXT("each surface bleeds more power"), After2 < After1);

    // --- ExitDamageFactor ---
    TestEqual(TEXT("a blocked round deals no exit damage"),
        FBulletPenetration::ExitDamageFactor(5.0, 10.0, 3.0), 0.0);
    TestTrue(TEXT("an overkill round retains most damage"),
        FBulletPenetration::ExitDamageFactor(100.0, 2.0, 0.5) > 0.9);
    // A marginal penetration is floored, not zero.
    const double Marginal = FBulletPenetration::ExitDamageFactor(2.01, 4.0, 0.5); // cost 2, just makes it
    TestTrue(TEXT("a marginal penetration is floored above zero"),
        Marginal >= FBulletPenetration::MinExitFactor - GtcTest::Eps && Marginal > 0.0);
    // Thicker cover bleeds more exit damage.
    TestTrue(TEXT("thicker cover reduces exit damage more"),
        FBulletPenetration::ExitDamageFactor(20.0, 8.0, 1.0) < FBulletPenetration::ExitDamageFactor(20.0, 2.0, 1.0));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
