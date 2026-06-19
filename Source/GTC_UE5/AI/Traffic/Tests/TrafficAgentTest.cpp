// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TrafficAgent.h"
#include "../../../World/RoadNetwork/RoadNetwork.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTrafficAgent — a single car driving the real FRoadNetwork graph.
 * GTC-original (no parity oracle): exercises the composition of FLanePath,
 * FTurnChoice and FTrafficModel that the agent ties together. Builds a 3x3 city
 * grid (100 m blocks) and drives a car over it.
 */
namespace
{
    constexpr double GridStep = 100.0;

    // A 3x3 grid of intersections wired both ways along every row and column.
    FRoadNetwork MakeGrid()
    {
        FRoadNetwork Net(1.0);
        for (int32 z = 0; z < 3; ++z)
        {
            Net.AddPolyline(TArray<FVector>({FVector(0, 0, z * GridStep),
                FVector(GridStep, 0, z * GridStep), FVector(2 * GridStep, 0, z * GridStep)}));
        }
        for (int32 x = 0; x < 3; ++x)
        {
            Net.AddPolyline(TArray<FVector>({FVector(x * GridStep, 0, 0),
                FVector(x * GridStep, 0, GridStep), FVector(x * GridStep, 0, 2 * GridStep)}));
        }
        return Net;
    }

    int32 NodeAt(const FRoadNetwork& Net, const FVector& P)
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

    // The directed segment From->To, or -1 if there is no such edge.
    int32 SegmentBetween(const FRoadNetwork& Net, int32 From, int32 To)
    {
        for (const int32 Seg : Net.SegmentsFrom(From))
        {
            if (Net.SegmentEndNode(Seg) == To)
            {
                return Seg;
            }
        }
        return -1;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTrafficAgentTest,
    "GTC.AI.Traffic.Agent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficAgentTest::RunTest(const FString& Parameters)
{
    const FRoadNetwork Net = MakeGrid();
    TestEqual(TEXT("grid has 9 nodes"), Net.NodeCount(), 9);
    TestEqual(TEXT("grid has 24 directed segments"), Net.SegmentCount(), 24);

    const int32 N00 = NodeAt(Net, FVector(0, 0, 0));
    const int32 N10 = NodeAt(Net, FVector(GridStep, 0, 0));
    const int32 N22 = NodeAt(Net, FVector(2 * GridStep, 0, 2 * GridStep));

    // --- StartOnSegment validity ------------------------------------------------
    {
        FTrafficAgent Car;
        TestFalse(TEXT("invalid start segment -> not driving"),
            Car.StartOnSegment(Net, 999, 0.0, 5.0));
        TestFalse(TEXT("not driving after bad start"), Car.IsDriving());
        TestEqual(TEXT("segment is -1 after bad start"), Car.Segment(), -1);

        const int32 Seg = SegmentBetween(Net, N00, N10);
        TestTrue(TEXT("found the N00->N10 segment"), Seg >= 0);
        TestTrue(TEXT("valid start segment -> driving"), Car.StartOnSegment(Net, Seg, 0.0, 5.0));
        TestTrue(TEXT("driving after good start"), Car.IsDriving());
        // Pose at the very start sits at the start node (lateral offset 0).
        TestTrue(TEXT("starts at the start node"),
            Car.Pose().Pos.Equals(FVector(0, 0, 0), 1e-3));
    }

    // --- IDM longitudinal control: brake for a near leader, cruise on open road --
    {
        FTrafficAgent Car;
        Car.Drive.DesiredSpeed = 12.0;
        const int32 Seg = SegmentBetween(Net, N00, N10);
        Car.StartOnSegment(Net, Seg, 0.0, 5.0);

        // A leader right on the bumper (minimum gap, stopped) must slow the car.
        const double Before = Car.GetSpeed();
        Car.Step(Net, 0.2, /*LeaderGap*/ FTrafficAgent::FDriveParams().MinGap, /*LeaderSpeed*/ 0.0);
        TestTrue(TEXT("brakes for a stopped leader on the bumper"), Car.GetSpeed() < Before);

        // Open road: the car accelerates toward its desired speed.
        FTrafficAgent Cruiser;
        Cruiser.Drive.DesiredSpeed = 12.0;
        Cruiser.StartOnSegment(Net, Seg, 0.0, 5.0);
        const double Was = Cruiser.GetSpeed();
        Cruiser.Step(Net, 0.2, FTrafficAgent::OpenRoadGap, 0.0);
        TestTrue(TEXT("accelerates on open road"), Cruiser.GetSpeed() > Was);
        TestTrue(TEXT("never exceeds desired by more than a hair"),
            Cruiser.GetSpeed() <= 12.0 + 1e-6);
    }

    // --- Free-roam: go straight through a junction, turn at the corner, never strand
    {
        FTrafficAgent Car;
        Car.Drive.DesiredSpeed = 12.0;
        Car.StartOnSegment(Net, SegmentBetween(Net, N00, N10), 0.0, 8.0);

        bool bWentStraightPastMiddle = false; // reached the far bottom-right area, X high, Z low
        bool bTurnedUp = false;               // later climbed in +Z (turned the corner)
        bool bEverStranded = false;
        for (int32 i = 0; i < 600; ++i)
        {
            Car.Step(Net, 0.1, FTrafficAgent::OpenRoadGap, 0.0);
            if (!Car.IsDriving())
            {
                bEverStranded = true;
                break;
            }
            const FVector P = Car.Pose().Pos;
            TestFalse(TEXT("pose never NaNs"), P.ContainsNaN());
            TestTrue(TEXT("speed stays bounded"),
                Car.GetSpeed() >= 0.0 && Car.GetSpeed() <= 12.0 + 1e-6);
            if (P.X > 1.5 * GridStep && FMath::Abs(P.Z) < 5.0)
            {
                bWentStraightPastMiddle = true;
            }
            if (bWentStraightPastMiddle && P.Z > 5.0)
            {
                bTurnedUp = true;
            }
        }
        TestFalse(TEXT("free-roam car never strands on the grid"), bEverStranded);
        TestTrue(TEXT("drove straight through the middle junction"), bWentStraightPastMiddle);
        TestTrue(TEXT("turned the corner at the grid edge"), bTurnedUp);
    }

    // --- Routed: follow an A* path around a corner to the destination -----------
    {
        const TArray<int32> Path = Net.FindPath(N00, N22);
        TestTrue(TEXT("A* finds a path N00->N22"),
            Path.Num() >= 3 && Path[0] == N00 && Path.Last() == N22);

        FTrafficAgent Car;
        Car.Drive.DesiredSpeed = 12.0;
        Car.SetRoute(Path);
        Car.StartOnSegment(Net, SegmentBetween(Net, Path[0], Path[1]), 0.0, 8.0);

        const FVector Goal = Net.NodePosition(N22);
        double MinDistToGoal = TNumericLimits<double>::Max();
        bool bExhausted = false;
        for (int32 i = 0; i < 1200; ++i)
        {
            Car.Step(Net, 0.1, FTrafficAgent::OpenRoadGap, 0.0);
            if (!Car.IsDriving())
            {
                break;
            }
            MinDistToGoal = FMath::Min(MinDistToGoal, FVector::Dist(Car.Pose().Pos, Goal));
            if (Car.IsRouteExhausted())
            {
                bExhausted = true;
            }
        }
        TestTrue(TEXT("routed car reaches its destination node"), MinDistToGoal < 3.0);
        TestTrue(TEXT("route reports exhausted after the last node"), bExhausted);
    }

    // --- Lateral offset places the car on its side of the centreline ------------
    {
        FTrafficAgent Car;
        Car.LateralOffset = 3.0; // 3 m to the right of travel (right-hand traffic)
        // Travel +X; RightOf((1,0,0)) == (0,0,-1) in the XZ frame, so the lane sits
        // at Z = -3 along the bottom row.
        Car.StartOnSegment(Net, SegmentBetween(Net, N00, N10), GridStep * 0.5, 0.0);
        TestTrue(TEXT("lane is offset to the driver's right (Z = -3)"),
            FMath::Abs(Car.Pose().Pos.Z + 3.0) <= 1e-3);
    }

    // --- Determinism: identical inputs -> identical motion ----------------------
    {
        auto Drive = [&Net](FVector& OutPos, double& OutSpeed)
        {
            FTrafficAgent Car;
            Car.Drive.DesiredSpeed = 11.0;
            Car.StartOnSegment(Net, SegmentBetween(Net, NodeAt(Net, FVector(0, 0, 0)),
                                                   NodeAt(Net, FVector(GridStep, 0, 0))),
                0.0, 6.0);
            for (int32 i = 0; i < 200; ++i)
            {
                Car.Step(Net, 0.1, FTrafficAgent::OpenRoadGap, 0.0);
            }
            OutPos = Car.Pose().Pos;
            OutSpeed = Car.GetSpeed();
        };
        FVector PosA, PosB;
        double SpeedA, SpeedB;
        Drive(PosA, SpeedA);
        Drive(PosB, SpeedB);
        TestTrue(TEXT("two identical runs end at the same pose"), PosA.Equals(PosB, 0.0));
        TestEqual(TEXT("two identical runs end at the same speed"), SpeedA, SpeedB);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
