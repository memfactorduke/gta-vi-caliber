// Headless verifier for FWatercraftFloat. Compiles the REAL core .cpp against the shim
// and re-asserts the GTC.Vehicles.Watercraft.Float invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/Vehicles/Watercraft/WatercraftFloat.cpp"

using FHullSample = FWatercraftFloat::FHullSample;

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

// A square hull: 4 samples at (+/-100, +/-100).
static const FHullSample kHull[4] = { {100,100}, {100,-100}, {-100,100}, {-100,-100} };

int main()
{
    const double Eps = 1e-6;
    const FWatercraftFloat::FParams P; // draft 40, capsize 1.2, roughRef 200

    // ---- Flat water: level, floats at draft -----------------------------------
    {
        const double H[4] = { 50, 50, 50, 50 };
        auto Pose = FWatercraftFloat::ResolvePose(H, kHull, 4, P);
        CHECK(Absd(Pose.PitchRad) < Eps && Absd(Pose.RollRad) < Eps, "flat water -> level");
        CHECK(Absd(Pose.Zcm - 10.0) < Eps, "floats at surface(50) - draft(40) = 10");
        CHECK(Pose.Roughness01 < Eps, "flat water -> zero roughness");
        CHECK(!Pose.bCapsized, "flat water -> not capsized");
    }

    // ---- Swell rising toward the bow -> nose up -------------------------------
    {
        const double H[4] = { 60, 60, 40, 40 }; // higher at +x (bow)
        auto Pose = FWatercraftFloat::ResolvePose(H, kHull, 4, P);
        CHECK(Absd(Pose.PitchRad - std::atan(0.1)) < Eps, "bow swell -> nose up by atan(slope)");
        CHECK(Absd(Pose.RollRad) < Eps, "symmetric in Y -> no roll");
        CHECK(Absd(Pose.Zcm - 10.0) < Eps, "centre height still 50 -> Z 10");
        CHECK(Absd(Pose.Roughness01 - 0.1) < Eps, "spread 20 / ref 200 = 0.1");
    }

    // ---- Swell rising to starboard -> roll right ------------------------------
    {
        const double H[4] = { 60, 40, 60, 40 }; // higher at +y (starboard)
        auto Pose = FWatercraftFloat::ResolvePose(H, kHull, 4, P);
        CHECK(Absd(Pose.RollRad - std::atan(0.1)) < Eps, "starboard swell -> roll right");
        CHECK(Absd(Pose.PitchRad) < Eps, "symmetric in X -> no pitch");
    }

    // ---- Steep wave face -> capsize -------------------------------------------
    {
        const double H[4] = { 410, 410, -310, -310 }; // slope 3.6 along x
        auto Pose = FWatercraftFloat::ResolvePose(H, kHull, 4, P);
        CHECK(Absd(Pose.PitchRad - std::atan(3.6)) < Eps, "steep face -> big pitch");
        CHECK(Pose.bCapsized, "tilt past 1.2 rad -> capsized");
        CHECK(Absd(Pose.Roughness01 - 1.0) < Eps, "huge spread clamps roughness to 1");
    }

    // ---- No samples: level at -draft ------------------------------------------
    {
        auto Pose = FWatercraftFloat::ResolvePose(nullptr, nullptr, 0, P);
        CHECK(Absd(Pose.Zcm - (-40.0)) < Eps, "no samples -> sea level minus draft");
        CHECK(!Pose.bCapsized && Pose.Roughness01 < Eps, "no samples -> level, calm");
    }

    // ---- Degenerate (single sample) -> level fallback at that height ----------
    {
        const FHullSample One[1] = { {10, 20} };
        const double H[1] = { 70 };
        auto Pose = FWatercraftFloat::ResolvePose(H, One, 1, P);
        CHECK(Absd(Pose.PitchRad) < Eps && Absd(Pose.RollRad) < Eps, "single sample -> level");
        CHECK(Absd(Pose.Zcm - 30.0) < Eps, "single sample -> height(70) - draft(40)");
    }

    // ---- Collinear samples (no Y span) -> level fallback ----------------------
    {
        const FHullSample Line[3] = { {-100, 0}, {0, 0}, {100, 0} };
        const double H[3] = { 40, 50, 60 };
        auto Pose = FWatercraftFloat::ResolvePose(H, Line, 3, P);
        CHECK(Absd(Pose.PitchRad) < Eps && Absd(Pose.RollRad) < Eps, "collinear -> level fallback");
        CHECK(Absd(Pose.Zcm - 10.0) < Eps, "collinear -> mean(50) - draft(40)");
    }

    if (g_fail == 0) { std::printf("watercraft_float_verify: ALL PASS\n"); return 0; }
    std::printf("watercraft_float_verify: %d FAILED\n", g_fail);
    return 1;
}
