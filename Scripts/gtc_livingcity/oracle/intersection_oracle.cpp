// Out-of-tree oracle for FIntersection — compiles + runs the REAL Intersection.cpp
// and re-asserts GTC.AI.Traffic.Intersection.
#include "../../../Source/GTC_UE5/AI/Traffic/Intersection.h"
#include "harness.h"

int main()
{
    using FApproach = FIntersection::FApproach;

    {
        TArray<FApproach> Claims;
        Claims.Add(FApproach{0, 5.0, 1});
        Claims.Add(FApproach{1, 5.0, 0});
        Check(FIntersection::RightOfWay(Claims) == 0, "major road has right-of-way");
        Check(FIntersection::Yields(Claims[1], Claims[0]), "minor yields to major");
        Check(!FIntersection::Yields(Claims[0], Claims[1]), "major does not yield");
        Check(FIntersection::MayProceed(Claims, 0), "major may proceed");
        Check(!FIntersection::MayProceed(Claims, 1), "minor must wait");
    }

    {
        TArray<FApproach> Claims;
        Claims.Add(FApproach{0, 8.0, 0});
        Claims.Add(FApproach{1, 3.0, 0});
        Claims.Add(FApproach{2, 5.0, 0});
        Check(FIntersection::RightOfWay(Claims) == 1, "earliest arrival has right-of-way");
    }

    {
        TArray<FApproach> Claims;
        Claims.Add(FApproach{2, 4.0, 0});
        Claims.Add(FApproach{0, 4.0, 0});
        Claims.Add(FApproach{1, 4.0, 0});
        Check(FIntersection::RightOfWay(Claims) == 1, "tie breaks to lowest approach id");
    }

    Check(FIntersection::RightOfWay(TArray<FApproach>()) == -1, "no claims -> none");
    Check(!FIntersection::MayProceed(TArray<FApproach>(), 0), "out-of-range may not proceed");

    CheckNear(FIntersection::StopLineGap(8.0, 2.25), 5.75, "stop-line gap nets front half");
    CheckNear(FIntersection::StopLineGap(1.0, 2.25), 0.0, "stop-line gap clamps at the line");

    return OracleSummary("FIntersection");
}
