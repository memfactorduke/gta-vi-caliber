// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Apprehend.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FApprehend — the decision that lets the police bust loop actually land
 * by sending low-heat cops in to arrest an unarmed suspect instead of holding at
 * gunfight range. Prefix GTC.AI.Combat.Apprehend.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FApprehendShouldApprehendTest,
    "GTC.AI.Combat.Apprehend.ShouldApprehend",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FApprehendShouldApprehendTest::RunTest(const FString& Parameters)
{
    // Wanted at low heat, unarmed, calm -> move in to arrest.
    TestTrue(TEXT("1 star unarmed calm -> apprehend"), FApprehend::ShouldApprehend(1, false, 0.0));
    TestTrue(TEXT("2 stars unarmed calm -> apprehend"), FApprehend::ShouldApprehend(2, false, 0.2));
    // Not wanted -> never.
    TestFalse(TEXT("0 stars -> no apprehend"), FApprehend::ShouldApprehend(0, false, 0.0));
    // High heat -> lethal response, no cuffing.
    TestFalse(TEXT("3 stars -> gunfight, not apprehend"), FApprehend::ShouldApprehend(3, false, 0.0));
    // Armed suspect -> never close in to cuff.
    TestFalse(TEXT("armed -> no apprehend"), FApprehend::ShouldApprehend(1, true, 0.0));
    // Threatening (even if technically unarmed) -> no.
    TestFalse(TEXT("high threat -> no apprehend"), FApprehend::ShouldApprehend(1, false, 0.9));
    // Threat exactly at the ceiling is excluded (strict <).
    TestFalse(TEXT("threat at ceiling -> no apprehend"),
        FApprehend::ShouldApprehend(1, false, FApprehend::ThreatCeiling));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FApprehendApproachTest,
    "GTC.AI.Combat.Apprehend.Approach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FApprehendApproachTest::RunTest(const FString& Parameters)
{
    // Catch-range boundary is inclusive.
    TestTrue(TEXT("inside catch range"), FApprehend::InCatchRange(1.5, 2.0));
    TestTrue(TEXT("exactly at catch range"), FApprehend::InCatchRange(2.0, 2.0));
    TestFalse(TEXT("outside catch range"), FApprehend::InCatchRange(2.5, 2.0));

    // Approach heading points at the target, planar (Y stays 0).
    const FVector Self(0, 0, 0);
    const FVector Target(10, 5, 20); // Y=5 must be ignored
    const FVector H = FApprehend::ApproachHeading(Self, Target);
    TestEqual(TEXT("heading is planar (Y==0)"), H.Y, 0.0, Eps);
    TestEqual(TEXT("heading is unit length"), H.Size(), 1.0, Eps);
    // Coincident planar position -> zero heading.
    TestTrue(TEXT("coincident -> zero heading"),
        FApprehend::ApproachHeading(Self, FVector(0, 99, 0)).IsNearlyZero());

    // Approach speed: full run while closing, plant inside catch range.
    TestEqual(TEXT("full run while closing"), FApprehend::ApproachSpeed(460.0, 10.0, 2.0), 460.0, Eps);
    TestEqual(TEXT("plant inside catch range"), FApprehend::ApproachSpeed(460.0, 1.0, 2.0), 0.0, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
