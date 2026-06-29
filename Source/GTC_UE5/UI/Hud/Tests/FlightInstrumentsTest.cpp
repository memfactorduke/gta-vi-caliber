// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FlightInstruments.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FFlightInstruments — cm/s and cm to knots, km/h, feet, feet-per-minute.
 * GTC-original. Mirrors Scripts/gtc_aircraft/flight_instruments_verify.cpp.
 * Prefix GTC.UI.FlightInstruments.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFlightInstrumentsTest,
    "GTC.UI.FlightInstruments",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlightInstrumentsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("one knot"), FFlightInstruments::Knots(FFlightInstruments::CmSPerKnot), 1.0, Eps);
    TestEqual(TEXT("100 knots"), FFlightInstruments::Knots(FFlightInstruments::CmSPerKnot * 100.0), 100.0, Eps);
    TestEqual(TEXT("zero -> 0 knots"), FFlightInstruments::Knots(0.0), 0.0, Eps);

    TestEqual(TEXT("1000 cm/s -> 36 km/h"), FFlightInstruments::Kmh(1000.0), 36.0, Eps);

    TestEqual(TEXT("3048 cm -> 100 ft"), FFlightInstruments::Feet(3048.0), 100.0, Eps);

    TestEqual(TEXT("30.48 cm/s -> 60 ft/min"), FFlightInstruments::FeetPerMin(30.48), 60.0, Eps);
    TestEqual(TEXT("descent -> negative ft/min"), FFlightInstruments::FeetPerMin(-30.48), -60.0, Eps);
    TestEqual(TEXT("level -> 0 ft/min"), FFlightInstruments::FeetPerMin(0.0), 0.0, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
