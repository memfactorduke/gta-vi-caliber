// Copyright (c) 2026 GTC contributors

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../RoadNetwork.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Parity oracle: game/tests/unit/test_road_network.gd. Each It(...) maps 1:1 to
// one the reference test function. Vector "is_equal_approx" checks use the shared
// GtcTest::Eps; the oracle's explicit `< 0.001` magnitude checks are reproduced
// as-is.
// _line(a, b) helper from the oracle. Uniquely prefixed for DEFAULT-unity hygiene.
static TArray<FVector> GtcRoadNetworkLine(const FVector& A, const FVector& B)
{
    return TArray<FVector>({A, B});
}

// _node_at(net, p): index of the node approx-equal to p, or -1.
static int32 GtcRoadNetworkNodeAt(const FRoadNetwork& Net, const FVector& P)
{
    const TArray<FVector>& Ns = Net.GetNodes();
    for (int32 i = 0; i < Ns.Num(); ++i)
    {
        if (Ns[i].Equals(P, Eps))
        {
            return i;
        }
    }
    return -1;
}

BEGIN_DEFINE_SPEC(FRoadNetworkSpec, "GTC.World.RoadNetwork",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FRoadNetworkSpec)

void FRoadNetworkSpec::Define()
{
    // test_single_segment_is_two_way
    It("makes a single segment two-way", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        TestTrue(TEXT("2 nodes, 2 directed segments"),
            Net.NodeCount() == 2 && Net.SegmentCount() == 2);
    });

    // test_shared_endpoints_merge_into_junction
    It("merges shared endpoints into a junction", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        Net.AddPolyline(GtcRoadNetworkLine(FVector(10, 0, 0), FVector(10, 0, 10)));
        TestTrue(TEXT("3 nodes, 4 directed segments"),
            Net.NodeCount() == 3 && Net.SegmentCount() == 4);
    });

    // test_close_points_snap_together
    It("snaps close points together", [this]()
    {
        FRoadNetwork Net(2.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        Net.AddPolyline(GtcRoadNetworkLine(FVector(10.5, 0, 0.2), FVector(20, 0, 0)));
        TestEqual(TEXT("endpoint within 2 m grid snaps -> 3 nodes"), Net.NodeCount(), 3);
    });

    // test_segments_from_junction
    It("reports outgoing segments from a junction", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        Net.AddPolyline(GtcRoadNetworkLine(FVector(10, 0, 0), FVector(10, 0, 10)));
        int32 Junction = -1;
        for (int32 i = 0; i < Net.NodeCount(); ++i)
        {
            if (Net.GetNodes()[i].Equals(FVector(10, 0, 0), Eps))
            {
                Junction = i;
            }
        }
        TestTrue(TEXT("junction found with 2 outgoing segments"),
            Junction != -1 && Net.SegmentsFrom(Junction).Num() == 2);
    });

    // test_point_on_segment_endpoints_and_midpoint
    It("samples segment endpoints and midpoint", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        const FRoadNetwork::FSegmentPoint Start = Net.PointOnSegment(0, 0.0);
        const FRoadNetwork::FSegmentPoint Mid = Net.PointOnSegment(0, 5.0);
        const FRoadNetwork::FSegmentPoint Done = Net.PointOnSegment(0, 10.0);
        TestTrue(TEXT("start/mid/end positions"),
            Start.Pos.Equals(FVector(0, 0, 0), Eps)
            && Mid.Pos.Equals(FVector(5, 0, 0), Eps)
            && Done.Pos.Equals(FVector(10, 0, 0), Eps));
    });

    // test_heading_is_unit_along_segment
    It("gives a unit heading along the segment", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(0, 0, 8)));
        const FVector H = Net.PointOnSegment(0, 2.0).Heading;
        TestTrue(TEXT("unit length, points +Z"),
            FMath::Abs(H.Length() - 1.0) < 0.001 && H.Equals(FVector(0, 0, 1), Eps));
    });

    // test_offset_clamps_past_segment_end
    It("clamps an offset past the segment end", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        const FVector P = Net.PointOnSegment(0, 999.0).Pos;
        TestTrue(TEXT("clamps to the end node"), P.Equals(FVector(10, 0, 0), Eps));
    });

    // test_nearest_point_snaps_to_centreline
    It("snaps nearest point to the centreline", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(20, 0, 0)));
        const FRoadNetwork::FNearestPoint Np = Net.NearestPoint(FVector(10, 0, 3));
        TestTrue(TEXT("valid, on centreline, dist ~3 m"),
            Np.bValid
            && Np.Pos.Equals(FVector(10, 0, 0), Eps)
            && FMath::Abs(Np.Dist - 3.0) < 0.001);
    });

    // test_nearest_point_picks_closer_road
    It("picks the closer road", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(20, 0, 0)));   // z = 0
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 40), FVector(20, 0, 40))); // z = 40
        TestTrue(TEXT("snaps to the z = 0 road"),
            FMath::Abs(Net.NearestPoint(FVector(10, 0, 5)).Pos.Z) < 0.001);
    });

    // test_nearest_point_heading_along_road
    It("gives nearest heading along the road", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(0, 0, 20))); // north-south
        const FVector H = Net.NearestPoint(FVector(2, 0, 10)).Heading;
        TestTrue(TEXT("heading is along Z, no X"),
            FMath::Abs(FMath::Abs(H.Z) - 1.0) < 0.001 && FMath::Abs(H.X) < 0.001);
    });

    // test_nearest_point_empty_on_blank_graph
    It("returns empty on a blank graph", [this]()
    {
        FRoadNetwork Net(1.0);
        TestFalse(TEXT("blank graph -> invalid nearest point"),
            Net.NearestPoint(FVector(5, 0, 5)).bValid);
    });

    // test_find_path_connects_across_junction
    It("connects a path across a junction", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        Net.AddPolyline(GtcRoadNetworkLine(FVector(10, 0, 0), FVector(10, 0, 10)));
        const int32 Start = GtcRoadNetworkNodeAt(Net, FVector(0, 0, 0));
        const int32 Goal = GtcRoadNetworkNodeAt(Net, FVector(10, 0, 10));
        const TArray<int32> Path = Net.FindPath(Start, Goal);
        TestTrue(TEXT("path 0,0,0 -> 10,0,0 -> 10,0,10"),
            Path.Num() == 3 && Path[0] == Start && Path.Last() == Goal);
    });

    // test_find_path_same_node_is_singleton
    It("returns a singleton path for the same node", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));
        TestEqual(TEXT("same node -> [0]"), Net.FindPath(0, 0), TArray<int32>({0}));
    });

    // test_find_path_unreachable_is_empty
    It("returns empty for an unreachable goal", [this]()
    {
        FRoadNetwork Net(1.0);
        Net.AddPolyline(GtcRoadNetworkLine(FVector(0, 0, 0), FVector(10, 0, 0)));       // component A
        Net.AddPolyline(GtcRoadNetworkLine(FVector(100, 0, 0), FVector(110, 0, 0)));    // disjoint B
        const int32 A = GtcRoadNetworkNodeAt(Net, FVector(0, 0, 0));
        const int32 B = GtcRoadNetworkNodeAt(Net, FVector(100, 0, 0));
        TestEqual(TEXT("disjoint components -> empty path"), Net.FindPath(A, B).Num(), 0);
    });
}

#endif // WITH_AUTOMATION_TESTS
