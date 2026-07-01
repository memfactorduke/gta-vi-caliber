// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FVehicleDriveResolver — the pure, STATELESS input-shaping half of the drive-command pipeline.
 * FVehicleDriveInput exposes the shaping math (steering deadzone + speed-sensitive authority +
 * frame-rate-independent smoothing, and pedal resolution with auto-reverse) but leaves "reading the
 * Enhanced Input axes and pushing the resulting scalars into the movement component" to the deferred
 * Car adapter. This composes those pieces into one Shape() call so the UObject shell can hand the
 * result straight to the stateful FVehicleTransmission.
 *
 * The steering smoother needs last frame's value, so the shell carries PrevSteer (the resolver
 * itself stays pure). ForwardSpeed's SIGN drives auto-reverse (brake becomes reverse at a crawl),
 * and the resolved direction is folded into GearboxThrottle — the signed throttle the transmission
 * uses to pick Drive vs Reverse at rest.
 *
 * Units: speeds in cm/s (the shaping constants assume cm/s); Dt in seconds; all control signals
 * dimensionless.
 */
struct GTC_UE5_API FVehicleDriveResolver
{
    struct FInput
    {
        double RawSteer = 0.0;        // [-1,1] Enhanced Input steer axis
        double RawThrottle = 0.0;     // [0,1] accelerator trigger
        double RawBrake = 0.0;        // [0,1] brake trigger
        double SpeedCmS = 0.0;        // |ground speed| for steering authority
        double ForwardSpeedCmS = 0.0; // signed forward speed for pedal auto-reverse
        double PrevSteer = 0.0;       // last frame's smoothed steer (shell-held state)
        double Dt = 0.0;
    };

    struct FOutput
    {
        double Steer = 0.0;           // smoothed, shaped steer [-1,1] (store as next PrevSteer)
        double Throttle = 0.0;        // 0..1 drive demand (in the resolved direction)
        double Brake = 0.0;           // 0..1 service brake
        bool bReverse = false;        // auto-reverse engaged (throttle drives backward)
        double GearboxThrottle = 0.0; // signed throttle for FVehicleTransmission (- when reversing)
    };

    /** Shape one frame of raw input into transmission-ready commands. Pure + deterministic. */
    static FOutput Shape(const FInput& In);

    /**
     * Driven-wheel RPM (rev/min) for a car rolling at ForwardSpeedCmS on wheels of WheelRadiusCm:
     * ForwardSpeed / wheel-circumference, per minute. Signed (forward speed -> positive RPM). The
     * shell uses it both to estimate wheel RPM from speed and to convert a linear speed into the
     * gearbox's standstill-RPM units (so the reverse-engage speed and the standstill threshold line
     * up). Radius is floored at 1 cm.
     */
    static double WheelRpmFromForwardSpeed(double ForwardSpeedCmS, double WheelRadiusCm);
};
