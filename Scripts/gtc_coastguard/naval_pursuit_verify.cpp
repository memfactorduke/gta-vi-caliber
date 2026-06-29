// Headless verifier for FNavalPursuit. Compiles the REAL core .cpp (and the reused
// FPursuitTactics.cpp it delegates to) against the shim, checking the UE<->Godot remap
// and the naval-specific moves. Mirrors GTC.AI.Pursuit.NavalPursuit.
#include <cstdio>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/AI/Pursuit/PursuitTactics.cpp"
#include "../../Source/GTC_UE5/AI/Pursuit/NavalPursuit.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    const double Eps = 1e-6;

    CHECK(FNavalPursuit::DeployStars == 4, "deploy star matches coast guard escalation");

    // ---- Intercept: leads on the water plane, stays at Z=0 (remap correct) -----
    {
        const FVector Tp(0, 0, 0), Tv(100, 0, 0), Pp(0, -1000, 0);
        const FVector R = FNavalPursuit::InterceptPoint(Tp, Tv, Pp, 200.0);
        CHECK(Absd(R.Z) < Eps, "intercept stays on the water plane (Z=0)");
        CHECK(R.X > 0.0, "intercept leads the target's +X travel");
    }

    // ---- Stationary target -> returns its position (remap round-trips exactly) -
    {
        const FVector Tp(5, 6, 7);
        const FVector R = FNavalPursuit::InterceptPoint(Tp, FVector::ZeroVector, FVector(0, 0, 0), 100.0);
        CHECK(Absd(R.X - 5) < Eps && Absd(R.Y - 6) < Eps && Absd(R.Z - 7) < Eps, "stationary target -> its pos (remap identity)");
    }

    // ---- Broadside offset: abeam the target on the water ----------------------
    {
        const FVector Tp(0, 0, 0), Tv(100, 0, 0);
        const FVector Star = FNavalPursuit::BroadsideOffset(Tp, Tv, +1.0, 500.0);
        CHECK(Absd(Star.X) < Eps && Absd(Star.Y + 500.0) < Eps && Absd(Star.Z) < Eps, "starboard run is -Y of +X travel");
        const FVector Port = FNavalPursuit::BroadsideOffset(Tp, Tv, -1.0, 500.0);
        CHECK(Absd(Port.Y - 500.0) < Eps, "port run is +Y");
        const FVector NoHeading = FNavalPursuit::BroadsideOffset(Tp, FVector::ZeroVector, +1.0, 500.0);
        CHECK(Absd(NoHeading.Y - 500.0) < Eps, "no heading -> step out along +Y");
    }

    // ---- Clamp to water: ride the surface, ground on the shore ----------------
    {
        const FVector Deep = FNavalPursuit::ClampToWater(FVector(10, 20, 999), -50.0, 0.0);
        CHECK(Absd(Deep.Z) < Eps && Absd(Deep.X - 10) < Eps && Absd(Deep.Y - 20) < Eps, "deep water -> rides sea level");
        const FVector Beach = FNavalPursuit::ClampToWater(FVector(10, 20, 999), 30.0, 0.0);
        CHECK(Absd(Beach.Z - 30.0) < Eps, "shore above water -> grounds on the shore");
    }

    // ---- Ram decision: close + aligned + heat (delegates through the remap) ----
    {
        const FVector Pp(0, 0, 0), Heading(100, 0, 0), Near(200, 0, 0), Far(5000, 0, 0);
        CHECK(FNavalPursuit::ShouldRamHull(Pp, Heading, Near, 500.0, 4), "close + aligned + 4 stars -> ram");
        CHECK(!FNavalPursuit::ShouldRamHull(Pp, Heading, Near, 500.0, 2), "low heat -> no ram");
        CHECK(!FNavalPursuit::ShouldRamHull(Pp, Heading, Far, 500.0, 4), "out of range -> no ram");
    }

    if (g_fail == 0) { std::printf("naval_pursuit_verify: ALL PASS\n"); return 0; }
    std::printf("naval_pursuit_verify: %d FAILED\n", g_fail);
    return 1;
}
