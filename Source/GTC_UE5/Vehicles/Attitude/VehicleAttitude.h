// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Visual attitude for the kinematic flight pawns — how the mesh leans, separate from the
 * flight cores that own velocity/heading. Two cues sell flight: an aircraft BANKS into a
 * turn, and its nose PITCHES to follow the flight path (up in a climb, down in a dive).
 * FVehicleAttitude is that trig, capped so the craft never rolls or pitches past a sane
 * limit. (Boats take their pitch/roll from the buoyancy/float pose instead; helicopters
 * may drive a small forward/side tilt from cyclic — this is mainly the fixed-wing look.)
 *
 * BankAngle uses the coordinated-turn relation: lateral accel v*omega against gravity, so
 * bank is zero at a standstill or flying straight and grows with both speed and turn rate.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * stateless static helpers (the FVehicleChaseCamera shape). Units: angles radians, speeds
 * cm/s, yaw rate rad/s. Unit-tested headless
 * (Vehicles/Attitude/Tests/VehicleAttitudeTest.cpp, prefix GTC.Vehicles.Attitude).
 */
struct GTC_UE5_API FVehicleAttitude
{
    /** Gravity (cm/s^2) used in the coordinated-turn bank. */
    static constexpr double GravityCmS2 = 980.0;

    /**
     * Bank (roll) angle into a turn: atan2(Speed*YawRate, g) * Gain, clamped to
     * +/- MaxRoll. Zero at a standstill (Speed 0) or wings-level (YawRate 0); sign
     * follows the turn direction; `Gain` exaggerates for arcade feel.
     */
    static double BankAngle(double YawRateRad, double SpeedCmS, double Gain, double MaxRollRad);

    /**
     * Nose pitch following the velocity vector: atan2(ClimbRate, HorizSpeed), clamped to
     * +/- MaxPitch. Positive (nose up) climbing, negative diving, 0 level. A near-vertical
     * climb clamps to MaxPitch rather than snapping to 90 degrees.
     */
    static double PitchFromVelocity(double ClimbRateCmS, double HorizSpeedCmS, double MaxPitchRad);
};
