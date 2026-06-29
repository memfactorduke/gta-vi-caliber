// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NavalPursuit.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FNavalPursuit — the Coast Guard's water-plane pursuit, reusing FPursuitTactics
 * across a UE<->Godot frame remap. GTC-original. Pins the lead-intercept staying on the
 * water plane, the stationary-target round-trip, the broadside firing run, ride-the-water /
 * ground-on-shore clamping, and the ram decision. Mirrors
 * Scripts/gtc_coastguard/naval_pursuit_verify.cpp. Prefix GTC.AI.Pursuit.NavalPursuit.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNavalPursuitTest,
    "GTC.AI.Pursuit.NavalPursuit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNavalPursuitTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("deploy star = coast guard escalation"), FNavalPursuit::DeployStars, 4);

    // ---- Intercept on the water plane -----------------------------------------
    {
        const FVector R = FNavalPursuit::InterceptPoint(FVector(0, 0, 0), FVector(100, 0, 0), FVector(0, -1000, 0), 200.0);
        TestTrue(TEXT("intercept stays on the water plane"), FMath::Abs(R.Z) <= Eps);
        TestTrue(TEXT("intercept leads +X travel"), R.X > 0.0);
    }

    // ---- Stationary target -> its position (remap round-trips) ----------------
    {
        const FVector R = FNavalPursuit::InterceptPoint(FVector(5, 6, 7), FVector::ZeroVector, FVector(0, 0, 0), 100.0);
        TestTrue(TEXT("stationary -> its pos"),
            FMath::Abs(R.X - 5) <= Eps && FMath::Abs(R.Y - 6) <= Eps && FMath::Abs(R.Z - 7) <= Eps);
    }

    // ---- Broadside firing run -------------------------------------------------
    {
        const FVector Star = FNavalPursuit::BroadsideOffset(FVector(0, 0, 0), FVector(100, 0, 0), +1.0, 500.0);
        TestTrue(TEXT("starboard run is -Y of +X travel"),
            FMath::Abs(Star.X) <= Eps && FMath::Abs(Star.Y + 500.0) <= Eps && FMath::Abs(Star.Z) <= Eps);
        const FVector Port = FNavalPursuit::BroadsideOffset(FVector(0, 0, 0), FVector(100, 0, 0), -1.0, 500.0);
        TestTrue(TEXT("port run is +Y"), FMath::Abs(Port.Y - 500.0) <= Eps);
        const FVector NoHeading = FNavalPursuit::BroadsideOffset(FVector(0, 0, 0), FVector::ZeroVector, +1.0, 500.0);
        TestTrue(TEXT("no heading -> step along +Y"), FMath::Abs(NoHeading.Y - 500.0) <= Eps);
    }

    // ---- Clamp to water -------------------------------------------------------
    {
        const FVector Deep = FNavalPursuit::ClampToWater(FVector(10, 20, 999), -50.0, 0.0);
        TestTrue(TEXT("deep water -> rides sea level"),
            FMath::Abs(Deep.Z) <= Eps && FMath::Abs(Deep.X - 10) <= Eps && FMath::Abs(Deep.Y - 20) <= Eps);
        const FVector Beach = FNavalPursuit::ClampToWater(FVector(10, 20, 999), 30.0, 0.0);
        TestTrue(TEXT("shore above water -> grounds"), FMath::Abs(Beach.Z - 30.0) <= Eps);
    }

    // ---- Ram decision ---------------------------------------------------------
    {
        const FVector Pp(0, 0, 0), Heading(100, 0, 0), Near(200, 0, 0), Far(5000, 0, 0);
        TestTrue(TEXT("close + aligned + 4 stars -> ram"), FNavalPursuit::ShouldRamHull(Pp, Heading, Near, 500.0, 4));
        TestFalse(TEXT("low heat -> no ram"), FNavalPursuit::ShouldRamHull(Pp, Heading, Near, 500.0, 2));
        TestFalse(TEXT("out of range -> no ram"), FNavalPursuit::ShouldRamHull(Pp, Heading, Far, 500.0, 4));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
