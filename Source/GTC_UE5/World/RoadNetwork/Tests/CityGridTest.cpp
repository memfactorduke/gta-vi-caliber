// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CityGrid.h"
#include "../RoadNetwork.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FCityGrid — the runtime grid road-graph source. GTC-original. Builds
 * an FRoadNetwork from the generated polylines and asserts it is a real, routable
 * grid (shared junctions, correct connectivity). File-local helpers are uniquely
 * prefixed for unity-build hygiene.
 */
namespace
{
    // Outgoing directed-segment count at the node nearest to P (-1 if no node there).
    int32 GtcCityGridDegreeAt(const FRoadNetwork& Net, const FVector& P)
    {
        const TArray<FVector>& Ns = Net.GetNodes();
        for (int32 i = 0; i < Ns.Num(); ++i)
        {
            if (Ns[i].Equals(P, Eps))
            {
                return Net.SegmentsFrom(i).Num();
            }
        }
        return -1;
    }

    int32 GtcCityGridNodeAt(const FRoadNetwork& Net, const FVector& P)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCityGridTest,
    "GTC.World.CityGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCityGridTest::RunTest(const FString& Parameters)
{
    FCityGrid::FSpec Spec;
    Spec.Origin = FVector::ZeroVector;
    Spec.BlockSize = 100.0;
    Spec.BlocksX = 2;
    Spec.BlocksZ = 2;

    // Lattice counts and centring.
    TestEqual(TEXT("3 intersection columns"), FCityGrid::CountX(Spec), 3);
    TestEqual(TEXT("3 intersection rows"), FCityGrid::CountZ(Spec), 3);
    TestTrue(TEXT("centre intersection lands on Origin"),
        FCityGrid::Intersection(Spec, 1, 1).Equals(FVector::ZeroVector, Eps));
    TestTrue(TEXT("corner (0,0) is down-left of Origin"),
        FCityGrid::Intersection(Spec, 0, 0).Equals(FVector(-100, 0, -100), Eps));

    // Build a real road graph from the polylines.
    FRoadNetwork Net(1.0);
    for (const TArray<FVector>& Poly : FCityGrid::Polylines(Spec))
    {
        Net.AddPolyline(Poly);
    }
    TestEqual(TEXT("3x3 lattice -> 9 junction nodes"), Net.NodeCount(), 9);
    // 12 undirected block edges -> 24 directed segments.
    TestEqual(TEXT("24 directed segments"), Net.SegmentCount(), 24);

    // Junction degrees: interior 4-way, edge 3-way, corner 2-way.
    TestEqual(TEXT("interior node is 4-way"),
        GtcCityGridDegreeAt(Net, FVector(0, 0, 0)), 4);
    TestEqual(TEXT("edge node is 3-way"),
        GtcCityGridDegreeAt(Net, FVector(0, 0, -100)), 3);
    TestEqual(TEXT("corner node is 2-way"),
        GtcCityGridDegreeAt(Net, FVector(-100, 0, -100)), 2);

    // The grid is fully connected: A* threads opposite corners.
    {
        const int32 A = GtcCityGridNodeAt(Net, FVector(-100, 0, -100));
        const int32 B = GtcCityGridNodeAt(Net, FVector(100, 0, 100));
        const TArray<int32> Path = Net.FindPath(A, B);
        TestTrue(TEXT("A* connects opposite corners"),
            Path.Num() >= 5 && Path[0] == A && Path.Last() == B);
    }

    // A degenerate spec still yields a minimal 2x2 graph rather than nothing.
    {
        FCityGrid::FSpec Tiny;
        Tiny.BlocksX = 0; // clamped up to 1
        Tiny.BlocksZ = 0;
        TestEqual(TEXT("clamped to 2 columns"), FCityGrid::CountX(Tiny), 2);
        FRoadNetwork TinyNet(1.0);
        for (const TArray<FVector>& Poly : FCityGrid::Polylines(Tiny))
        {
            TinyNet.AddPolyline(Poly);
        }
        TestEqual(TEXT("2x2 lattice -> 4 nodes"), TinyNet.NodeCount(), 4);
    }

    // SnapOriginTo rounds to the nearest block boundary from Origin.
    TestTrue(TEXT("snaps 140 -> 100"),
        FCityGrid::SnapOriginTo(Spec, FVector(140, 0, 0)).Equals(FVector(100, 0, 0), Eps));
    TestTrue(TEXT("snaps 160 -> 200, -260 -> -300"),
        FCityGrid::SnapOriginTo(Spec, FVector(160, 0, -260)).Equals(FVector(200, 0, -300), Eps));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
