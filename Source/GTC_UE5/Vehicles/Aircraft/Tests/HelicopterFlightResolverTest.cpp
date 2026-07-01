// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../HelicopterFlightResolver.h"
#include "../HelicopterFlight.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FHelicopterFlightResolver — integrating the rotor model's velocity into a world
 * position with a ground floor + HUD telemetry, plus the flight envelope driven through the real
 * FHelicopterFlight core. Prefix GTC.Vehicles.HelicopterResolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHelicopterFlightResolverTest,
    "GTC.Vehicles.HelicopterResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHelicopterFlightResolverTest::RunTest(const FString& Parameters)
{
    // --- Integration + ground floor + telemetry. ---
    {
        FHelicopterFlightResolver::FInput In;
        In.Position = FVector(0, 0, 1000);
        In.Velocity = FVector(100, 0, 50);
        In.Dt = 1.0;
        const FHelicopterFlightResolver::FOutput O = FHelicopterFlightResolver::Integrate(In);
        TestTrue(TEXT("position advances by velocity * dt"), O.Position.X == 100.0 && O.Position.Z == 1050.0);
        TestFalse(TEXT("high above the floor -> airborne"), O.bGrounded);
        TestTrue(TEXT("climb rate is vertical velocity"), FMath::IsNearlyEqual(O.ClimbRateCmS, 50.0, Eps));
        TestTrue(TEXT("ground speed is horizontal velocity"), FMath::IsNearlyEqual(O.GroundSpeedCmS, 100.0, Eps));
    }
    {
        FHelicopterFlightResolver::FInput In;
        In.Position = FVector(0, 0, 10);
        In.Velocity = FVector(0, 0, -100);
        In.Dt = 1.0;
        const FHelicopterFlightResolver::FOutput O = FHelicopterFlightResolver::Integrate(In);
        TestTrue(TEXT("cannot sink through the ground"), FMath::IsNearlyEqual(O.Position.Z, 0.0, Eps));
        TestTrue(TEXT("on the deck -> grounded"), O.bGrounded);
        TestTrue(TEXT("grounded -> no reported descent"), FMath::IsNearlyEqual(O.ClimbRateCmS, 0.0, Eps));
    }
    {
        FHelicopterFlightResolver::FInput In;
        In.Velocity = FVector(30, 40, 9999);
        const FHelicopterFlightResolver::FOutput O = FHelicopterFlightResolver::Integrate(In);
        TestTrue(TEXT("ground speed ignores vertical velocity"), FMath::IsNearlyEqual(O.GroundSpeedCmS, 50.0, Eps));
    }

    // --- Flight envelope through the real rotor model. ---
    FHelicopterFlight::FParams P;
    {
        FHelicopterFlight H;
        H.Configure(P);
        TestTrue(TEXT("hover collective cancels gravity"), FMath::IsNearlyEqual(H.HoverCollective(), 980.0 / 2000.0, Eps));
    }
    {
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int32 i = 0; i < 20; ++i)
        {
            H.Update(Hover, 0.0, 0.0, 0.0, 0.1);
        }
        TestTrue(TEXT("at hover collective the chopper holds altitude"), FMath::Abs(H.Velocity().Z) < 1.0);
    }
    {
        FHelicopterFlight Up;
        Up.Configure(P);
        FHelicopterFlight Down;
        Down.Configure(P);
        for (int32 i = 0; i < 10; ++i)
        {
            Up.Update(1.0, 0.0, 0.0, 0.0, 0.1);
            Down.Update(0.0, 0.0, 0.0, 0.0, 0.1);
        }
        TestTrue(TEXT("full collective climbs"), Up.Velocity().Z > 0.0);
        TestTrue(TEXT("no collective sinks"), Down.Velocity().Z < 0.0);
    }
    {
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int32 i = 0; i < 10; ++i)
        {
            H.Update(Hover, -1.0, 0.0, 0.0, 0.1);
        }
        FHelicopterFlightResolver::FInput In;
        In.Velocity = H.Velocity();
        TestTrue(TEXT("nose-down cyclic builds forward ground speed"),
            FHelicopterFlightResolver::Integrate(In).GroundSpeedCmS > 0.0);
    }
    {
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int32 i = 0; i < 10; ++i)
        {
            H.Update(Hover, 0.0, 0.0, 1.0, 0.1);
        }
        TestTrue(TEXT("full pedal turns the heading"), H.Heading() > 0.0);
    }
    {
        // Core is Godot-handed: raw +roll drives -Y (the adapter negates roll so bank-right slides right).
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int32 i = 0; i < 5; ++i)
        {
            H.Update(Hover, 0.0, 1.0, 0.0, 0.1);
        }
        TestTrue(TEXT("core convention: positive roll drives -Y (adapter negates it for UE)"), H.Velocity().Y < 0.0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
