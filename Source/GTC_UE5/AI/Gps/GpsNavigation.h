// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure GPS / route-progress math for the minimap navigation line — the "blue
 * line to the waypoint" half of open-world-style street nav. Given a precomputed
 * polyline route (an array of FVector waypoints, the last being the
 * destination), it answers progress / distance / ETA / next-turn questions for
 * a player somewhere along it.
 *
 * Direct port of the the reference `GpsNavigation` (RefCounted) at
 * `game/scripts/ai/gps_navigation.gd`. All static, FVector-in, scalar/struct-out,
 * no UObject — unit-tested headless via the reference behavior
 * (Tests/GpsNavigationTest.cpp, prefix GTC.AI.Gps).
 *
 * Double precision throughout, to match the the reference implementation float math. Work happens in
 * the XZ plane (the reference Y-up); `Ground(V)` flattens to `FVector(V.X, 0, V.Z)`.
 * Defensive throughout — empty or single-point routes and zero speed never
 * produce NaN/INF surprises (ETA is the one deliberate +infinity).
 *
 * NOTE: no Godot->UE Z-up axis remap is baked in here — the model stays in the
 * the reference XZ frame so the ported unit tests match the oracle bit-for-bit. Porting
 * the axis convention to UE's Z-up space is a DEFERRED Wave-3 concern.
 *
 * PURE-CORE boundary: this is NOT pathfinding. Recast/navmesh path GENERATION is
 * the deferred Wave-3 adapter — the model only CONSUMES a caller-supplied
 * polyline. Route generation is NOT implemented here and NOT covered by the
 * parity tests.
 */

/**
 * Which way the route bends at a turn waypoint. Positive signed turn (XZ, Y-up)
 * is a Left turn, negative is Right — matching the Godot "left"/"right" strings.
 */
enum class ETurnDirection : uint8
{
    Left,
    Right,
};

struct GTC_UE5_API FGpsNavigation
{
    /** Planar tolerance mirroring the the reference EPS (degenerate segment / zero speed). */
    static constexpr double Eps = 1e-4;

    /**
     * The next waypoint where the route bends by more than the turn threshold.
     * Mirrors the the reference `{position, distance, direction}` dictionary; an empty
     * the reference `{}` (no upcoming turn) maps to bHasTurn == false.
     */
    struct FNextTurn
    {
        bool bHasTurn = false;
        FVector Position = FVector::ZeroVector;
        double Distance = 0.0;
        ETurnDirection Direction = ETurnDirection::Left;
    };

    /** Drop the vertical component — navigation reasons on the ground (XZ) plane. */
    static FVector Ground(const FVector& V);

    /**
     * Total length of the route polyline (sum of planar segment lengths). 0 for an
     * empty or single-point route.
     */
    static double RouteLength(const TArray<FVector>& Route);

    /**
     * Index of the route segment the player is currently on — the segment whose
     * nearest point to Pos is closest (strict <, first wins ties). Segment i runs
     * Route[i] -> Route[i + 1], so the index is in [0, Route.Num() - 2]. Returns 0
     * when the route is too short to have a segment.
     */
    static int32 NearestSegment(const FVector& Pos, const TArray<FVector>& Route);

    /**
     * Along-route distance from the player's projected position to the destination:
     * the leftover of the current segment past the projection, plus every later
     * segment. NOT straight-line. 0 for a degenerate route.
     */
    static double DistanceRemaining(const FVector& Pos, const TArray<FVector>& Route);

    /**
     * Fraction of the route completed, 0..1. 0 at the start, 1 at the destination.
     * A degenerate route reports 1.0 (nothing left to travel, no divide-by-zero).
     */
    static double Progress(const FVector& Pos, const TArray<FVector>& Route);

    /**
     * Estimated travel time = DistanceRemaining / Speed, in seconds. A non-positive
     * speed (a stopped player has no finite ETA) returns true +infinity. Tests
     * assert the guard via !FMath::IsFinite.
     */
    static double EtaSeconds(const FVector& Pos, const TArray<FVector>& Route, double Speed);

    /**
     * The next waypoint where the route bends by more than TurnThresholdRadians.
     * Distance is the along-route distance from the player to that bend; Direction
     * is the signed turn (XZ, Y-up). Returns bHasTurn == false when the route runs
     * near-straight to the destination from here.
     */
    static FNextTurn NextTurn(const FVector& Pos, const TArray<FVector>& Route, double TurnThresholdRadians);

    /**
     * Whether the player has reached the destination — within ArriveRadius of the
     * LAST waypoint (planar distance). Always false for an empty route.
     */
    static bool HasArrived(const FVector& Pos, const TArray<FVector>& Route, double ArriveRadius);

    /**
     * Normalized heading the player should follow right now: along the current
     * segment toward its end. Zero vector for a degenerate route (no direction to
     * give).
     */
    static FVector DirectionToNext(const FVector& Pos, const TArray<FVector>& Route);

private:
    /**
     * Parameter t in [0, 1] of P projected onto segment A->B (clamped to the
     * segment). 0 when the segment has no length.
     */
    static double SegmentT(const FVector& P, const FVector& A, const FVector& B);

    /** Closest point on segment A->B to P, clamped to the segment endpoints. */
    static FVector ProjectPoint(const FVector& P, const FVector& A, const FVector& B);

    /**
     * Signed turn angle (radians) from Incoming to Outgoing in the XZ plane.
     * Positive = left turn, negative = right (Y-up). Uses the perpendicular dot
     * for the sign and Atan2 for a stable magnitude.
     */
    static double SignedTurn(const FVector& Incoming, const FVector& Outgoing);

    /**
     * Along-route distance from the player to waypoint index Target, where the
     * player projects onto segment Seg. Sums the leftover of the current segment
     * plus whole segments up to Target.
     */
    static double DistanceToWaypoint(const FVector& Pos, const TArray<FVector>& Route, int32 Seg, int32 Target);
};
