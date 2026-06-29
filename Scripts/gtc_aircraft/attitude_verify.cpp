// Headless verifier for FVehicleAttitude. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.Vehicles.Attitude invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/Vehicles/Attitude/VehicleAttitude.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    const double Eps = 1e-6;

    // ---- Bank into a turn ------------------------------------------------------
    {
        CHECK(Absd(FVehicleAttitude::BankAngle(0.0, 3000.0, 1.0, 0.6)) < Eps, "wings level -> no bank");
        CHECK(Absd(FVehicleAttitude::BankAngle(0.5, 0.0, 1.0, 0.6)) < Eps, "standstill -> no bank");

        const double Expect = std::atan2(1000.0 * 0.1, 980.0); // ~0.1017
        CHECK(Absd(FVehicleAttitude::BankAngle(0.1, 1000.0, 1.0, 1.5) - Expect) < Eps, "gentle bank matches coordinated turn");
        CHECK(FVehicleAttitude::BankAngle(-0.1, 1000.0, 1.0, 1.5) < 0.0, "opposite yaw -> opposite bank");
        CHECK(Absd(FVehicleAttitude::BankAngle(0.1, 1000.0, 2.0, 1.5) - 2.0 * Expect) < Eps, "gain scales the bank");
        CHECK(Absd(FVehicleAttitude::BankAngle(0.5, 3000.0, 1.0, 0.6) - 0.6) < Eps, "hard turn clamps to max roll");
    }

    // ---- Pitch follows the flight path -----------------------------------------
    {
        const double Climb = std::atan2(500.0, 1000.0); // ~0.4636
        CHECK(Absd(FVehicleAttitude::PitchFromVelocity(500.0, 1000.0, 0.6) - Climb) < Eps, "climbing -> nose up");
        CHECK(Absd(FVehicleAttitude::PitchFromVelocity(0.0, 1000.0, 0.6)) < Eps, "level -> no pitch");
        CHECK(Absd(FVehicleAttitude::PitchFromVelocity(-500.0, 1000.0, 0.6) - (-Climb)) < Eps, "diving -> nose down");
        CHECK(Absd(FVehicleAttitude::PitchFromVelocity(10000.0, 1.0, 0.5) - 0.5) < Eps, "near-vertical climb clamps to max pitch");
        CHECK(Absd(FVehicleAttitude::PitchFromVelocity(500.0, -1000.0, 0.6) - 0.6) < Eps, "negative horiz speed treated as 0 (clamps)");
    }

    if (g_fail == 0) { std::printf("attitude_verify: ALL PASS\n"); return 0; }
    std::printf("attitude_verify: %d FAILED\n", g_fail);
    return 1;
}
