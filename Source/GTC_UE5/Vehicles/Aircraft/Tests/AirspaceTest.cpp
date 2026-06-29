// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Airspace.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FAircraftAirspace — the soft service ceiling + restricted floor. GTC-original
 * (no Godot oracle). Pins the climb-authority ramp, that only ascent is restricted (you
 * can always dive back down), the collapsed/inverted-band step, and the no-fly floor.
 * Z-up, cm. Mirrors Scripts/gtc_aircraft/airspace_verify.cpp. Prefix
 * GTC.Vehicles.Aircraft.Airspace.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAircraftAirspaceTest,
    "GTC.Vehicles.Aircraft.Airspace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAircraftAirspaceTest::RunTest(const FString& Parameters)
{
    const FAircraftAirspace::FParams P; // soft 200000, hard 250000

    // ---- Ceiling authority ramp -----------------------------------------------
    {
        TestTrue(TEXT("full authority at sea level"),
            FMath::IsNearlyEqual(FAircraftAirspace::CeilingAuthority(0.0, P), 1.0, Eps));
        TestTrue(TEXT("full at soft ceiling"),
            FMath::IsNearlyEqual(FAircraftAirspace::CeilingAuthority(200000.0, P), 1.0, Eps));
        TestTrue(TEXT("half-way through band -> 0.5"),
            FMath::IsNearlyEqual(FAircraftAirspace::CeilingAuthority(225000.0, P), 0.5, Eps));
        TestTrue(TEXT("zero at hard ceiling"), FAircraftAirspace::CeilingAuthority(250000.0, P) <= Eps);
        TestTrue(TEXT("zero above hard ceiling"), FAircraftAirspace::CeilingAuthority(300000.0, P) <= Eps);
    }

    // ---- ClampClimb only restricts ascent -------------------------------------
    {
        TestTrue(TEXT("low altitude: full climb"),
            FMath::IsNearlyEqual(FAircraftAirspace::ClampClimb(0.0, 500.0, P), 500.0, Eps));
        TestTrue(TEXT("mid band: climb halved"),
            FMath::IsNearlyEqual(FAircraftAirspace::ClampClimb(225000.0, 500.0, P), 250.0, Eps));
        TestTrue(TEXT("at ceiling: no climb"), FMath::Abs(FAircraftAirspace::ClampClimb(250000.0, 500.0, P)) <= Eps);
        TestTrue(TEXT("can always descend from ceiling"),
            FMath::IsNearlyEqual(FAircraftAirspace::ClampClimb(250000.0, -500.0, P), -500.0, Eps));
        TestTrue(TEXT("descent never scaled"),
            FMath::IsNearlyEqual(FAircraftAirspace::ClampClimb(0.0, -300.0, P), -300.0, Eps));
    }

    // ---- Degenerate / inverted band: clean step -------------------------------
    {
        FAircraftAirspace::FParams D;
        D.CeilingZCm = 100000.0;
        D.SoftCeilingZCm = 100000.0; // soft == hard
        TestTrue(TEXT("below collapsed ceiling -> 1"),
            FMath::IsNearlyEqual(FAircraftAirspace::CeilingAuthority(99999.0, D), 1.0, Eps));
        TestTrue(TEXT("at collapsed ceiling -> 0"), FAircraftAirspace::CeilingAuthority(100000.0, D) <= Eps);

        FAircraftAirspace::FParams E;
        E.CeilingZCm = 100000.0;
        E.SoftCeilingZCm = 150000.0; // soft above hard: clamped down internally
        TestTrue(TEXT("inverted band sane below ceiling"),
            FMath::IsNearlyEqual(FAircraftAirspace::CeilingAuthority(99999.0, E), 1.0, Eps));
        TestTrue(TEXT("inverted band zero above ceiling"), FAircraftAirspace::CeilingAuthority(100001.0, E) <= Eps);
    }

    // ---- Restricted floor -----------------------------------------------------
    {
        TestTrue(TEXT("floor = ground + min alt"),
            FMath::IsNearlyEqual(FAircraftAirspace::RestrictedFloorZ(1000.0, 5000.0), 6000.0, Eps));
        TestTrue(TEXT("negative min alt -> ground"),
            FMath::IsNearlyEqual(FAircraftAirspace::RestrictedFloorZ(1000.0, -50.0), 1000.0, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
