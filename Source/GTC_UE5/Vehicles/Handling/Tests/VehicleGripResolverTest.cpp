// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleGripResolver.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FVehicleGripResolver — the per-frame composition of FRoadGrip (wet-road
 * grip) x tyre grip -> FVehicleHandling (slip/drift/lateral retention). Prefix
 * GTC.Vehicles.GripResolver.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleGripResolverTest,
    "GTC.Vehicles.GripResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleGripResolverTest::RunTest(const FString& Parameters)
{
    const FRoadGrip::FParams P; // defaults: WetGripLoss 0.35, aquaplane 90..160 km/h, floor 0.25
    const FVector Fwd(1, 0, 0);
    const FVector Right(0, 1, 0);
    const double KmhToCms = 1.0 / FVehicleGripResolver::CmsToKmh; // km/h -> cm/s for building velocities

    // A car tracking straight ahead at a given ground speed (km/h) on chosen tyres.
    auto MakeIn = [&](double Wetness, double GroundKmh, double TyreGrip, bool bHandbrake)
    {
        FVehicleGripResolver::FInput In;
        In.Velocity = Fwd * (GroundKmh * KmhToCms);
        In.Forward = Fwd;
        In.Right = Right;
        In.Wetness = Wetness;
        In.TyreGrip = TyreGrip;
        In.bHandbrake = bHandbrake;
        return In;
    };

    // --- Dry road: the identity. Grip = tyre grip, nothing slides, no aquaplane. ---
    {
        const FVehicleGripResolver::FOutput O = FVehicleGripResolver::Resolve(MakeIn(0.0, 72.0, 1.0, false), P);
        TestTrue(TEXT("dry road -> full road grip"), FMath::IsNearlyEqual(O.RoadGrip, 1.0, GtcTest::Eps));
        TestTrue(TEXT("dry + fresh tyres -> full grip factor"), FMath::IsNearlyEqual(O.GripFactor, 1.0, GtcTest::Eps));
        TestFalse(TEXT("dry road never aquaplanes"), O.bAquaplaning);
        TestTrue(TEXT("tracking straight -> no drift"), FMath::IsNearlyEqual(O.DriftFactor, 0.0, GtcTest::Eps));
        TestTrue(TEXT("ground speed reported in km/h"), FMath::IsNearlyEqual(O.GroundSpeedKmh, 72.0, 1e-3));
    }

    // --- Wet but slow: exact wet grip 1 - WetGripLoss, and no aquaplaning below the threshold. ---
    FVehicleGripResolver::FOutput WetSlow = FVehicleGripResolver::Resolve(MakeIn(1.0, 50.0, 1.0, false), P);
    {
        TestTrue(TEXT("wet-slow road grip == 1 - WetGripLoss (0.65)"),
            FMath::IsNearlyEqual(WetSlow.RoadGrip, 0.65, GtcTest::Eps));
        TestFalse(TEXT("wet but under aquaplane speed -> not aquaplaning"), WetSlow.bAquaplaning);
    }

    // --- Wet and fast: grip collapses to the aquaplane floor and the tail steps out more. ---
    FVehicleGripResolver::FOutput WetFast = FVehicleGripResolver::Resolve(MakeIn(1.0, 200.0, 1.0, false), P);
    FVehicleGripResolver::FOutput DryFast = FVehicleGripResolver::Resolve(MakeIn(0.0, 200.0, 1.0, false), P);
    {
        TestTrue(TEXT("wet + fast aquaplanes"), WetFast.bAquaplaning);
        TestTrue(TEXT("fully-aquaplaning road grip == AquaplaneFloor (0.25)"),
            FMath::IsNearlyEqual(WetFast.RoadGrip, 0.25, GtcTest::Eps));
        // Worse grip => the tyres keep MORE of any sideways velocity (it slides more).
        TestTrue(TEXT("aquaplaning slides more than a dry road"),
            WetFast.LateralRetention > DryFast.LateralRetention + GtcTest::Eps);
    }

    // --- Worn tyres compound with the road: same road, less tyre grip => less total grip. ---
    {
        const FVehicleGripResolver::FOutput Worn = FVehicleGripResolver::Resolve(MakeIn(1.0, 50.0, 0.6, false), P);
        TestTrue(TEXT("worn tyres cut the grip factor below fresh on the same wet road"),
            Worn.GripFactor < WetSlow.GripFactor - GtcTest::Eps);
        TestTrue(TEXT("tyre wear does not change the road's own grip"),
            FMath::IsNearlyEqual(Worn.RoadGrip, WetSlow.RoadGrip, GtcTest::Eps));
        TestTrue(TEXT("grip factor == road grip x tyre grip"),
            FMath::IsNearlyEqual(Worn.GripFactor, 0.65 * 0.6, GtcTest::Eps));
    }

    // --- Lateral retention hits its documented endpoints for grip vs handbrake. ---
    {
        const FVehicleGripResolver::FOutput Grip = FVehicleGripResolver::Resolve(MakeIn(0.0, 72.0, 1.0, false), P);
        const FVehicleGripResolver::FOutput Hand = FVehicleGripResolver::Resolve(MakeIn(0.0, 72.0, 1.0, true), P);
        TestTrue(TEXT("full grip, gripping -> GripRetention (0.10)"),
            FMath::IsNearlyEqual(Grip.LateralRetention, 0.10, GtcTest::Eps));
        TestTrue(TEXT("full grip, handbrake -> HandbrakeRetention (0.85)"),
            FMath::IsNearlyEqual(Hand.LateralRetention, 0.85, GtcTest::Eps));
    }

    // --- An actual forward slide: heading forward but moving partly sideways. ---
    {
        FVehicleGripResolver::FInput In;
        In.Velocity = FVector(1000, 600, 0); // forward-and-sideways (cm/s)
        In.Forward = Fwd;
        In.Right = Right;
        In.Wetness = 0.0;
        In.TyreGrip = 1.0;
        In.bHandbrake = false;

        const FVehicleGripResolver::FOutput O = FVehicleGripResolver::Resolve(In, P);
        TestTrue(TEXT("sliding sideways -> positive slip angle"), O.SlipAngleRad > GtcTest::Eps);
        TestTrue(TEXT("sliding forward at speed -> drifting"), O.DriftFactor > 0.0);
        TestTrue(TEXT("gripped velocity preserves forward speed"),
            FMath::IsNearlyEqual(FVector::DotProduct(O.GrippedVelocity, Fwd), 1000.0, 1.0));
        TestTrue(TEXT("gripped velocity kills most of the sideways slide"),
            FMath::Abs(FVector::DotProduct(O.GrippedVelocity, Right)) < 600.0);
    }

    // --- Reversing straight is NOT a drift (regression: unsigned slip would read pi -> full drift). ---
    {
        FVehicleGripResolver::FInput In;
        In.Velocity = FVector(-1500, 0, 0); // straight backwards
        In.Forward = Fwd;
        In.Right = Right;
        const FVehicleGripResolver::FOutput O = FVehicleGripResolver::Resolve(In, P);
        TestTrue(TEXT("reversing straight scores no drift"), FMath::IsNearlyEqual(O.DriftFactor, 0.0, GtcTest::Eps));
    }

    // --- Airborne / falling: vertical drop is not road speed, so no phantom aquaplane/drift. ---
    {
        FVehicleGripResolver::FInput In;
        In.Velocity = FVector(0, 0, -6000); // plummeting straight down
        In.Forward = Fwd;
        In.Right = Right;
        In.Wetness = 1.0; // soaking — the old bug would aquaplane mid-air
        In.TyreGrip = 1.0;
        const FVehicleGripResolver::FOutput O = FVehicleGripResolver::Resolve(In, P);
        TestTrue(TEXT("falling straight down -> zero ground speed"), FMath::IsNearlyEqual(O.GroundSpeedKmh, 0.0, 1e-3));
        TestFalse(TEXT("a falling car does not aquaplane"), O.bAquaplaning);
        TestTrue(TEXT("a falling car does not drift"), FMath::IsNearlyEqual(O.DriftFactor, 0.0, GtcTest::Eps));
    }

    // --- Parked car: everything degenerate-safe, dry grip full. ---
    {
        FVehicleGripResolver::FInput In;
        In.Forward = Fwd;
        In.Right = Right;
        const FVehicleGripResolver::FOutput O = FVehicleGripResolver::Resolve(In, P);
        TestTrue(TEXT("parked -> no slip"), FMath::IsNearlyEqual(O.SlipAngleRad, 0.0, GtcTest::Eps));
        TestTrue(TEXT("parked -> no drift"), FMath::IsNearlyEqual(O.DriftFactor, 0.0, GtcTest::Eps));
        TestTrue(TEXT("parked on a dry road -> full grip"), FMath::IsNearlyEqual(O.GripFactor, 1.0, GtcTest::Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
