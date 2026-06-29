// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleGroundContact.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;
using EContact = FVehicleGroundContact::EContact;
using ETouchdown = FVehicleGroundContact::ETouchdown;

/**
 * Tests for FVehicleGroundContact — the kinematic ground/landing clamp brain.
 * GTC-original (no Godot oracle). Pins AGL/rest-height helpers, the soft/hard/crash
 * touchdown classification + hardness curve, the airborne<->grounded edge with its
 * clamp, the spawn-below-ground lift, and the liftoff hysteresis margin. Z-up, cm.
 * Mirrors Scripts/gtc_aircraft/ground_contact_verify.cpp. Prefix GTC.Vehicles.Contact.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleGroundContactTest,
    "GTC.Vehicles.Contact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleGroundContactTest::RunTest(const FString& Parameters)
{
    const FVehicleGroundContact::FParams P; // gear 150, soft 300, crash 800, liftoff 25

    // ---- Plain helpers --------------------------------------------------------
    {
        TestTrue(TEXT("AGL = body - ground"),
            FMath::IsNearlyEqual(FVehicleGroundContact::AltitudeAGL(1000.0, 200.0), 800.0, Eps));
        TestTrue(TEXT("rest Z = ground + gear"),
            FMath::IsNearlyEqual(FVehicleGroundContact::RestZCm(200.0, P), 350.0, Eps));
    }

    // ---- Touchdown classification + hardness curve ----------------------------
    {
        TestTrue(TEXT("0 speed -> Soft"), FVehicleGroundContact::ClassifyTouchdown(0.0, P) == ETouchdown::Soft);
        TestTrue(TEXT("at soft thr -> Soft"), FVehicleGroundContact::ClassifyTouchdown(300.0, P) == ETouchdown::Soft);
        TestTrue(TEXT("mid band -> Hard"), FVehicleGroundContact::ClassifyTouchdown(500.0, P) == ETouchdown::Hard);
        TestTrue(TEXT("at crash thr -> Crash"), FVehicleGroundContact::ClassifyTouchdown(800.0, P) == ETouchdown::Crash);
        TestTrue(TEXT("way past -> Crash"), FVehicleGroundContact::ClassifyTouchdown(9999.0, P) == ETouchdown::Crash);

        TestTrue(TEXT("hardness 0 below soft"), FVehicleGroundContact::ImpactHardness01(200.0, P) <= Eps);
        TestTrue(TEXT("hardness 0.5 mid band"),
            FMath::IsNearlyEqual(FVehicleGroundContact::ImpactHardness01(550.0, P), 0.5, Eps));
        TestTrue(TEXT("hardness 1 at crash"),
            FMath::IsNearlyEqual(FVehicleGroundContact::ImpactHardness01(800.0, P), 1.0, Eps));
        TestTrue(TEXT("hardness clamps at 1"),
            FMath::IsNearlyEqual(FVehicleGroundContact::ImpactHardness01(5000.0, P), 1.0, Eps));
    }

    // ---- No floor below -> airborne, never clamps -----------------------------
    {
        const FVehicleGroundContact::FResult R =
            FVehicleGroundContact::Evaluate(5000.0, /*hit*/ false, 0.0, -1000.0, EContact::Airborne, P);
        TestTrue(TEXT("no hit -> airborne"), R.Contact == EContact::Airborne);
        TestFalse(TEXT("no hit -> no clamp"), R.bClampToGround);
        TestTrue(TEXT("no hit -> no touchdown"), R.Touchdown == ETouchdown::None);
    }

    // ---- High above ground, descending fast: still flying ---------------------
    {
        const FVehicleGroundContact::FResult R =
            FVehicleGroundContact::Evaluate(5000.0, true, 0.0, -700.0, EContact::Airborne, P);
        TestTrue(TEXT("above rest height stays airborne"), R.Contact == EContact::Airborne);
        TestFalse(TEXT("airborne -> no clamp"), R.bClampToGround);
    }

    // ---- Soft / Hard / Crash touchdown on the airborne->grounded edge ---------
    {
        // Ground 0, gear 150 -> rest height 150; reaching it from airborne is a touchdown.
        const FVehicleGroundContact::FResult Soft =
            FVehicleGroundContact::Evaluate(150.0, true, 0.0, -100.0, EContact::Airborne, P);
        TestTrue(TEXT("reach rest -> grounded"), Soft.Contact == EContact::Grounded);
        TestTrue(TEXT("slow descent -> Soft"), Soft.Touchdown == ETouchdown::Soft);
        TestTrue(TEXT("soft clamps to rest Z"), Soft.bClampToGround && FMath::IsNearlyEqual(Soft.ClampZCm, 150.0, Eps));
        TestTrue(TEXT("soft hardness 0"), Soft.ImpactHardness01 <= Eps);

        const FVehicleGroundContact::FResult Hard =
            FVehicleGroundContact::Evaluate(150.0, true, 0.0, -550.0, EContact::Airborne, P);
        TestTrue(TEXT("mid descent -> Hard"), Hard.Touchdown == ETouchdown::Hard);
        TestTrue(TEXT("hard hardness in (0,1)"), Hard.ImpactHardness01 > 0.0 && Hard.ImpactHardness01 < 1.0);

        const FVehicleGroundContact::FResult Crash =
            FVehicleGroundContact::Evaluate(150.0, true, 0.0, -1200.0, EContact::Airborne, P);
        TestTrue(TEXT("fast descent -> Crash"), Crash.Touchdown == ETouchdown::Crash);
        TestTrue(TEXT("crash hardness 1"), FMath::IsNearlyEqual(Crash.ImpactHardness01, 1.0, Eps));
    }

    // ---- Spawned below ground: lifted to rest as a zero-speed Soft set-down ----
    {
        const FVehicleGroundContact::FResult R =
            FVehicleGroundContact::Evaluate(-500.0, true, 0.0, 0.0, EContact::Airborne, P);
        TestTrue(TEXT("below ground -> grounded"), R.Contact == EContact::Grounded);
        TestTrue(TEXT("below ground clamps UP to rest Z"),
            R.bClampToGround && FMath::IsNearlyEqual(R.ClampZCm, 150.0, Eps));
        TestTrue(TEXT("zero-speed contact is soft"), R.Touchdown == ETouchdown::Soft && R.ImpactHardness01 <= Eps);
    }

    // ---- Grounded resting: held on the floor, no fresh touchdown ---------------
    {
        const FVehicleGroundContact::FResult R =
            FVehicleGroundContact::Evaluate(151.0, true, 0.0, 0.0, EContact::Grounded, P);
        TestTrue(TEXT("settled stays grounded"), R.Contact == EContact::Grounded);
        TestTrue(TEXT("settled holds rest Z"), R.bClampToGround && FMath::IsNearlyEqual(R.ClampZCm, 150.0, Eps));
        TestTrue(TEXT("settled is not a fresh touchdown"), R.Touchdown == ETouchdown::None);
    }

    // ---- Hysteresis: within the liftoff margin stays grounded -----------------
    {
        // rest 150, margin 25 -> still grounded at 170, airborne past 175.
        const FVehicleGroundContact::FResult Within =
            FVehicleGroundContact::Evaluate(170.0, true, 0.0, 50.0, EContact::Grounded, P);
        TestTrue(TEXT("within margin stays grounded"), Within.Contact == EContact::Grounded);
        TestTrue(TEXT("within margin still clamps"), Within.bClampToGround);

        const FVehicleGroundContact::FResult Lifted =
            FVehicleGroundContact::Evaluate(400.0, true, 0.0, 200.0, EContact::Grounded, P);
        TestTrue(TEXT("clear of margin -> airborne"), Lifted.Contact == EContact::Airborne);
        TestFalse(TEXT("lifted -> no clamp"), Lifted.bClampToGround);
        TestTrue(TEXT("takeoff is not a touchdown"), Lifted.Touchdown == ETouchdown::None);
    }

    // ---- Non-zero ground Z: everything is relative to the traced floor ---------
    {
        // ground 1000 -> rest 1150.
        const FVehicleGroundContact::FResult R =
            FVehicleGroundContact::Evaluate(1150.0, true, 1000.0, -100.0, EContact::Airborne, P);
        TestTrue(TEXT("touchdown relative to raised floor"), R.Contact == EContact::Grounded);
        TestTrue(TEXT("clamp Z tracks the floor"), FMath::IsNearlyEqual(R.ClampZCm, 1150.0, Eps));
    }

    // ---- Degenerate params don't invert the band ------------------------------
    {
        FVehicleGroundContact::FParams D;
        D.SoftLandingSpeedCm = 400.0;
        D.CrashLandingSpeedCm = 100.0; // crash < soft: clamped up to soft internally
        TestTrue(TEXT("past soft is crash when band collapses"),
            FVehicleGroundContact::ClassifyTouchdown(401.0, D) == ETouchdown::Crash);
        TestTrue(TEXT("collapsed band -> full hardness past soft"),
            FMath::IsNearlyEqual(FVehicleGroundContact::ImpactHardness01(401.0, D), 1.0, Eps));
        TestTrue(TEXT("below soft still soft"),
            FVehicleGroundContact::ClassifyTouchdown(399.0, D) == ETouchdown::Soft);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
