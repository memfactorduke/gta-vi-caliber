// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Driver INPUT shaping — the seam between Enhanced Input and a Chaos wheeled
 * vehicle. Raw axes (a steer axis in [-1,1] and two trigger pedals in [0,1])
 * come in; clean throttle / brake / steer values the movement component can take
 * come out. This is the missing half FVehicleHandling assumed: that one models
 * the chassis FEEL once it's moving, this decides what the player is actually
 * asking the chassis to do.
 *
 * What it shapes:
 *   - Steering deadzone + rescale (stick/keyboard noise near centre → true zero).
 *   - Speed-sensitive steering authority (full lock parking, calmer at speed so
 *     the car doesn't dart) — the classic arcade stability trick.
 *   - Steering smoothing toward a target (frame-rate-independent interp) so taps
 *     don't snap the wheels.
 *   - Pedal resolution with auto-reverse: the brake trigger brakes while rolling
 *     forward, but becomes reverse drive once the car is at/below a crawl.
 *
 * PURE-CORE: double precision, scene-free, deterministic, no engine coupling.
 * Speeds are the adapter's units (cm/s assumed in the named constants, but the
 * math is scale-free). The Car adapter calls SteerTarget()+SteerInterp() each
 * frame and feeds the result to SetSteeringInput, and ResolvePedals() to drive
 * SetThrottleInput / SetBrakeInput (+ a reverse gear when bReverse). Unit-tested
 * headless: Vehicles/Drive/Tests/VehicleDriveInputTest.cpp, prefix
 * GTC.Vehicles.DriveInput.
 */
struct GTC_UE5_API FVehicleDriveInput
{
    /** Steer-axis magnitude below which input reads as centred (no steering). */
    static constexpr double SteerDeadzone = 0.05;

    /** At/under this speed the player has full steering authority (parking lock). */
    static constexpr double SteerFullAuthoritySpeed = 800.0;   // ~29 km/h
    /** At/over this speed steering authority bottoms out at MinSteerAuthority. */
    static constexpr double SteerMinAuthoritySpeed = 6000.0;   // ~216 km/h
    /** Floor on steering authority so the car always responds, even flat out. */
    static constexpr double MinSteerAuthority = 0.35;

    /** Frame-rate-independent steering interp rate (toward the shaped target). */
    static constexpr double SteerInterpRate = 5.0;

    /** Forward speed at/under which a brake request engages reverse instead. */
    static constexpr double ReverseEngageSpeed = 50.0;         // ~1.8 km/h

    /** Resolved pedal state for the movement component. */
    struct FDriveOutput
    {
        double Throttle = 0.0;   // forward (or reverse, when bReverse) drive, 0..1
        double Brake = 0.0;      // service brake, 0..1
        bool bReverse = false;   // true => engage reverse gear; Throttle drives backward
    };

    /** Deadzone + rescale a raw axis to [-1,1]: 0 inside the deadzone, smoothly to ±1. */
    static double ApplyDeadzone(double Raw, double Deadzone);

    /** Steering authority multiplier (MinSteerAuthority..1) at a given speed. */
    static double SteerAuthority(double SpeedCmS);

    /** Shaped steering target: deadzoned steer scaled by speed-sensitive authority, [-1,1]. */
    static double SteerTarget(double RawSteer, double SpeedCmS);

    /** Smooth Current toward Target at SteerInterpRate (engine FInterpTo semantics). */
    static double SteerInterp(double Current, double Target, double Dt);

    /**
     * Resolve the two trigger pedals into throttle/brake/reverse given signed
     * forward speed (velocity along the car's forward axis). Brake trigger brakes
     * while rolling forward; once at/below ReverseEngageSpeed and the gas isn't
     * pressed, it becomes reverse drive. Inputs are clamped to their valid ranges.
     */
    static FDriveOutput ResolvePedals(double RawAccel, double RawBrake, double ForwardSpeedCmS);
};
