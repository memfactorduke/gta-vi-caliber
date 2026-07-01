// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FixedWingFlightResolver.h"
#include "../FixedWingFlight.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FFixedWingFlightResolver — integrating the flight model's velocity into a world position
 * with a ground floor + stall/altitude telemetry, plus the stall flight envelope driven through the
 * real FFixedWingFlight core. Prefix GTC.Vehicles.FixedWingResolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFixedWingFlightResolverTest,
    "GTC.Vehicles.FixedWingResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFixedWingFlightResolverTest::RunTest(const FString& Parameters)
{
    // --- Integration + ground floor + stall telemetry. ---
    {
        FFixedWingFlightResolver::FInput In;
        In.Position = FVector(0, 0, 1000);
        In.Velocity = FVector(1500, 0, 100);
        In.Dt = 1.0;
        In.Airspeed = 1500.0;
        In.StallSpeed = 800.0;
        const FFixedWingFlightResolver::FOutput O = FFixedWingFlightResolver::Resolve(In);
        TestTrue(TEXT("position advances by velocity * dt"), O.Position.X == 1500.0 && O.Position.Z == 1100.0);
        TestFalse(TEXT("airborne"), O.bGrounded);
        TestTrue(TEXT("ground speed is horizontal velocity"), FMath::IsNearlyEqual(O.GroundSpeedCmS, 1500.0, Eps));
        TestFalse(TEXT("above stall -> flying"), O.bStalled);
        TestTrue(TEXT("stall margin is the airspeed cushion"),
            FMath::IsNearlyEqual(O.StallMargin, (1500.0 - 800.0) / 800.0, Eps));
    }
    {
        FFixedWingFlightResolver::FInput In;
        In.Velocity = FVector(500, 0, -600);
        In.Airspeed = 500.0;
        In.StallSpeed = 800.0;
        const FFixedWingFlightResolver::FOutput O = FFixedWingFlightResolver::Resolve(In);
        TestTrue(TEXT("below stall -> stalled"), O.bStalled);
        TestTrue(TEXT("below stall -> zero margin"), FMath::IsNearlyEqual(O.StallMargin, 0.0, Eps));
    }
    {
        FFixedWingFlightResolver::FInput In;
        In.Position = FVector(0, 0, 5);
        In.Velocity = FVector(0, 0, -100);
        In.Dt = 1.0;
        In.StallSpeed = 800.0;
        const FFixedWingFlightResolver::FOutput O = FFixedWingFlightResolver::Resolve(In);
        TestTrue(TEXT("cannot sink through the runway"), FMath::IsNearlyEqual(O.Position.Z, 0.0, Eps));
        TestTrue(TEXT("grounded"), O.bGrounded);
    }

    // --- Flight envelope through the real model. ---
    FFixedWingFlight::FParams P;
    {
        FFixedWingFlight F;
        F.Configure(P);
        TestTrue(TEXT("starts stalled at rest"), F.IsStalled());
        for (int32 i = 0; i < 200; ++i)
        {
            F.Update(1.0, 0.0, 0.0, 0.1);
        }
        TestTrue(TEXT("full throttle accelerates past stall"), F.Airspeed() > P.StallSpeed);
        TestFalse(TEXT("fast enough -> flying"), F.IsStalled());
    }
    {
        FFixedWingFlight F;
        F.Configure(P);
        F.Update(0.0, 0.0, 0.0, 0.1);
        TestTrue(TEXT("stalled -> sinks"), F.ClimbRate() < 0.0);
    }
    {
        FFixedWingFlight Level;
        Level.Configure(P);
        FFixedWingFlight Climb;
        Climb.Configure(P);
        for (int32 i = 0; i < 120; ++i)
        {
            Level.Update(1.0, 0.0, 0.0, 0.1);
            Climb.Update(1.0, 0.0, 0.0, 0.1);
        }
        for (int32 i = 0; i < 20; ++i)
        {
            Level.Update(1.0, 0.0, 0.0, 0.1);
            Climb.Update(1.0, 1.0, 0.0, 0.1);
        }
        TestTrue(TEXT("up-elevator climbs"), Climb.ClimbRate() > 0.0);
        TestTrue(TEXT("climbing costs airspeed"), Climb.Airspeed() < Level.Airspeed());
    }
    {
        FFixedWingFlight F;
        F.Configure(P);
        for (int32 i = 0; i < 120; ++i)
        {
            F.Update(1.0, 0.0, 0.0, 0.1);
        }
        const double H0 = F.Heading();
        for (int32 i = 0; i < 20; ++i)
        {
            F.Update(1.0, 0.0, 1.0, 0.1);
        }
        TestTrue(TEXT("positive aileron turns heading right (increasing yaw)"), F.Heading() > H0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
