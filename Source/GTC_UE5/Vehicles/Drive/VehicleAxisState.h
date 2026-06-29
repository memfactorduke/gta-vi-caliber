// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Persistent (held-setting) control axes for the flight pawns. A car pedal springs back
 * to zero, but a helicopter's COLLECTIVE and a plane's THROTTLE are settings you nudge
 * up and down and they STAY — let go of the stick and the lever holds. FVehicleAxisState
 * is that integration: take the current setting and a signed raise/lower input, move it
 * by a rate per second, clamp to its range. It's the one genuinely new bit of input math
 * the feature needs; momentary axes (boat throttle, cyclic, steer) pass straight through.
 *
 * Pairs with FVehicleDriveInput::ApplyDeadzone upstream (clean the raw stick first), and
 * the FParams-free, stateless shape matches FVehicleChaseCamera.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject. Units:
 * the value and Lo/Hi are dimensionless 0..1 control settings by default, RatePerSec in
 * units/second, Dt seconds. Unit-tested headless
 * (Vehicles/Drive/Tests/VehicleAxisStateTest.cpp, prefix GTC.Vehicles.AxisState).
 */
struct GTC_UE5_API FVehicleAxisState
{
    /**
     * Move a held control toward its limits. `Current` is the present setting;
     * `RaiseLower` (clamped -1..1, e.g. collective-up minus collective-down) is the
     * signed direction; `RatePerSec` is how fast a full-deflection input moves it;
     * `Dt` (clamped >= 0) is the frame time. The result is clamped to [Lo, Hi]
     * (Lo/Hi swapped if passed inverted). A non-positive Dt returns Current unchanged.
     */
    static double Integrate(double Current, double RaiseLower, double RatePerSec, double Dt,
        double Lo = 0.0, double Hi = 1.0);
};
