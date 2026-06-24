// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Suppression.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

/** Tests for FSuppression — the build/decay/effect curve that pins officers under
 *  sustained fire. Prefix GTC.AI.Combat.Suppression. */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSuppressionCurveTest,
    "GTC.AI.Combat.Suppression.Curve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSuppressionCurveTest::RunTest(const FString& Parameters)
{
    // FromHit: a graze adds some, a heavy hit more, clamped to 1.
    TestTrue(TEXT("graze adds some"), FSuppression::FromHit(0.0) > 0.0);
    TestTrue(TEXT("harder hit suppresses more"),
        FSuppression::FromHit(1.0) > FSuppression::FromHit(0.0));
    TestTrue(TEXT("from-hit clamped <= 1"), FSuppression::FromHit(5.0) <= 1.0 + Eps);

    // Add accumulates and clamps at 1.
    TestEqual(TEXT("add accumulates"), FSuppression::Add(0.2, 0.3), 0.5, Eps);
    TestEqual(TEXT("add clamps at 1"), FSuppression::Add(0.9, 0.5), 1.0, Eps);

    // Decay bleeds off and floors at 0.
    TestEqual(TEXT("decay partial"), FSuppression::Decay(1.0, 1.0, 0.35), 0.65, Eps);
    TestEqual(TEXT("decay floors at 0"), FSuppression::Decay(0.1, 1.0, 0.35), 0.0, Eps);

    // Threshold (inclusive).
    TestTrue(TEXT("at threshold is suppressed"), FSuppression::IsSuppressed(0.5));
    TestFalse(TEXT("below threshold not suppressed"), FSuppression::IsSuppressed(0.49));

    // Fire-rate stretch: none at 0, ~2.2x at full.
    TestEqual(TEXT("no suppression -> 1x fire interval"), FSuppression::FireRateMult(0.0), 1.0, Eps);
    TestEqual(TEXT("full suppression -> 2.2x fire interval"), FSuppression::FireRateMult(1.0), 2.2, Eps);

    // Effective health: unchanged at 0, halved at full suppression.
    TestEqual(TEXT("no suppression keeps health"), FSuppression::EffectiveHealthFrac(0.8, 0.0), 0.8, Eps);
    TestEqual(TEXT("full suppression halves health"), FSuppression::EffectiveHealthFrac(0.8, 1.0), 0.4, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
