// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A train running a line — the on-rails mover for the metro/freight that gives a
 * city its background motion. Unlike the free-roaming car/boat/aircraft cores, a
 * train is 1-DOF: it lives at a distance along a looping track, and "driving it
 * well" is one constraint — STOP EXACTLY at the next station, dwell, pull away,
 * repeat. FTrainMover is that controller: distance along the line plus a speed,
 * advancing itself station to station.
 *
 * The stopping is the whole trick. Instead of a hand-tuned "start braking here"
 * trigger, the controller caps its speed by the braking distance to the next stop:
 * vCap = sqrt(2 * Brake * distanceToStation). Far from the platform that cap is huge
 * so it cruises at LineSpeed; as the platform nears the cap squeezes the speed down
 * to zero right at the stop. It then dwells DwellSeconds and advances to the next
 * station, wrapping around the loop. Acceleration and braking are rate-limited so
 * the motion is smooth.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * a 1-D position/speed integrator; the adapter maps distance-along-track to a world
 * transform on the spline. Unit-tested headless (Tests/TrainMoverTest.cpp, prefix
 * GTC.Vehicles.Rail).
 *
 * PURE-CORE boundary: placing the track spline, mapping Position to a world
 * transform, the door/announcement timing during dwell, and carriage articulation
 * is the DEFERRED adapter and is NOT covered by the tests. Units: positions/distances
 * cm, speeds cm/s, accelerations cm/s^2, Dt and DwellSeconds seconds.
 */
struct FTrainMover
{
    /** Line tuning. */
    struct FParams
    {
        /** Total loop length of the line (cm); the train wraps at this distance. */
        double TrackLength = 100000.0;
        /** Top cruising speed (cm/s). */
        double LineSpeed = 2000.0;
        /** Acceleration when pulling away / speeding up (cm/s^2). */
        double Accel = 300.0;
        /** Braking deceleration into a station (cm/s^2). */
        double Brake = 400.0;
        /** Seconds stopped at each station. */
        double DwellSeconds = 5.0;
    };

    /**
     * Install the line. `InStations` are distances (cm) along the loop in increasing
     * order; the train serves them in order and wraps. Starts stopped at distance 0,
     * heading toward the first station ahead. An empty station list makes the train
     * simply cruise the loop at LineSpeed (no stops).
     */
    void Configure(const FParams& InParams, const TArray<double>& InStations);

    /**
     * Advance `Dt` seconds: while dwelling, count down then depart to the next
     * station; while running, chase the braking-distance speed cap toward the next
     * stop, move, and arrive (snap + dwell) on reaching it. A negative Dt is ignored.
     */
    void Advance(double Dt);

    /** Distance along the loop (cm), in [0, TrackLength). */
    double Position() const { return Pos; }

    /** Current speed (cm/s). */
    double Speed() const { return Vel; }

    /** True while stopped at a station for its dwell. */
    bool IsDwelling() const { return bDwelling; }

    /** Index of the station the train is currently heading for (or stopped at), or INDEX_NONE if none. */
    int32 TargetStation() const { return Stations.Num() > 0 ? Target : INDEX_NONE; }

    /** Forward distance (cm) along the loop to the next stop (0 when no stations). */
    double DistanceToNextStop() const;

private:
    double ForwardDistanceTo(double TargetPos) const;

    FParams Params;
    TArray<double> Stations;
    double Pos = 0.0;
    double Vel = 0.0;
    int32 Target = 0;
    bool bDwelling = false;
    double DwellTimer = 0.0;
};
