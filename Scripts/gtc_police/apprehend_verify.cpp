// Headless verifier for FApprehend (the arrest "close to cuff" decision).
// Compiles the REAL header against the host-clang shim and re-asserts the same
// invariants the GTC.AI.Combat.Apprehend automation test checks.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/AI/Combat/Apprehend.h"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)

int main()
{
    const double Eps = 1e-4;

    // ShouldApprehend
    CHECK(FApprehend::ShouldApprehend(1, false, 0.0), "1 star unarmed calm -> apprehend");
    CHECK(FApprehend::ShouldApprehend(2, false, 0.2), "2 stars unarmed calm -> apprehend");
    CHECK(!FApprehend::ShouldApprehend(0, false, 0.0), "0 stars -> no apprehend");
    CHECK(!FApprehend::ShouldApprehend(3, false, 0.0), "3 stars -> gunfight");
    CHECK(!FApprehend::ShouldApprehend(1, true, 0.0), "armed -> no apprehend");
    CHECK(!FApprehend::ShouldApprehend(1, false, 0.9), "high threat -> no apprehend");
    CHECK(!FApprehend::ShouldApprehend(1, false, FApprehend::ThreatCeiling), "threat at ceiling -> no");

    // InCatchRange
    CHECK(FApprehend::InCatchRange(1.5, 2.0), "inside catch range");
    CHECK(FApprehend::InCatchRange(2.0, 2.0), "at catch range");
    CHECK(!FApprehend::InCatchRange(2.5, 2.0), "outside catch range");

    // ApproachHeading
    {
        const FVector H = FApprehend::ApproachHeading(FVector(0, 0, 0), FVector(10, 5, 20));
        CHECK(std::fabs(H.Y) < Eps, "heading planar (Y==0)");
        CHECK(std::fabs(H.Size() - 1.0) < Eps, "heading unit length");
        CHECK(FApprehend::ApproachHeading(FVector(0, 0, 0), FVector(0, 99, 0)).IsNearlyZero(), "coincident -> zero");
    }

    // ApproachSpeed
    CHECK(std::fabs(FApprehend::ApproachSpeed(460.0, 10.0, 2.0) - 460.0) < Eps, "full run while closing");
    CHECK(std::fabs(FApprehend::ApproachSpeed(460.0, 1.0, 2.0) - 0.0) < Eps, "plant in catch range");

    if (g_fail != 0)
    {
        std::printf("=== apprehend_verify FAILED (%d) ===\n", g_fail);
        return 1;
    }
    std::printf("=== apprehend_verify PASSED ===\n");
    return 0;
}
