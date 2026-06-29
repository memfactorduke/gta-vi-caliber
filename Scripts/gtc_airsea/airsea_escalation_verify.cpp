// Headless verifier for FAirSeaEscalation. Compiles the REAL core .cpp against the shim
// and re-asserts the GTC.AI.PoliceDispatch.AirSeaEscalation invariants with a host clang.
#include <cstdio>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/AI/PoliceDispatch/AirSeaEscalation.cpp"

using M = EPlayerMedium;

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)

int main()
{
    // ---- ClampStars -----------------------------------------------------------
    CHECK(FAirSeaEscalation::ClampStars(-5) == 0, "clamp below 0");
    CHECK(FAirSeaEscalation::ClampStars(99) == 6, "clamp above max");
    CHECK(FAirSeaEscalation::ClampStars(3) == 3, "in range passes through");

    // ---- Police air deploy: 3 on land/sea, 2 in the air -----------------------
    CHECK(!FAirSeaEscalation::ShouldDeployPoliceAir(2, M::Land), "land 2 stars -> no chopper");
    CHECK(FAirSeaEscalation::ShouldDeployPoliceAir(3, M::Land), "land 3 stars -> chopper");
    CHECK(FAirSeaEscalation::ShouldDeployPoliceAir(3, M::Sea), "sea 3 stars -> chopper");
    CHECK(FAirSeaEscalation::ShouldDeployPoliceAir(2, M::Air), "air 2 stars -> chopper (scramble early)");
    CHECK(!FAirSeaEscalation::ShouldDeployPoliceAir(1, M::Air), "air 1 star -> no chopper (never at <1 threshold)");

    // ---- Coast Guard deploy: sea only, 4+ -------------------------------------
    CHECK(!FAirSeaEscalation::ShouldDeployCoastGuard(3, M::Sea), "sea 3 -> no coast guard");
    CHECK(FAirSeaEscalation::ShouldDeployCoastGuard(4, M::Sea), "sea 4 -> coast guard");
    CHECK(!FAirSeaEscalation::ShouldDeployCoastGuard(6, M::Land), "land 6 -> no coast guard");
    CHECK(!FAirSeaEscalation::ShouldDeployCoastGuard(6, M::Air), "air 6 -> no coast guard");

    // ---- Counts ---------------------------------------------------------------
    CHECK(FAirSeaEscalation::PoliceAirCount(2, M::Land) == 0, "no chopper -> count 0");
    CHECK(FAirSeaEscalation::PoliceAirCount(3, M::Land) == 1, "3 stars -> 1 chopper");
    CHECK(FAirSeaEscalation::PoliceAirCount(4, M::Land) == 1, "4 stars -> 1 chopper");
    CHECK(FAirSeaEscalation::PoliceAirCount(5, M::Land) == 2, "5 stars -> 2 choppers");
    CHECK(FAirSeaEscalation::PoliceAirCount(6, M::Land) == 2, "6 stars -> 2 choppers");
    CHECK(FAirSeaEscalation::PoliceAirCount(2, M::Air) == 1, "air 2 stars -> 1 chopper");

    CHECK(FAirSeaEscalation::CoastGuardBoatCount(3, M::Sea) == 0, "sea 3 -> 0 boats");
    CHECK(FAirSeaEscalation::CoastGuardBoatCount(4, M::Sea) == 1, "sea 4 -> 1 boat");
    CHECK(FAirSeaEscalation::CoastGuardBoatCount(5, M::Sea) == 2, "sea 5 -> 2 boats");
    CHECK(FAirSeaEscalation::CoastGuardBoatCount(6, M::Sea) == 3, "sea 6 -> 3 boats (capped)");
    CHECK(FAirSeaEscalation::CoastGuardBoatCount(6, M::Land) == 0, "land 6 -> 0 boats");

    if (g_fail == 0) { std::printf("airsea_escalation_verify: ALL PASS\n"); return 0; }
    std::printf("airsea_escalation_verify: %d FAILED\n", g_fail);
    return 1;
}
