// Headless verifier for FVehicleGroundContact. Compiles the REAL core .cpp against
// the shim and re-asserts the GTC.Vehicles.Contact invariants with a host clang —
// the same math the in-editor automation test checks, no engine required.
#include <cstdio>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/Vehicles/Contact/VehicleGroundContact.cpp"

using EContact = FVehicleGroundContact::EContact;
using ETouchdown = FVehicleGroundContact::ETouchdown;

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    const double Eps = 1e-9;
    const FVehicleGroundContact::FParams P; // gear 150, soft 300, crash 800, liftoff 25

    // ---- Plain helpers ---------------------------------------------------------
    {
        CHECK(Absd(FVehicleGroundContact::AltitudeAGL(1000.0, 200.0) - 800.0) < Eps, "AGL = body - ground");
        CHECK(Absd(FVehicleGroundContact::RestZCm(200.0, P) - 350.0) < Eps, "rest Z = ground + gear");
    }

    // ---- Touchdown classification + hardness curve -----------------------------
    {
        CHECK(FVehicleGroundContact::ClassifyTouchdown(0.0, P)   == ETouchdown::Soft,  "0 speed -> Soft");
        CHECK(FVehicleGroundContact::ClassifyTouchdown(300.0, P) == ETouchdown::Soft,  "at soft thr -> Soft");
        CHECK(FVehicleGroundContact::ClassifyTouchdown(500.0, P) == ETouchdown::Hard,  "mid band -> Hard");
        CHECK(FVehicleGroundContact::ClassifyTouchdown(800.0, P) == ETouchdown::Crash, "at crash thr -> Crash");
        CHECK(FVehicleGroundContact::ClassifyTouchdown(9999.0, P)== ETouchdown::Crash, "way past -> Crash");

        CHECK(FVehicleGroundContact::ImpactHardness01(200.0, P) < Eps, "hardness 0 below soft");
        CHECK(Absd(FVehicleGroundContact::ImpactHardness01(550.0, P) - 0.5) < Eps, "hardness 0.5 mid band");
        CHECK(Absd(FVehicleGroundContact::ImpactHardness01(800.0, P) - 1.0) < Eps, "hardness 1 at crash");
        CHECK(Absd(FVehicleGroundContact::ImpactHardness01(5000.0, P) - 1.0) < Eps, "hardness clamps at 1");
    }

    // ---- No floor below -> airborne, never clamps ------------------------------
    {
        auto R = FVehicleGroundContact::Evaluate(5000.0, /*hit*/ false, 0.0, -1000.0, EContact::Airborne, P);
        CHECK(R.Contact == EContact::Airborne, "no hit -> airborne");
        CHECK(!R.bClampToGround, "no hit -> no clamp");
        CHECK(R.Touchdown == ETouchdown::None, "no hit -> no touchdown");
    }

    // ---- High above ground, descending fast: still flying ----------------------
    {
        auto R = FVehicleGroundContact::Evaluate(5000.0, true, 0.0, -700.0, EContact::Airborne, P);
        CHECK(R.Contact == EContact::Airborne, "above rest height stays airborne");
        CHECK(!R.bClampToGround, "airborne -> no clamp");
        CHECK(R.Touchdown == ETouchdown::None, "airborne -> no touchdown");
    }

    // ---- Soft / Hard / Crash touchdown on the airborne->grounded edge ----------
    {
        // Ground Z 0, gear 150 -> rest height 150. Reaching it from airborne is a
        // touchdown; the descent speed sets the quality.
        auto Soft = FVehicleGroundContact::Evaluate(150.0, true, 0.0, -100.0, EContact::Airborne, P);
        CHECK(Soft.Contact == EContact::Grounded, "reach rest -> grounded");
        CHECK(Soft.Touchdown == ETouchdown::Soft, "slow descent -> Soft");
        CHECK(Soft.bClampToGround && Absd(Soft.ClampZCm - 150.0) < Eps, "soft clamps to rest Z");
        CHECK(Soft.ImpactHardness01 < Eps, "soft hardness 0");

        auto Hard = FVehicleGroundContact::Evaluate(150.0, true, 0.0, -550.0, EContact::Airborne, P);
        CHECK(Hard.Touchdown == ETouchdown::Hard, "mid descent -> Hard");
        CHECK(Hard.ImpactHardness01 > 0.0 && Hard.ImpactHardness01 < 1.0, "hard hardness in (0,1)");

        auto Crash = FVehicleGroundContact::Evaluate(150.0, true, 0.0, -1200.0, EContact::Airborne, P);
        CHECK(Crash.Touchdown == ETouchdown::Crash, "fast descent -> Crash");
        CHECK(Absd(Crash.ImpactHardness01 - 1.0) < Eps, "crash hardness 1");
    }

    // ---- Spawned below ground: lifted to rest as a zero-speed Soft set-down -----
    {
        auto R = FVehicleGroundContact::Evaluate(-500.0, true, 0.0, 0.0, EContact::Airborne, P);
        CHECK(R.Contact == EContact::Grounded, "below ground -> grounded");
        CHECK(R.bClampToGround && Absd(R.ClampZCm - 150.0) < Eps, "below ground clamps UP to rest Z");
        CHECK(R.Touchdown == ETouchdown::Soft && R.ImpactHardness01 < Eps, "zero-speed contact is soft");
    }

    // ---- Grounded resting: held on the floor, no fresh touchdown ----------------
    {
        auto R = FVehicleGroundContact::Evaluate(151.0, true, 0.0, 0.0, EContact::Grounded, P);
        CHECK(R.Contact == EContact::Grounded, "settled stays grounded");
        CHECK(R.bClampToGround && Absd(R.ClampZCm - 150.0) < Eps, "settled holds rest Z");
        CHECK(R.Touchdown == ETouchdown::None, "settled is not a fresh touchdown");
    }

    // ---- Hysteresis: within the liftoff margin stays grounded -------------------
    {
        // rest 150, margin 25 -> still grounded at 170, airborne past 175.
        auto Within = FVehicleGroundContact::Evaluate(170.0, true, 0.0, 50.0, EContact::Grounded, P);
        CHECK(Within.Contact == EContact::Grounded, "within margin stays grounded");
        CHECK(Within.bClampToGround, "within margin still clamps");

        auto Lifted = FVehicleGroundContact::Evaluate(400.0, true, 0.0, 200.0, EContact::Grounded, P);
        CHECK(Lifted.Contact == EContact::Airborne, "clear of margin -> airborne");
        CHECK(!Lifted.bClampToGround, "lifted -> no clamp");
        CHECK(Lifted.Touchdown == ETouchdown::None, "takeoff is not a touchdown");
    }

    // ---- Non-zero ground Z: everything is relative to the traced floor ----------
    {
        // ground 1000 -> rest 1150. Body at 1150 from airborne is a touchdown.
        auto R = FVehicleGroundContact::Evaluate(1150.0, true, 1000.0, -100.0, EContact::Airborne, P);
        CHECK(R.Contact == EContact::Grounded, "touchdown relative to raised floor");
        CHECK(Absd(R.ClampZCm - 1150.0) < Eps, "clamp Z tracks the floor");
    }

    // ---- Degenerate params don't invert the band -------------------------------
    {
        FVehicleGroundContact::FParams D;
        D.SoftLandingSpeedCm = 400.0;
        D.CrashLandingSpeedCm = 100.0; // crash < soft: clamped up to soft internally
        CHECK(FVehicleGroundContact::ClassifyTouchdown(401.0, D) == ETouchdown::Crash, "past soft is crash when band collapses");
        CHECK(Absd(FVehicleGroundContact::ImpactHardness01(401.0, D) - 1.0) < Eps, "collapsed band -> full hardness past soft");
        CHECK(FVehicleGroundContact::ClassifyTouchdown(399.0, D) == ETouchdown::Soft, "below soft still soft");
    }

    if (g_fail == 0) { std::printf("ground_contact_verify: ALL PASS\n"); return 0; }
    std::printf("ground_contact_verify: %d FAILED\n", g_fail);
    return 1;
}
