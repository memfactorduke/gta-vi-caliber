// Out-of-tree oracle for FTailResolver — compiles + runs the REAL TailResolver.cpp against the
// REAL TailObjective.cpp and re-asserts the "tail the mark" adapter the live
// UGTCTailObjectiveComponent leans on: follower/mark positions -> distance + view-cone -> the
// two-sided-failure FTailObjective (lost / spotted / succeeded).
#include "../../../Source/GTC_UE5/Missions/Activities/TailResolver.h"
#include "../../../Source/GTC_UE5/Missions/Activities/TailObjective.h"
#include "harness.h"

static FTailResolver::FOutput Cook(const FVector& Follower, const FVector& Target, const FVector& Fwd)
{
    FTailResolver::FParams P; // 45 deg half-angle, 4000 cm range
    FTailResolver::FInput In;
    In.FollowerPos = Follower;
    In.TargetPos = Target;
    In.TargetForward = Fwd;
    return FTailResolver::Cook(In, P);
}

int main()
{
    const FVector O(0, 0, 0);
    const FVector FwdX(1, 0, 0);

    // --- View cone: in front & in range = seen; behind / wide / far = not. ---
    {
        Check(Cook(FVector(2000, 0, 0), O, FwdX).bInView, "directly in front, in range -> in view");
        Check(!Cook(FVector(-2000, 0, 0), O, FwdX).bInView, "behind the mark -> not in view");
        Check(!Cook(FVector(0, 2000, 0), O, FwdX).bInView, "90 deg to the side (> 45 half-angle) -> not in view");
        Check(Cook(FVector(1732, 1000, 0), O, FwdX).bInView, "30 deg off (< 45 half-angle) -> in view");
        Check(!Cook(FVector(5000, 0, 0), O, FwdX).bInView, "in front but beyond range -> not in view");
        Check(Cook(O, O, FwdX).bInView, "right on top of the mark -> in view");
        CheckNear(Cook(FVector(1500, 0, 0), O, FwdX).Distance, 1500.0, "distance is the follower->mark length");

        // Ground-plane (yaw-only): a mark pitched up a ramp still sees a follower dead ahead, and
        // height (Z) is dropped from the tail distance.
        Check(Cook(FVector(2000, 0, 0), O, FVector(1, 0, 2)).bInView,
            "a mark facing up a ramp still sees a follower dead ahead (cone is yaw-only)");
        CheckNear(Cook(FVector(1500, 0, 5000), O, FwdX).Distance, 1500.0,
            "height is dropped from the tail distance (ground-plane)");
    }

    // --- End-to-end: ride the band out of view for the required time -> succeed. ---
    {
        FTailObjective T;
        FTailObjective::FParams TP; // Min 400, Max 3000, grace 4, rise 0.4, fall 0.3, required 20
        T.Configure(TP);
        for (int i = 0; i < 20; ++i)
        {
            const auto C = Cook(FVector(-1500, 0, 0), O, FwdX); // 1500 cm behind, out of view
            T.Update(C.Distance, C.bInView, 1.0);
        }
        Check(T.IsSucceeded(), "riding the band out of view for 20s succeeds");
        CheckNear(T.Suspicion(), 0.0, "a clean tail never raises suspicion");
    }

    // --- End-to-end: drift too far -> the lost grace clock expires. ---
    {
        FTailObjective T;
        FTailObjective::FParams TP;
        T.Configure(TP);
        bool Lost = false;
        for (int i = 0; i < 5; ++i)
        {
            const auto C = Cook(FVector(-4000, 0, 0), O, FwdX); // 4000 cm > MaxDistance 3000
            T.Update(C.Distance, C.bInView, 1.0);
            if (T.IsLost())
            {
                Lost = true;
            }
        }
        Check(Lost, "staying past MaxDistance beyond the grace period loses the mark");
    }

    // --- End-to-end: sit too close & in view -> suspicion fills and you're spotted. ---
    {
        FTailObjective T;
        FTailObjective::FParams TP;
        T.Configure(TP);
        bool Spotted = false;
        for (int i = 0; i < 4; ++i)
        {
            const auto C = Cook(FVector(300, 0, 0), O, FwdX); // 300 cm < MinDistance 400, and in view
            Check(C.bInView, "sitting in front and close reads as in the mark's view");
            T.Update(C.Distance, C.bInView, 1.0);
            if (T.IsSpotted())
            {
                Spotted = true;
            }
        }
        Check(Spotted, "hugging the mark in its view fills suspicion and blows the tail");
    }

    return OracleSummary("tail_oracle");
}
