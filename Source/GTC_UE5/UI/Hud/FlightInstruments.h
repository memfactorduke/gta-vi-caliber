// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Cockpit/HUD unit conversions for the air + sea vehicles — the small, exact arithmetic
 * that turns the sim's raw cm/s and cm into the numbers a player reads on an altimeter,
 * airspeed indicator and vertical-speed gauge (and a boat's speedo). The flight cores work
 * in cm/s and cm; instruments speak knots, feet, feet-per-minute and km/h. Keeping the
 * conversions here (pure + tested) means the HUD widget is a thin formatter and the factors
 * can't drift between gauges.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * stateless static helpers, no engine math needed. Signed throughout (a negative climb is
 * a descent / negative ft-min). Unit-tested headless
 * (UI/Hud/Tests/FlightInstrumentsTest.cpp, prefix GTC.UI.FlightInstruments).
 */
struct GTC_UE5_API FFlightInstruments
{
    /** Centimetres per international foot. */
    static constexpr double CmPerFoot = 30.48;
    /** cm/s in one knot (1852 m/h). */
    static constexpr double CmSPerKnot = 185200.0 / 3600.0;
    /** cm/s in one km/h. */
    static constexpr double CmSPerKmh = 100000.0 / 3600.0;

    /** Airspeed in knots from a cm/s speed. */
    static double Knots(double SpeedCmS);

    /** Ground/water speed in km/h from a cm/s speed (boat speedo). */
    static double Kmh(double SpeedCmS);

    /** Altitude in feet from a cm height. */
    static double Feet(double HeightCm);

    /** Vertical speed in feet-per-minute from a cm/s climb rate (signed: descent < 0). */
    static double FeetPerMin(double ClimbRateCmS);
};
