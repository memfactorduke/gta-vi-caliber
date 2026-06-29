// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlightInstruments.h"

double FFlightInstruments::Knots(double SpeedCmS)
{
    return SpeedCmS / CmSPerKnot;
}

double FFlightInstruments::Kmh(double SpeedCmS)
{
    return SpeedCmS / CmSPerKmh;
}

double FFlightInstruments::Feet(double HeightCm)
{
    return HeightCm / CmPerFoot;
}

double FFlightInstruments::FeetPerMin(double ClimbRateCmS)
{
    // cm/s -> cm/min -> ft/min
    return ClimbRateCmS * 60.0 / CmPerFoot;
}
