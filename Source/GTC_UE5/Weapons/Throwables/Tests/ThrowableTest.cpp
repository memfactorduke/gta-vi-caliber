// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Throwable.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FThrowable — grenade cook/fuse and the ballistic throw arc.
 * Prefix GTC.Weapons.Throwable.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FThrowableTest,
    "GTC.Weapons.Throwable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FThrowableTest::RunTest(const FString& Parameters)
{
    const double G = FThrowable::DefaultGravity;

    // --- Fuse / cook ---
    TestTrue(TEXT("cooking shortens the fuse"),
        FMath::IsNearlyEqual(FThrowable::FuseAfterCook(3.5, 1.0), 2.5, GtcTest::Eps));
    TestFalse(TEXT("a brief cook does not blow up in hand"), FThrowable::CookedOff(3.5, 1.0));
    TestTrue(TEXT("cooking past the fuse blows up in hand"), FThrowable::CookedOff(3.5, 4.0));
    TestTrue(TEXT("fuse never goes negative"),
        FMath::IsNearlyEqual(FThrowable::FuseAfterCook(3.5, 99.0), 0.0, GtcTest::Eps));

    // --- Launch velocity ---
    {
        const FVector V = FThrowable::LaunchVelocity(FVector(0, 0, 10), 50.0); // unnormalised up
        TestTrue(TEXT("launch speed matches the throw speed"),
            FMath::IsNearlyEqual(V.Size(), 50.0, GtcTest::Eps));
        TestTrue(TEXT("degenerate aim yields no launch"),
            FThrowable::LaunchVelocity(FVector::ZeroVector, 50.0).IsNearlyZero());
    }

    // --- PositionAtTime ---
    {
        const FVector Origin(0, 0, 100);
        const FVector Vel(200, 0, 0); // thrown level
        TestTrue(TEXT("t=0 is the origin"),
            (FThrowable::PositionAtTime(Origin, Vel, G, 0.0) - Origin).IsNearlyZero());

        const FVector At1 = FThrowable::PositionAtTime(Origin, Vel, G, 1.0);
        TestTrue(TEXT("horizontal travels at launch speed"),
            FMath::IsNearlyEqual(At1.X, 200.0, GtcTest::Eps));
        TestTrue(TEXT("gravity drops it on Z"),
            FMath::IsNearlyEqual(At1.Z, 100.0 - 0.5 * G * 1.0, GtcTest::Eps));
    }

    // --- TimeToGround ---
    {
        // Dropped from height H with no horizontal/vertical launch: t = sqrt(2H/g).
        const double H = 490.0;
        const double T = FThrowable::TimeToGround(FVector(0, 0, H), FVector::ZeroVector, G, 0.0);
        TestTrue(TEXT("free-fall time matches sqrt(2H/g)"),
            FMath::IsNearlyEqual(T, FMath::Sqrt(2.0 * H / G), GtcTest::Eps));

        // Thrown straight up from ground level returns to ground at t = 2*Vz/g.
        const double Vz = 300.0;
        const double Tup = FThrowable::TimeToGround(FVector(0, 0, 0), FVector(0, 0, Vz), G, 0.0);
        TestTrue(TEXT("up-throw returns at 2*Vz/g"),
            FMath::IsNearlyEqual(Tup, 2.0 * Vz / G, GtcTest::Eps));

        // A throw that never descends to a plane above its apex returns -1.
        TestTrue(TEXT("never reaching a high plane returns -1"),
            FThrowable::TimeToGround(FVector(0, 0, 0), FVector(0, 0, 10), G, 100000.0) < 0.0);
    }

    // --- PredictLanding ---
    {
        const FVector Origin(0, 0, 100);
        const FVector Vel(200, 0, 0);
        const FVector Land = FThrowable::PredictLanding(Origin, Vel, G, 0.0);
        TestTrue(TEXT("landing pins to the ground plane"), FMath::IsNearlyEqual(Land.Z, 0.0, GtcTest::Eps));
        TestTrue(TEXT("a faster throw lands farther"),
            FThrowable::PredictLanding(Origin, FVector(400, 0, 0), G, 0.0).X > Land.X);
    }

    // --- DetonationPoint ---
    {
        const FVector Origin(0, 0, 100);
        const FVector Vel(200, 0, 0);

        // A long fuse: it lands well before the fuse expires, so it bursts on the ground.
        const FVector Rest = FThrowable::DetonationPoint(Origin, Vel, G, 0.0, 10.0);
        const FVector Land = FThrowable::PredictLanding(Origin, Vel, G, 0.0);
        TestTrue(TEXT("a long fuse detonates at the landing point"),
            (Rest - Land).IsNearlyZero());

        // A very short fuse: it air-bursts before touching down (still above ground).
        const FVector Air = FThrowable::DetonationPoint(Origin, Vel, G, 0.0, 0.05);
        TestTrue(TEXT("a short fuse air-bursts above the ground"), Air.Z > 0.0);
        TestTrue(TEXT("the air-burst matches the arc at the fuse time"),
            (Air - FThrowable::PositionAtTime(Origin, Vel, G, 0.05)).IsNearlyZero());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
