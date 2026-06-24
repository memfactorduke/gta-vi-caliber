// Headless verifier for the radial-menu pure-core (GtcRadial). Compiles the REAL
// shipping core (Source/GTC_UE5/UI/Radial/RadialMenu.cpp) against the host-clang
// shim and re-asserts the same geometry/selection invariants the
// GTC.UI.Radial.RadialMenu automation test checks — so the actual wheel math is
// exercised with a plain compiler, the hook-safe way (the in-editor UE automation
// runner is intentionally NOT launched).
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/UI/Radial/RadialMenu.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)

static bool Near(double A, double B, double Eps = 1e-6) { return std::fabs(A - B) <= Eps; }

int main()
{
    using namespace GtcRadial;
    const double Eps = 1e-6;

    // 1. Slice span tiles the circle; single/empty wheels stay one full ring.
    CHECK(Near(SliceSpan(4), UE_DOUBLE_TWO_PI / 4.0), "4 slices = quarter turn");
    CHECK(Near(SliceSpan(8), UE_DOUBLE_TWO_PI / 8.0), "8 slices = eighth turn");
    CHECK(Near(SliceSpan(1), UE_DOUBLE_TWO_PI), "1-item wheel = full ring");
    CHECK(Near(SliceSpan(0), UE_DOUBLE_TWO_PI), "0-item wheel = full ring (no div0)");

    // 2. Slice 0 at top; clockwise; index wraps.
    CHECK(Near(SliceCenterAngle(0, 4), 0.0), "slice 0 at north");
    CHECK(Near(SliceCenterAngle(1, 4), UE_DOUBLE_PI / 2.0), "slice 1/4 east");
    CHECK(Near(SliceCenterAngle(2, 4), UE_DOUBLE_PI), "slice 2/4 south");
    CHECK(Near(SliceCenterAngle(3, 4), 3.0 * UE_DOUBLE_PI / 2.0), "slice 3/4 west");
    CHECK(Near(SliceCenterAngle(-1, 4), SliceCenterAngle(3, 4)), "index -1 wraps to last");
    CHECK(Near(SliceCenterAngle(4, 4), SliceCenterAngle(0, 4)), "index Count wraps to 0");

    // 3. UnitDirection points where expected (X right, Y down).
    {
        const FVector2D N = UnitDirection(0.0);
        CHECK(Near(N.X, 0.0, Eps) && N.Y < -0.99, "north points up");
        const FVector2D E = UnitDirection(UE_DOUBLE_PI / 2.0);
        CHECK(E.X > 0.99 && Near(E.Y, 0.0, Eps), "east points right");
        const FVector2D S = UnitDirection(UE_DOUBLE_PI);
        CHECK(Near(S.X, 0.0, Eps) && S.Y > 0.99, "south points down");
        const FVector2D W = UnitDirection(3.0 * UE_DOUBLE_PI / 2.0);
        CHECK(W.X < -0.99 && Near(W.Y, 0.0, Eps), "west points left");
    }

    // 4. AngleOf inverts UnitDirection (and a zero vector is 0, not NaN).
    CHECK(Near(AngleOf(FVector2D(0.0, -1.0)), 0.0, Eps), "up -> 0");
    CHECK(Near(AngleOf(FVector2D(1.0, 0.0)), UE_DOUBLE_PI / 2.0, Eps), "right -> PI/2");
    CHECK(Near(AngleOf(FVector2D(0.0, 1.0)), UE_DOUBLE_PI, Eps), "down -> PI");
    CHECK(Near(AngleOf(FVector2D(-1.0, 0.0)), 3.0 * UE_DOUBLE_PI / 2.0, Eps), "left -> 3PI/2");
    CHECK(AngleOf(FVector2D::ZeroVector) == 0.0, "zero vector -> 0");

    // 5. Layout <-> hit-test round-trip: a label drawn at slice i is selected at i.
    for (int32 Count = 1; Count <= 12; ++Count)
    {
        for (int32 i = 0; i < Count; ++i)
        {
            const FVector2D At = UnitDirection(SliceCenterAngle(i, Count)) * 100.0;
            if (SelectionAt(At, Count, 10.0) != i)
            {
                std::printf("  FAIL: roundtrip Count=%d slice=%d\n", Count, i);
                ++g_fail;
            }
        }
    }

    // 6. Cardinal pointer offsets on a 4-wheel.
    const double Far = 80.0;
    CHECK(SelectionAt(FVector2D(0.0, -Far), 4, 20.0) == 0, "up -> 0");
    CHECK(SelectionAt(FVector2D(Far, 0.0), 4, 20.0) == 1, "right -> 1");
    CHECK(SelectionAt(FVector2D(0.0, Far), 4, 20.0) == 2, "down -> 2");
    CHECK(SelectionAt(FVector2D(-Far, 0.0), 4, 20.0) == 3, "left -> 3");

    // 7. Wrap-around seam at the top.
    {
        const int32 Count = 6;
        const double Span = SliceSpan(Count);
        CHECK(IndexAtAngle(Span * 0.4, Count) == 0, "touch CW of north -> 0");
        CHECK(IndexAtAngle(UE_DOUBLE_TWO_PI - Span * 0.4, Count) == 0, "touch CCW of north -> 0");
        CHECK(IndexAtAngle(Span * 0.6, Count) == 1, "past CW half-boundary -> 1");
        CHECK(IndexAtAngle(UE_DOUBLE_TWO_PI - Span * 0.6, Count) == Count - 1, "past CCW half-boundary -> last");
    }

    // 8. Dead zone neutralises the center hub (strict <).
    CHECK(SelectionAt(FVector2D(5.0, 0.0), 4, 20.0) == INDEX_NONE, "inside dead zone -> none");
    CHECK(SelectionAt(FVector2D(20.0, 0.0), 4, 20.0) == 1, "on dead-zone radius -> selects");
    CHECK(SelectionAt(FVector2D(0.5, 0.0), 4, 0.0) == 1, "zero dead zone -> selects");

    // 9. Degenerate counts stay safe.
    CHECK(SelectionAt(FVector2D(50.0, 0.0), 0, 10.0) == INDEX_NONE, "0-item wheel -> none");
    CHECK(IndexAtAngle(1.0, -3) == INDEX_NONE, "negative count -> none");
    CHECK(SelectionAt(FVector2D(0.0, 50.0), 1, 10.0) == 0, "1-item wheel -> slice 0");

    if (g_fail != 0)
    {
        std::printf("=== radial checks FAILED (%d) ===\n", g_fail);
        return 1;
    }
    std::printf("=== all radial checks PASSED ===\n");
    return 0;
}
