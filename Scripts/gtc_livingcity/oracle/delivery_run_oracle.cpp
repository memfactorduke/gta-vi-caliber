// Out-of-tree oracle for FDeliveryRunResolver — compiles + runs the REAL
// DeliveryRunResolver.cpp against the REAL DeliveryRun.cpp and re-asserts the courier
// adapter the live UGTCDeliveryRunComponent leans on: distance-to-drop + crash impulse ->
// monotonic route progress + cargo damage -> FDeliveryRun. This is the pure half the
// UObject shell only plumbs.
#include "../../../Source/GTC_UE5/Missions/Activities/DeliveryRunResolver.h"
#include "../../../Source/GTC_UE5/Missions/Activities/DeliveryRun.h"
#include "harness.h"

// Radius defaults to 0 for the pure-fraction cases; arrival-radius cases pass it explicitly.
static FDeliveryRunResolver::FOutput Cook(double Dist, double Route, double Prev, double Impulse, double Radius = 0.0)
{
    FDeliveryRunResolver::FParams P;
    P.DamagePerImpulse = 0.0002;
    P.ArrivalRadius = Radius;
    FDeliveryRunResolver::FInput In;
    In.DistanceToDropoff = Dist;
    In.RouteLength = Route;
    In.PrevProgress = Prev;
    In.ImpactImpulse = Impulse;
    return FDeliveryRunResolver::Cook(In, P);
}

int main()
{
    // Progress fraction along a 1000-unit route.
    CheckNear(Cook(1000, 1000, 0, 0).Progress, 0.0, "at the start -> 0 progress");
    CheckNear(Cook(500, 1000, 0, 0).Progress, 0.5, "halfway -> 0.5 progress");
    CheckNear(Cook(500, 1000, 0, 0).ProgressDelta, 0.5, "halfway from 0 -> 0.5 delta");
    CheckNear(Cook(0, 1000, 0, 0).Progress, 1.0, "at the drop -> full progress");

    // Monotonic high-water: backtracking never re-awards the route.
    CheckNear(Cook(1000, 1000, 0.5, 0).Progress, 0.5, "backtracking holds the high-water progress");
    CheckNear(Cook(1000, 1000, 0.5, 0).ProgressDelta, 0.0, "backtracking awards no new progress");
    CheckNear(Cook(500, 1000, 0.5, 0).ProgressDelta, 0.0, "re-covering old ground awards no delta");
    CheckNear(Cook(250, 1000, 0.5, 0).ProgressDelta, 0.25, "only new ground past the high-water counts");

    // Degenerate routes.
    CheckNear(Cook(0, 0, 0, 0).Progress, 1.0, "zero-length route is already delivered");
    CheckNear(Cook(2000, 1000, 0, 0).ProgressDelta, 0.0, "driving past the start clamps to 0");

    // Arrival radius: progress reaches 1.0 near (not exactly on) the drop, and ramps to it.
    CheckNear(Cook(400, 1000, 0, 0, 500).Progress, 1.0, "within the arrival radius -> full progress");
    CheckNear(Cook(500, 1000, 0, 0, 500).Progress, 1.0, "exactly at the radius edge -> full progress");
    CheckNear(Cook(750, 1000, 0, 0, 500).Progress, 0.5, "progress ramps to 1.0 at the radius (750 -> 0.5)");
    CheckNear(Cook(300, 400, 0, 0, 500).Progress, 1.0, "a route shorter than the radius is already there");

    // Cargo damage.
    CheckNear(Cook(500, 1000, 0, 5000).Damage, 5000.0 * 0.0002, "impulse -> proportional cargo damage");
    CheckNear(Cook(500, 1000, 0, 0).Damage, 0.0, "no impact -> no damage");
    CheckNear(Cook(500, 1000, 0, -100).Damage, 0.0, "negative impulse ignored");

    // End-to-end: a clean run delivers with a healthy payout.
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 100.0;
        D.Configure(DP);
        double High = 0.0;
        for (int i = 1; i <= 10; ++i)
        {
            const double Dist = 1000.0 - i * 100.0;
            const auto C = Cook(Dist, 1000, High, 0);
            High = C.Progress;
            D.Update(C.ProgressDelta, C.Damage, 1.0);
        }
        Check(D.IsDelivered(), "clean run delivers");
        Check(D.PayoutFactor() > 0.8, "clean run pays well");
    }

    // End-to-end with the arrival radius: stopping NEAR the drop (400 cm) still delivers.
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 100.0;
        D.Configure(DP);
        double High = 0.0;
        for (int i = 1; i <= 6; ++i)
        {
            const double Dist = FMath::Max(400.0, 1000.0 - i * 100.0);
            const auto C = Cook(Dist, 1000, High, 0, 500);
            High = C.Progress;
            D.Update(C.ProgressDelta, C.Damage, 1.0);
        }
        Check(D.IsDelivered(), "stopping within the arrival radius delivers");
    }

    // End-to-end: oscillating short of the drop never falsely delivers (high-water regression).
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 1000.0;
        D.Configure(DP);
        double High = 0.0;
        for (int i = 0; i < 20; ++i)
        {
            const double Dist = (i % 2 == 0) ? 500.0 : 1000.0;
            const auto C = Cook(Dist, 1000, High, 0);
            High = C.Progress;
            D.Update(C.ProgressDelta, C.Damage, 1.0);
        }
        Check(!D.IsDelivered(), "oscillating short of the drop never delivers");
        CheckNear(D.Progress(), 0.5, "progress capped at the furthest reached (0.5)");
    }

    // End-to-end: a massive crash wrecks the cargo.
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 100.0;
        D.Configure(DP);
        const auto C = Cook(900, 1000, 0, 100000);
        D.Update(C.ProgressDelta, C.Damage, 1.0);
        Check(D.IsWrecked(), "a massive crash wrecks the cargo");
    }

    return OracleSummary("delivery_run_oracle");
}
