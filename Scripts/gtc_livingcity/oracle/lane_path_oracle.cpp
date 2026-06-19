// Out-of-tree oracle for FLanePath — compiles + runs the REAL LanePath.cpp and
// re-asserts the GTC.World.LanePath cases headlessly (no UE editor needed).
#include "../../../Source/GTC_UE5/World/RoadNetwork/LanePath.h"
#include "harness.h"

int main()
{
    // RightOf convention.
    Check(VecNear(FLanePath::RightOf(FVector(1, 0, 0)), FVector(0, 0, -1)), "right of +X is -Z");
    Check(VecNear(FLanePath::RightOf(FVector(0, 0, 1)), FVector(1, 0, 0)), "right of +Z is +X");
    Check(VecNear(FLanePath::RightOf(FVector::ZeroVector), FVector::ZeroVector), "right of zero is zero");

    // Straight lane along +X, offset 2 to the right (== -Z).
    {
        FLanePath Lane;
        Lane.BuildFromCenterline(TArray<FVector>({FVector(0, 0, 0), FVector(10, 0, 0), FVector(20, 0, 0)}), 2.0);

        Check(Lane.NumPoints() == 3, "point count preserved");
        CheckNear(Lane.Length(), 20.0, "length is 20");
        Check(VecNear(Lane.Point(0), FVector(0, 0, -2)), "vertices shifted to -Z by offset");
        Check(VecNear(Lane.Point(2), FVector(20, 0, -2)), "last vertex shifted too");

        const FLanePath::FPose Mid = Lane.PoseAtDistance(5.0);
        Check(VecNear(Mid.Pos, FVector(5, 0, -2)), "pose at s=5 sits mid-lane");
        Check(VecNear(Mid.Heading, FVector(1, 0, 0)), "pose heading is +X");

        const FLanePath::FPose End = Lane.PoseAtDistance(999.0);
        Check(VecNear(End.Pos, FVector(20, 0, -2)), "pose clamps to lane end");

        CheckNear(Lane.DistanceToEnd(5.0), 15.0, "distance to end from s=5");
        CheckNear(Lane.DistanceToEnd(25.0), 0.0, "distance to end clamps at 0");

        bool bReached = false;
        const double S1 = Lane.Advance(18.0, 5.0, bReached);
        CheckNear(S1, 20.0, "advance clamps to length");
        Check(bReached, "advance past end reports reached");

        const double S2 = Lane.Advance(0.0, 5.0, bReached);
        CheckNear(S2, 5.0, "advance mid-lane moves s");
        Check(!bReached, "advance mid-lane not reached");

        const double S3 = Lane.Advance(20.0, -3.0, bReached);
        CheckNear(S3, 17.0, "reverse advance moves back");
        Check(!bReached, "reverse advance never reports reached");
    }

    // Right-angle corner: averaged-normal offset at the bend.
    {
        FLanePath Lane;
        Lane.BuildFromCenterline(TArray<FVector>({FVector(0, 0, 0), FVector(10, 0, 0), FVector(10, 0, 10)}), 2.0);
        Check(VecNear(Lane.Point(0), FVector(0, 0, -2)), "corner start offset -Z");
        Check(VecNear(Lane.Point(2), FVector(12, 0, 10)), "corner end offset +X");
        const double H = 2.0 / std::sqrt(2.0);
        Check(VecNear(Lane.Point(1), FVector(10.0 + H, 0.0, -H)), "corner bend uses averaged normal");
    }

    // Degenerate inputs.
    {
        FLanePath Empty;
        Empty.BuildFromCenterline(TArray<FVector>(), 2.0);
        Check(Empty.NumPoints() == 0, "empty lane has no points");
        CheckNear(Empty.Length(), 0.0, "empty lane length 0");
        const FLanePath::FPose P = Empty.PoseAtDistance(5.0);
        Check(VecNear(P.Pos, FVector::ZeroVector), "empty pose is origin");
        Check(VecNear(P.Heading, FVector(0, 0, -1)), "empty pose forward fallback");

        FLanePath Single;
        Single.BuildFromCenterline(TArray<FVector>({FVector(3, 0, 4)}), 2.0);
        Check(Single.NumPoints() == 1, "single-point lane keeps its point");
        CheckNear(Single.Length(), 0.0, "single lane length 0");
        Check(VecNear(Single.PoseAtDistance(9.0).Pos, FVector(3, 0, 4)), "single pose is that point");
    }

    return OracleSummary("FLanePath");
}
