// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrafficAgent.h"

#include "TurnChoice.h"
#include "../../World/RoadNetwork/RoadNetwork.h"
#include "../../Worldcore/TrafficModel.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    /** Travel direction of a directed segment (unit; zero for a degenerate one). */
    FVector SegmentHeading(const FRoadNetwork& Net, int32 Seg)
    {
        const FVector A = Net.NodePosition(Net.SegmentStartNode(Seg));
        const FVector B = Net.NodePosition(Net.SegmentEndNode(Seg));
        return (B - A).GetSafeNormal();
    }

    /** Most junctions a car may cross in a single Step (bounds a tiny-lane spin). */
    constexpr int32 MaxJunctionsPerStep = 8;
}

void FTrafficAgent::SetRoute(const TArray<int32>& InRouteNodes)
{
    RouteNodes = InRouteNodes;
    RouteCursor = 0;
}

void FTrafficAgent::BakeLane(const FRoadNetwork& Net, int32 NewSeg)
{
    // A single directed segment is a straight A->B centreline; the lane is that
    // centreline offset to the driver's side (FLanePath applies LateralOffset).
    const FVector A = Net.NodePosition(Net.SegmentStartNode(NewSeg));
    const FVector B = Net.NodePosition(Net.SegmentEndNode(NewSeg));
    Lane.BuildFromCenterline(TArray<FVector>({A, B}), LateralOffset);
    Seg = NewSeg;
}

bool FTrafficAgent::StartOnSegment(
    const FRoadNetwork& Net, int32 StartSeg, double StartS, double InitialSpeed)
{
    if (StartSeg < 0 || StartSeg >= Net.SegmentCount())
    {
        Seg = -1;
        return false;
    }
    BakeLane(Net, StartSeg);
    S = FMath::Clamp(StartS, 0.0, Lane.Length());
    Speed = FMath::Max(0.0, InitialSpeed);
    return true;
}

bool FTrafficAgent::AdvanceToNextSegment(const FRoadNetwork& Net)
{
    const int32 EndNode = Net.SegmentEndNode(Seg);
    if (EndNode < 0)
    {
        Seg = -1;
        return false;
    }
    const TArray<int32>& Out = Net.SegmentsFrom(EndNode);
    if (Out.Num() == 0)
    {
        Seg = -1; // genuine dead end (no outgoing segment at all)
        return false;
    }

    // The candidate continuations: each outgoing segment's destination node and
    // travel heading.
    TArray<int32> OutEndNodes;
    TArray<FVector> OutHeadings;
    OutEndNodes.Reserve(Out.Num());
    OutHeadings.Reserve(Out.Num());
    for (const int32 OutSeg : Out)
    {
        OutEndNodes.Add(Net.SegmentEndNode(OutSeg));
        OutHeadings.Add(SegmentHeading(Net, OutSeg));
    }

    int32 Pick = -1;

    // 1) Follow the planned route, if the node we just reached is on it. Searching
    //    from the cursor (rather than a strict step) keeps us on-route even if a
    //    free-roam junction skipped a node.
    if (RouteNodes.Num() >= 2)
    {
        for (int32 i = RouteCursor; i + 1 < RouteNodes.Num(); ++i)
        {
            if (RouteNodes[i] == EndNode)
            {
                RouteCursor = i + 1;
                Pick = FTurnChoice::ChooseByRoute(OutEndNodes, RouteNodes[i + 1]);
                break;
            }
        }
    }

    // 2) Free-roam: continue the straightest way that is not a U-turn.
    const FVector InHeading = SegmentHeading(Net, Seg);
    if (Pick < 0)
    {
        Pick = FTurnChoice::ChooseStraightest(InHeading, OutHeadings, /*bAllowUTurn*/ false);
    }
    // 3) Dead-end fallback: the only way out is back the way we came -> U-turn.
    if (Pick < 0)
    {
        Pick = FTurnChoice::ChooseStraightest(InHeading, OutHeadings, /*bAllowUTurn*/ true);
    }
    if (Pick < 0)
    {
        Seg = -1;
        return false;
    }

    BakeLane(Net, Out[Pick]);
    S = 0.0;
    return true;
}

void FTrafficAgent::Step(const FRoadNetwork& Net, double Dt, double LeaderGap, double LeaderSpeed)
{
    if (Seg < 0 || Dt <= 0.0)
    {
        return;
    }

    // Longitudinal control: IDM acceleration toward the desired speed, braking for
    // the leader the caller handed us. Integrate to a non-negative speed.
    const double Accel = FTrafficModel::CarFollowingAccel(
        Speed, LeaderGap, LeaderSpeed, Drive.DesiredSpeed, Drive.MaxAccel, Drive.ComfortDecel,
        Drive.MinGap, Drive.TimeHeadway);
    Speed = FMath::Max(0.0, Speed + Accel * Dt);

    // Walk that distance along the lane, crossing junctions as needed.
    double Remaining = Speed * Dt;
    int32 Guard = 0;
    while (Remaining > FLanePath::Eps && Guard++ < MaxJunctionsPerStep)
    {
        bool bReachedEnd = false;
        const double Before = S;
        S = Lane.Advance(S, Remaining, bReachedEnd);
        Remaining -= (S - Before);
        if (!bReachedEnd)
        {
            break; // still mid-lane; distance spent.
        }
        if (!AdvanceToNextSegment(Net))
        {
            break; // stranded at a dead end; Seg is now -1.
        }
        // New lane baked, S reset to 0; loop spends any residual distance on it.
    }
}
