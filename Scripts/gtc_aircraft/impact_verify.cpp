// Headless verifier for FVehicleImpact. Compiles the REAL core .cpp against the shim.
#include <cstdio>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/Vehicles/Contact/VehicleImpact.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    const double Eps = 1e-9;

    // ---- Damage curve: free below safe, quadratic above ------------------------
    CHECK(FVehicleImpact::DamageForSpeed(400.0, 500.0, 1e-4) < Eps, "below safe -> no damage");
    CHECK(FVehicleImpact::DamageForSpeed(500.0, 500.0, 1e-4) < Eps, "at safe -> no damage");
    CHECK(Absd(FVehicleImpact::DamageForSpeed(1500.0, 500.0, 1e-4) - 100.0) < Eps, "quadratic past safe (1000^2 * 1e-4)");
    CHECK(Absd(FVehicleImpact::DamageForSpeed(2500.0, 500.0, 1e-4) - 400.0) < Eps, "doubles overspeed -> 4x damage");
    CHECK(FVehicleImpact::DamageForSpeed(9000.0, 500.0, 0.0) < Eps, "zero scale -> no damage");
    CHECK(FVehicleImpact::DamageForSpeed(-100.0, 500.0, 1e-4) < Eps, "negative speed -> no damage");

    // monotonic non-decreasing in speed past safe
    {
        bool mono = true; double prev = -1.0;
        for (int i = 0; i <= 30; ++i)
        {
            const double d = FVehicleImpact::DamageForSpeed(500.0 + i * 100.0, 500.0, 1e-4);
            if (d < prev - Eps) mono = false;
            prev = d;
        }
        CHECK(mono, "damage rises monotonically with speed");
    }

    // ---- Restitution: glance keeps most, head-on stops -------------------------
    CHECK(Absd(FVehicleImpact::Restitution(800.0, 0.0) - 1.0) < Eps, "perpendicular glance keeps all (slide)");
    CHECK(FVehicleImpact::Restitution(800.0, 1.0) < Eps, "head-on hit dead-stops");
    CHECK(Absd(FVehicleImpact::Restitution(800.0, 0.25) - 0.75) < Eps, "partial graze keeps 0.75");
    CHECK(FVehicleImpact::Restitution(0.0, 0.0) < Eps, "no into-surface speed -> nothing retained");
    CHECK(FVehicleImpact::Restitution(-50.0, 0.0) < Eps, "moving away -> nothing retained");
    CHECK(Absd(FVehicleImpact::Restitution(800.0, 5.0) - 0.0) < Eps, "over-range dot clamps to head-on");

    if (g_fail == 0) { std::printf("impact_verify: ALL PASS\n"); return 0; }
    std::printf("impact_verify: %d FAILED\n", g_fail);
    return 1;
}
