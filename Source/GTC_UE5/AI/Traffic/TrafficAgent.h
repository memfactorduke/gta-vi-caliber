// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../../World/RoadNetwork/LanePath.h"

class FRoadNetwork;

/**
 * One car driving the REAL road graph — the keystone adapter that finally wires
 * the project's parity-tested traffic cores into a single moving vehicle, the
 * piece every header in the traffic stack called the "DEFERRED Wave-3 adapter".
 *
 * Until now the live traffic (UGTCTrafficSubsystem) drove an invisible
 * axis-aligned grid and never turned a corner; FRoadNetwork's A*, FRoadRoute,
 * FLanePath, FTurnChoice and FTrafficModel were all built and tested but unused
 * at runtime. FTrafficAgent composes them:
 *
 *   - rides ONE segment's offset lane (FLanePath) at arc-length `S`,
 *   - drives `S` forward each tick with the IDM longitudinal model
 *     (FTrafficModel), braking for the leader the caller hands it,
 *   - and, when it runs off the end of a segment, picks the next segment at the
 *     junction (FTurnChoice::ChooseStraightest — follow the route if one day it
 *     is given one, else continue the most natural way) and re-bakes its lane,
 *     carrying the residual distance so motion is continuous across the node.
 *
 * It owns NO actor and NO randomness: the subsystem adapter spawns/recycles the
 * AGTCTrafficVehicle, supplies the per-lane leader gap (FTrafficLane), and maps
 * the agent's XZ pose to UE Z-up. That keeps this a pure, deterministic,
 * headless-testable core (Tests/TrafficAgentTest.cpp, prefix GTC.AI.Traffic.Agent).
 *
 * Frame: the repo's planar XZ convention (Y up), matching FRoadNetwork /
 * FLanePath / FTurnChoice, so headings compose without a remap. Double precision
 * throughout. Speeds and distances are in the network's units (metres / m/s if
 * the graph is metric); the subsystem converts cm<->m at its boundary.
 *
 * Cornering: a single segment's lane is a straight offset polyline, so a turn at
 * a junction is a hard hand-off from one lane to the next (a slight corner cut).
 * Smoothing the turn arc is a later refinement; the routing/queueing behaviour —
 * the thing that makes traffic read as a city instead of a conveyor belt — is here.
 */
struct GTC_UE5_API FTrafficAgent
{
    /** IDM tuning, comfortable city driving. Units match the agent's speed units. */
    struct FDriveParams
    {
        double DesiredSpeed = 12.0; // v0 — free-road cruise
        double MaxAccel = 1.5;      // a
        double ComfortDecel = 2.0;  // b (positive)
        double MinGap = 2.0;        // s0 — standstill bumper distance
        double TimeHeadway = 1.4;   // T — desired time gap to the leader
    };

    /** A leader gap big enough to read as "open road" for the IDM. */
    static constexpr double OpenRoadGap = 1.0e6;

    /**
     * Lane the car rides on its side of travel: + places the lane to the driver's
     * RIGHT of the segment centreline (right-hand traffic), as FLanePath defines.
     */
    double LateralOffset = 0.0;

    /** Per-car IDM tuning (desired speed varies car-to-car). */
    FDriveParams Drive;

    /**
     * Give the car a planned node path (FRoadNetwork::FindPath output) to follow
     * through junctions, so it actually turns corners toward a destination instead
     * of always running straight. At each junction the car steers toward the next
     * node on the route (FTurnChoice::ChooseByRoute); once the route is exhausted or
     * the car has diverged from it, it free-roams the straightest way. Resets the
     * route cursor. Pass an empty path to clear the route (pure free-roam).
     */
    void SetRoute(const TArray<int32>& InRouteNodes);

    /**
     * True once a routed car has passed its final route node (so it is now
     * free-roaming). The subsystem uses this to hand the car a fresh destination.
     * Always false for an un-routed (free-roam) car.
     */
    bool IsRouteExhausted() const { return RouteNodes.Num() >= 2 && RouteCursor >= RouteNodes.Num() - 1; }

    /**
     * Place the car at the START of `StartSeg`, at arc-length offset `StartS`
     * (clamped into the lane), heading along the segment's travel direction, at
     * `InitialSpeed`. Returns false (and leaves the agent un-driveable, Segment()
     * == -1) when `StartSeg` is not a valid segment of `Net`.
     */
    bool StartOnSegment(const FRoadNetwork& Net, int32 StartSeg, double StartS, double InitialSpeed);

    /**
     * Advance one tick of `Dt` seconds. `LeaderGap` / `LeaderSpeed` describe the
     * nearest car ahead on THIS lane (pass OpenRoadGap / 0 for a clear road); the
     * caller computes them with FTrafficLane from the agents' arc-lengths. Updates
     * Speed (never negative) and the pose, crossing as many junctions as the
     * distance covers this frame (bounded, so a tiny lane can't spin forever).
     * A no-op when the agent never started or hit a true dead end.
     */
    void Step(const FRoadNetwork& Net, double Dt, double LeaderGap, double LeaderSpeed);

    /** Current pose (position + unit heading) in the network's XZ frame. */
    FLanePath::FPose Pose() const { return Lane.PoseAtDistance(S); }

    /** Current planar speed (>= 0), in the agent's speed units. */
    double GetSpeed() const { return Speed; }

    /** The segment the car is currently riding, or -1 before StartOnSegment / at a dead end. */
    int32 Segment() const { return Seg; }

    /** Arc-length along the current lane (metres from the lane start). */
    double Arc() const { return S; }

    /** Driveable: started, on a valid segment, not stranded at a dead end. */
    bool IsDriving() const { return Seg >= 0; }

private:
    /** Re-bake `Lane` from segment `NewSeg` of `Net` (straight offset lane). */
    void BakeLane(const FRoadNetwork& Net, int32 NewSeg);

    /**
     * At the end of the current segment, choose and switch to the next one. Prefers
     * a forward continuation (no U-turn); falls back to a U-turn only when the node
     * is a dead end. Returns false (and strands the agent, Seg = -1) when there is
     * no outgoing segment at all.
     */
    bool AdvanceToNextSegment(const FRoadNetwork& Net);

    int32 Seg = -1;
    FLanePath Lane;
    double S = 0.0;
    double Speed = 0.0;

    /** Optional planned node path; empty = free-roam. RouteCursor is the index of
     *  the next route node already consumed (search starts from here each junction). */
    TArray<int32> RouteNodes;
    int32 RouteCursor = 0;
};
