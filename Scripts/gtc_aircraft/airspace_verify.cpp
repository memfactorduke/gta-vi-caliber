// Headless verifier for FAircraftAirspace. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.Vehicles.Aircraft.Airspace invariants with a host clang.
#include <cstdio>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/Vehicles/Aircraft/Airspace.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    const double Eps = 1e-9;
    const FAircraftAirspace::FParams P; // soft 200000, hard 250000

    // ---- Ceiling authority ramp ------------------------------------------------
    {
        CHECK(Absd(FAircraftAirspace::CeilingAuthority(0.0, P) - 1.0) < Eps, "full authority at sea level");
        CHECK(Absd(FAircraftAirspace::CeilingAuthority(200000.0, P) - 1.0) < Eps, "full at soft ceiling");
        CHECK(Absd(FAircraftAirspace::CeilingAuthority(225000.0, P) - 0.5) < Eps, "half-way through band -> 0.5");
        CHECK(FAircraftAirspace::CeilingAuthority(250000.0, P) < Eps, "zero at hard ceiling");
        CHECK(FAircraftAirspace::CeilingAuthority(300000.0, P) < Eps, "zero above hard ceiling");
    }

    // ---- ClampClimb only restricts ascent --------------------------------------
    {
        CHECK(Absd(FAircraftAirspace::ClampClimb(0.0, 500.0, P) - 500.0) < Eps, "low altitude: full climb");
        CHECK(Absd(FAircraftAirspace::ClampClimb(225000.0, 500.0, P) - 250.0) < Eps, "mid band: climb halved");
        CHECK(Absd(FAircraftAirspace::ClampClimb(250000.0, 500.0, P)) < Eps, "at ceiling: no climb");
        // Descent is untouched at any altitude.
        CHECK(Absd(FAircraftAirspace::ClampClimb(250000.0, -500.0, P) - (-500.0)) < Eps, "can always descend from ceiling");
        CHECK(Absd(FAircraftAirspace::ClampClimb(0.0, -300.0, P) - (-300.0)) < Eps, "descent never scaled");
        CHECK(Absd(FAircraftAirspace::ClampClimb(225000.0, 0.0, P)) < Eps, "level stays level");
    }

    // ---- Degenerate band: hard step --------------------------------------------
    {
        FAircraftAirspace::FParams D;
        D.CeilingZCm = 100000.0;
        D.SoftCeilingZCm = 100000.0; // soft == hard
        CHECK(Absd(FAircraftAirspace::CeilingAuthority(99999.0, D) - 1.0) < Eps, "below collapsed ceiling -> 1");
        CHECK(FAircraftAirspace::CeilingAuthority(100000.0, D) < Eps, "at collapsed ceiling -> 0");

        FAircraftAirspace::FParams E;
        E.CeilingZCm = 100000.0;
        E.SoftCeilingZCm = 150000.0; // soft above hard: clamped down to hard internally
        CHECK(Absd(FAircraftAirspace::CeilingAuthority(99999.0, E) - 1.0) < Eps, "inverted band still sane below ceiling");
        CHECK(FAircraftAirspace::CeilingAuthority(100001.0, E) < Eps, "inverted band zero above ceiling");
    }

    // ---- Restricted floor ------------------------------------------------------
    {
        CHECK(Absd(FAircraftAirspace::RestrictedFloorZ(1000.0, 5000.0) - 6000.0) < Eps, "floor = ground + min alt");
        CHECK(Absd(FAircraftAirspace::RestrictedFloorZ(1000.0, -50.0) - 1000.0) < Eps, "negative min alt -> ground");
    }

    if (g_fail == 0) { std::printf("airspace_verify: ALL PASS\n"); return 0; }
    std::printf("airspace_verify: %d FAILED\n", g_fail);
    return 1;
}
