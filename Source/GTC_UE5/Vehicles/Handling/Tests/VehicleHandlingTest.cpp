// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleHandling.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FVehicleHandling — arcade grip/slip/drift math and the drift-combo
 * scorer. Prefix GTC.Vehicles.Handling.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleHandlingTest,
    "GTC.Vehicles.Handling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleHandlingTest::RunTest(const FString& Parameters)
{
    const FVector Fwd(1, 0, 0);
    const FVector Right(0, 1, 0);

    // --- SlipAngleRad ---
    TestTrue(TEXT("tracking straight => ~0 slip"),
        FMath::IsNearlyEqual(FVehicleHandling::SlipAngleRad(Fwd, FVector(10, 0, 0)), 0.0, GtcTest::Eps));
    TestTrue(TEXT("pure sideways => ~pi/2 slip"),
        FMath::IsNearlyEqual(FVehicleHandling::SlipAngleRad(Fwd, FVector(0, 10, 0)), PI / 2.0, GtcTest::Eps));
    TestEqual(TEXT("parked car has no slip"),
        FVehicleHandling::SlipAngleRad(Fwd, FVector::ZeroVector), 0.0);

    // --- LateralRetention ---
    TestTrue(TEXT("handbrake retains more lateral than gripping"),
        FVehicleHandling::LateralRetention(true, 1.0) > FVehicleHandling::LateralRetention(false, 1.0));
    TestTrue(TEXT("worn grip slides more than fresh grip"),
        FVehicleHandling::LateralRetention(false, 0.2) > FVehicleHandling::LateralRetention(false, 1.0));
    TestTrue(TEXT("retention stays within [0,1]"),
        FVehicleHandling::LateralRetention(true, 0.0) <= 1.0 + GtcTest::Eps
        && FVehicleHandling::LateralRetention(false, 1.0) >= 0.0 - GtcTest::Eps);

    // --- DriftFactor ---
    TestEqual(TEXT("no drift below min speed"),
        FVehicleHandling::DriftFactor(FVehicleHandling::FullDriftSlipRad, FVehicleHandling::MinDriftSpeed - 0.1), 0.0);
    TestEqual(TEXT("no drift inside the slip deadzone"),
        FVehicleHandling::DriftFactor(FVehicleHandling::SlipDeadzoneRad, 50.0), 0.0);
    TestTrue(TEXT("drift saturates at the full-drift slip angle"),
        FMath::IsNearlyEqual(FVehicleHandling::DriftFactor(FVehicleHandling::FullDriftSlipRad, 50.0), 1.0, GtcTest::Eps));
    TestTrue(TEXT("drift factor rises with slip"),
        FVehicleHandling::DriftFactor(0.3, 50.0) < FVehicleHandling::DriftFactor(0.5, 50.0));

    // --- ApplyGrip ---
    {
        const FVector V(10, 6, 0); // 10 forward, 6 sideways

        // Gripping scrubs off most of the sideways slide; forward speed is preserved.
        const FVector Gripped = FVehicleHandling::ApplyGrip(V, Fwd, Right, FVehicleHandling::GripRetention);
        TestTrue(TEXT("grip preserves forward speed"),
            FMath::IsNearlyEqual(FVector::DotProduct(Gripped, Fwd), 10.0, GtcTest::Eps));
        TestTrue(TEXT("grip kills most lateral speed"),
            FMath::IsNearlyEqual(FVector::DotProduct(Gripped, Right), 6.0 * FVehicleHandling::GripRetention, GtcTest::Eps));

        // Full retention (1.0) leaves the velocity untouched.
        const FVector Free = FVehicleHandling::ApplyGrip(V, Fwd, Right, 1.0);
        TestTrue(TEXT("full retention leaves lateral untouched"),
            FMath::IsNearlyEqual(FVector::DotProduct(Free, Right), 6.0, GtcTest::Eps));

        // Vertical (out-of-basis) component is preserved.
        const FVector V3(10, 6, 4);
        const FVector G3 = FVehicleHandling::ApplyGrip(V3, Fwd, Right, FVehicleHandling::GripRetention);
        TestTrue(TEXT("vertical component is preserved"),
            FMath::IsNearlyEqual(G3.Z, 4.0, GtcTest::Eps));

        // A degenerate basis returns the velocity unchanged.
        const FVector Same = FVehicleHandling::ApplyGrip(V, FVector::ZeroVector, Right, 0.1);
        TestTrue(TEXT("degenerate basis is a no-op"),
            FMath::IsNearlyEqual(FVector::DotProduct(Same, Right), 6.0, GtcTest::Eps));
    }

    // --- EffectiveTopSpeed ---
    TestTrue(TEXT("top speed scales by factor"),
        FMath::IsNearlyEqual(FVehicleHandling::EffectiveTopSpeed(100.0, 0.8), 80.0, GtcTest::Eps));
    TestEqual(TEXT("negative factor floors to zero"), FVehicleHandling::EffectiveTopSpeed(100.0, -1.0), 0.0);

    // --- FDriftScorer ---
    {
        FVehicleHandling::FDriftScorer S;
        TestFalse(TEXT("a fresh scorer is inactive"), S.IsActive());

        // Not drifting (below MinDriftFactor) => no accrual.
        S.Update(FVehicleHandling::FDriftScorer::MinDriftFactor - 0.05, 50.0, 0.1);
        TestFalse(TEXT("sub-threshold drift accrues nothing"), S.IsActive());

        // Hold a strong drift: score and multiplier both climb.
        for (int32 i = 0; i < 10; ++i)
        {
            S.Update(1.0, 50.0, 0.1);
        }
        TestTrue(TEXT("holding a drift accrues score"), S.Score > 0.0);
        TestTrue(TEXT("sustained drift grows the multiplier above 1"), S.Multiplier > 1.0);
        TestTrue(TEXT("pending payout applies the multiplier"),
            S.PendingPayout() > S.Score - GtcTest::Eps);

        // Banking pays Score*Multiplier and resets the run.
        const double Expected = S.Score * S.Multiplier;
        const double Paid = S.Bank();
        TestTrue(TEXT("bank pays score*multiplier"), FMath::IsNearlyEqual(Paid, Expected, GtcTest::Eps));
        TestFalse(TEXT("scorer resets after banking"), S.IsActive());
        TestTrue(TEXT("multiplier resets to 1"), FMath::IsNearlyEqual(S.Multiplier, 1.0, GtcTest::Eps));

        // Multiplier is capped however long the drift is held.
        FVehicleHandling::FDriftScorer Long;
        for (int32 i = 0; i < 1000; ++i)
        {
            Long.Update(1.0, 50.0, 0.1);
        }
        TestTrue(TEXT("multiplier is capped"),
            Long.Multiplier <= FVehicleHandling::FDriftScorer::MaxMultiplier + GtcTest::Eps);

        // Wiping loses the pending score and resets.
        const double WasScore = Long.Score;
        const double Lost = Long.Wipe();
        TestTrue(TEXT("wipe reports the lost score"), FMath::IsNearlyEqual(Lost, WasScore, GtcTest::Eps));
        TestFalse(TEXT("scorer resets after a wipe"), Long.IsActive());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
