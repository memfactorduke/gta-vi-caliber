// Out-of-tree oracle for FRoadRoute — compiles + runs the REAL RoadRoute.cpp
// (and LanePath.cpp for the pipeline) and re-asserts GTC.World.RoadRoute.
#include "../../../Source/GTC_UE5/World/RoadNetwork/RoadRoute.h"
#include "../../../Source/GTC_UE5/World/RoadNetwork/LanePath.h"
#include "harness.h"

int main()
{
    const TArray<FVector> Nodes({
        FVector(0, 0, 0), FVector(10, 0, 0), FVector(10, 0, 10), FVector(20, 0, 10)});

    {
        const TArray<FVector> C = FRoadRoute::Centerline(Nodes, TArray<int32>({0, 1, 2, 3}));
        Check(C.Num() == 4, "centerline has 4 points");
        Check(VecNear(C[0], Nodes[0]), "first point is node 0");
        Check(VecNear(C[3], Nodes[3]), "last point is node 3");
    }

    Check(FRoadRoute::Centerline(Nodes, TArray<int32>({0, 9})).IsEmpty(), "out-of-range index -> empty");
    Check(FRoadRoute::Centerline(Nodes, TArray<int32>()).IsEmpty(), "empty path -> empty");

    {
        const TArray<FVector> C = FRoadRoute::CenterlineThrough(
            Nodes, TArray<int32>({0, 1, 2, 3}), FVector(-5, 0, 0), FVector(25, 0, 10));
        Check(C.Num() == 6, "threaded centerline has 6 points");
        Check(VecNear(C[0], FVector(-5, 0, 0)), "threaded starts at spawn");
        Check(VecNear(C[5], FVector(25, 0, 10)), "threaded ends at destination");
    }

    {
        const TArray<FVector> C = FRoadRoute::CenterlineThrough(
            Nodes, TArray<int32>({0, 1, 2, 3}), FVector(0, 0, 0), FVector(20, 0, 10));
        Check(C.Num() == 4, "coincident ends are not duplicated");
    }

    {
        const TArray<FVector> C = FRoadRoute::Centerline(Nodes, TArray<int32>({0, 1, 2, 3}));
        FLanePath Lane;
        Lane.BuildFromCenterline(C, 2.0);
        Check(Lane.NumPoints() == 4, "lane keeps the route's vertices");
        Check(Lane.Length() > 1e-4, "lane has positive drivable length");
    }

    return OracleSummary("FRoadRoute");
}
