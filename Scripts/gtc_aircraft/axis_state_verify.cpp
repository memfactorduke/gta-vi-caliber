// Headless verifier for FVehicleAxisState. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.Vehicles.AxisState invariants with a host clang.
#include <cstdio>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/Vehicles/Drive/VehicleAxisState.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    const double Eps = 1e-9;

    // ---- Raise / lower a held setting -----------------------------------------
    CHECK(Absd(FVehicleAxisState::Integrate(0.5, 1.0, 0.4, 1.0) - 0.9) < Eps, "raise by rate*dt");
    CHECK(Absd(FVehicleAxisState::Integrate(0.5, -1.0, 0.4, 1.0) - 0.1) < Eps, "lower by rate*dt");

    // ---- Clamp to the [0,1] default range --------------------------------------
    CHECK(Absd(FVehicleAxisState::Integrate(0.9, 1.0, 0.4, 1.0) - 1.0) < Eps, "clamps at top");
    CHECK(Absd(FVehicleAxisState::Integrate(0.1, -1.0, 0.4, 1.0) - 0.0) < Eps, "clamps at bottom");

    // ---- Input direction clamps to [-1,1] --------------------------------------
    CHECK(Absd(FVehicleAxisState::Integrate(0.0, 5.0, 0.4, 1.0) - 0.4) < Eps, "over-range input clamps to full up");
    CHECK(Absd(FVehicleAxisState::Integrate(0.4, -5.0, 0.4, 1.0) - 0.0) < Eps, "over-range input clamps to full down");

    // ---- Non-positive dt holds (but still range-clamps) ------------------------
    CHECK(Absd(FVehicleAxisState::Integrate(0.37, 1.0, 0.4, 0.0) - 0.37) < Eps, "zero dt holds");
    CHECK(Absd(FVehicleAxisState::Integrate(0.37, 1.0, 0.4, -2.0) - 0.37) < Eps, "negative dt holds");
    CHECK(Absd(FVehicleAxisState::Integrate(5.0, 0.0, 0.0, 0.0) - 1.0) < Eps, "out-of-range current clamps even at dt 0");

    // ---- Custom and inverted ranges --------------------------------------------
    CHECK(Absd(FVehicleAxisState::Integrate(0.0, 1.0, 1.0, 1.0, -1.0, 1.0) - 1.0) < Eps, "custom hi");
    CHECK(Absd(FVehicleAxisState::Integrate(0.0, -1.0, 5.0, 1.0, -1.0, 1.0) - (-1.0)) < Eps, "custom lo");
    CHECK(Absd(FVehicleAxisState::Integrate(0.0, 1.0, 0.4, 1.0, 1.0, 0.0) - 0.4) < Eps, "inverted Lo/Hi treated as range");

    if (g_fail == 0) { std::printf("axis_state_verify: ALL PASS\n"); return 0; }
    std::printf("axis_state_verify: %d FAILED\n", g_fail);
    return 1;
}
