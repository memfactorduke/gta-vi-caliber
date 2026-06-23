// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The numbers a speedo HUD shows while you DRIVE — turn a car's measured planar speed
 * and its gearbox state into a ready-to-paint readout: speed in km/h AND mph, a needle
 * position 0..1 against a configurable max, an RPM fraction 0..1 against redline with a
 * redline-flash flag, and a short gear label (P/R/N or the 1..N forward gear).
 *
 * This composes with FVehicleTransmission (or any equivalent gearbox) rather than
 * re-deriving any gearbox logic: the adapter reads Gear()/Rpm()/ForwardGearNumber() off the
 * transmission and hands those scalars straight in here to be PRESENTED. We deliberately take plain
 * inputs (an EGear ordinal, a forward-gear number, an rpm + redline) instead of including
 * the transmission header, mirroring how FVehicleHandling consumes FVehicleCondition's
 * GripFactor()/TopSpeedFactor() as bare doubles — the math stays decoupled and scale-free.
 *
 * GTC-original pure-core: non-UObject, double precision, scene-free, deterministic — no
 * Slate, no engine context. Unit-tested headless (Vehicles/Speedometer/Tests/
 * VehicleSpeedometerTest.cpp, prefix GTC.Vehicles.Speedometer).
 *
 * Speeds are in the adapter's own measured units (e.g. cm/s, as the Chaos chassis / the
 * handling layer report) — the conversion constants below are written for cm/s in, and
 * the needle's MaxSpeed must be supplied in those same units so the fraction is unitless.
 *
 * PURE-CORE boundary: sampling the live chassis speed, reading the FVehicleTransmission,
 * and painting the dial/needle/RPM widget is the DEFERRED adapter and is NOT covered by
 * the tests.
 */
struct GTC_UE5_API FVehicleSpeedometer
{
    /** The gear-selector position, matching the transmission's selector ordinal 1:1. */
    enum class EGear : uint8
    {
        Park,
        Reverse,
        Neutral,
        Drive,  // a forward gear; the specific 1..N number is carried separately
    };

    /** Multiply a cm/s speed by this to read km/h ((cm/s)/100000 * 3600). */
    static constexpr double CmPerSecToKmPerHour = 0.036;
    /** Multiply a km/h speed by this to read mph (1 mile == 1.609344 km). */
    static constexpr double KmPerHourToMilesPerHour = 1.0 / 1.609344;

    /** A measured speed in the adapter's units, expressed for the dial. */
    static double SpeedKmPerHour(double SpeedUnitsPerSec);
    static double SpeedMilesPerHour(double SpeedUnitsPerSec);

    /**
     * Needle fill 0..1 = |speed| / MaxSpeed (both in the adapter's units), clamped to
     * [0,1]. Magnitude is used so reversing still sweeps the needle. A zero/negative
     * MaxSpeed is degenerate and yields 0 (a flat needle, never a divide blow-up).
     */
    static double NeedleFraction(double SpeedUnitsPerSec, double MaxSpeedUnitsPerSec);

    /**
     * Tachometer fill 0..1 = Rpm / RedlineRpm, clamped to [0,1]. A zero/negative redline
     * is degenerate and yields 0. Negative rpm clamps to 0 (the dial never reads below
     * idle-zero).
     */
    static double RpmFraction(double Rpm, double RedlineRpm);

    /**
     * Whether the redline-flash warning should light: the engine is at or past redline.
     * True when Rpm >= RedlineRpm AND RedlineRpm is positive (a degenerate redline never
     * latches the warning on). Equivalently, RpmFraction has saturated to 1 at a real
     * redline.
     */
    static bool IsRedlineFlashing(double Rpm, double RedlineRpm);

    /**
     * The short gear label a dial shows: "P", "R", "N", or the forward-gear NUMBER as a
     * string ("1".."N") when in Drive. ForwardGearNumber (the transmission's
     * ForwardGearNumber()) is only consulted in Drive and is clamped to at least 1 so a
     * not-yet-engaged "0" still reads as "1"; P/R/N ignore it.
     */
    static FString GearLabel(EGear Gear, int32 ForwardGearNumber);
};
