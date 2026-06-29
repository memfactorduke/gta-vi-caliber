// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../AirSeaEscalation.h"

/**
 * Tests for FAirSeaEscalation — when the police chopper scrambles and the Coast Guard
 * launches, by stars + player medium. GTC-original. Pins the canonical air-deploy star (3),
 * the early scramble for a flyer, the sea-only Coast Guard, and the unit counts. Mirrors
 * Scripts/gtc_airsea/airsea_escalation_verify.cpp. Prefix GTC.AI.PoliceDispatch.AirSeaEscalation.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAirSeaEscalationTest,
    "GTC.AI.PoliceDispatch.AirSeaEscalation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAirSeaEscalationTest::RunTest(const FString& Parameters)
{
    // ---- ClampStars -----------------------------------------------------------
    TestEqual(TEXT("clamp below 0"), FAirSeaEscalation::ClampStars(-5), 0);
    TestEqual(TEXT("clamp above max"), FAirSeaEscalation::ClampStars(99), 6);

    // ---- Police air deploy ----------------------------------------------------
    TestFalse(TEXT("land 2 -> no chopper"), FAirSeaEscalation::ShouldDeployPoliceAir(2, EPlayerMedium::Land));
    TestTrue(TEXT("land 3 -> chopper"), FAirSeaEscalation::ShouldDeployPoliceAir(3, EPlayerMedium::Land));
    TestTrue(TEXT("sea 3 -> chopper"), FAirSeaEscalation::ShouldDeployPoliceAir(3, EPlayerMedium::Sea));
    TestTrue(TEXT("air 2 -> chopper (early scramble)"), FAirSeaEscalation::ShouldDeployPoliceAir(2, EPlayerMedium::Air));
    TestFalse(TEXT("air 1 -> no chopper"), FAirSeaEscalation::ShouldDeployPoliceAir(1, EPlayerMedium::Air));

    // ---- Coast Guard deploy ---------------------------------------------------
    TestFalse(TEXT("sea 3 -> none"), FAirSeaEscalation::ShouldDeployCoastGuard(3, EPlayerMedium::Sea));
    TestTrue(TEXT("sea 4 -> launch"), FAirSeaEscalation::ShouldDeployCoastGuard(4, EPlayerMedium::Sea));
    TestFalse(TEXT("land 6 -> none"), FAirSeaEscalation::ShouldDeployCoastGuard(6, EPlayerMedium::Land));
    TestFalse(TEXT("air 6 -> none"), FAirSeaEscalation::ShouldDeployCoastGuard(6, EPlayerMedium::Air));

    // ---- Counts ---------------------------------------------------------------
    TestEqual(TEXT("no chopper -> 0"), FAirSeaEscalation::PoliceAirCount(2, EPlayerMedium::Land), 0);
    TestEqual(TEXT("3 -> 1 chopper"), FAirSeaEscalation::PoliceAirCount(3, EPlayerMedium::Land), 1);
    TestEqual(TEXT("5 -> 2 choppers"), FAirSeaEscalation::PoliceAirCount(5, EPlayerMedium::Land), 2);
    TestEqual(TEXT("air 2 -> 1 chopper"), FAirSeaEscalation::PoliceAirCount(2, EPlayerMedium::Air), 1);

    TestEqual(TEXT("sea 3 -> 0 boats"), FAirSeaEscalation::CoastGuardBoatCount(3, EPlayerMedium::Sea), 0);
    TestEqual(TEXT("sea 4 -> 1 boat"), FAirSeaEscalation::CoastGuardBoatCount(4, EPlayerMedium::Sea), 1);
    TestEqual(TEXT("sea 6 -> 3 boats (capped)"), FAirSeaEscalation::CoastGuardBoatCount(6, EPlayerMedium::Sea), 3);
    TestEqual(TEXT("land 6 -> 0 boats"), FAirSeaEscalation::CoastGuardBoatCount(6, EPlayerMedium::Land), 0);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
