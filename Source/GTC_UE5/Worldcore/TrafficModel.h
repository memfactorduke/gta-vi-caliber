// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free car-following model for the worldcore traffic sim — the
 * longitudinal brain of a vehicle. Given the car's speed, the gap to the car
 * ahead, and that leader's speed, return the acceleration that cruises toward a
 * desired speed on a clear road while keeping a safe time-headway gap (and
 * braking hard if the gap collapses). This is the Intelligent Driver Model (IDM).
 *
 * Direct C++->C++ port of the worldcore core
 * engine/src/worldcore/traffic_core.h (namespace worldcore_traffic). All static,
 * scalar-in/scalar-out, no UObject — unit-tested headless via the parity oracle
 * (Tests/TrafficModelTest.cpp, prefix GTC.Worldcore.TrafficModel).
 *
 * Double precision throughout, to match the C++ oracle's double math.
 *
 * PURE-CORE boundary: this is the pure longitudinal acceleration math only. Mass
 * / actor integration (applying this acceleration to a vehicle each tick) is a
 * DEFERRED Wave-3 adapter — the core stays pure, operating on caller-supplied
 * agent state, and is NOT covered by the parity tests.
 */
struct GTC_UE5_API FTrafficModel
{
    /**
     * IDM acceleration.
     *   Speed         current speed (m/s)
     *   Gap           bumper-to-bumper distance to the leader (m)
     *   LeaderSpeed   leader's speed (m/s)
     *   DesiredSpeed  v0 — free-road cruising speed
     *   MaxAccel      a  — comfortable acceleration
     *   ComfortDecel  b  — comfortable braking (positive)
     *   MinGap        s0 — standstill distance
     *   TimeHeadway   T  — desired time gap to the leader
     */
    static double CarFollowingAccel(double Speed, double Gap, double LeaderSpeed,
        double DesiredSpeed, double MaxAccel, double ComfortDecel, double MinGap,
        double TimeHeadway);
};
