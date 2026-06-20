// Headless verifier for FSuppression — compiles the REAL header against the host
// shim and re-asserts the GTC.AI.Combat.Suppression invariants.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/AI/Combat/Suppression.h"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static bool Near(double a, double b) { return std::fabs(a - b) < 1e-4; }

int main()
{
    CHECK(FSuppression::FromHit(0.0) > 0.0, "graze adds some");
    CHECK(FSuppression::FromHit(1.0) > FSuppression::FromHit(0.0), "harder hit suppresses more");
    CHECK(FSuppression::FromHit(5.0) <= 1.0 + 1e-4, "from-hit clamped");

    CHECK(Near(FSuppression::Add(0.2, 0.3), 0.5), "add accumulates");
    CHECK(Near(FSuppression::Add(0.9, 0.5), 1.0), "add clamps at 1");

    CHECK(Near(FSuppression::Decay(1.0, 1.0, 0.35), 0.65), "decay partial");
    CHECK(Near(FSuppression::Decay(0.1, 1.0, 0.35), 0.0), "decay floors at 0");

    CHECK(FSuppression::IsSuppressed(0.5), "at threshold suppressed");
    CHECK(!FSuppression::IsSuppressed(0.49), "below threshold not suppressed");

    CHECK(Near(FSuppression::FireRateMult(0.0), 1.0), "no suppression 1x");
    CHECK(Near(FSuppression::FireRateMult(1.0), 2.2), "full suppression 2.2x");

    CHECK(Near(FSuppression::EffectiveHealthFrac(0.8, 0.0), 0.8), "no suppression keeps health");
    CHECK(Near(FSuppression::EffectiveHealthFrac(0.8, 1.0), 0.4), "full suppression halves health");

    if (g_fail != 0) { std::printf("=== suppression_verify FAILED (%d) ===\n", g_fail); return 1; }
    std::printf("=== suppression_verify PASSED ===\n");
    return 0;
}
