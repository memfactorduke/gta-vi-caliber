// Headless verifier for FFlightInstruments. Compiles the REAL core .cpp against the shim.
#include <cstdio>

#include "../../Source/GTC_UE5/UI/Hud/FlightInstruments.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    const double Eps = 1e-6;

    // Knots: one knot is 185200/3600 cm/s.
    CHECK(Absd(FFlightInstruments::Knots(FFlightInstruments::CmSPerKnot) - 1.0) < Eps, "one knot");
    CHECK(Absd(FFlightInstruments::Knots(FFlightInstruments::CmSPerKnot * 100.0) - 100.0) < Eps, "100 knots");
    CHECK(Absd(FFlightInstruments::Knots(0.0)) < Eps, "zero speed -> 0 knots");

    // Km/h: 1000 cm/s = 36 km/h.
    CHECK(Absd(FFlightInstruments::Kmh(1000.0) - 36.0) < Eps, "1000 cm/s -> 36 km/h");

    // Feet: 3048 cm = 100 ft.
    CHECK(Absd(FFlightInstruments::Feet(3048.0) - 100.0) < Eps, "3048 cm -> 100 ft");

    // Feet per minute: 30.48 cm/s climb = 60 ft/min; descent is signed.
    CHECK(Absd(FFlightInstruments::FeetPerMin(30.48) - 60.0) < Eps, "30.48 cm/s -> 60 ft/min");
    CHECK(Absd(FFlightInstruments::FeetPerMin(-30.48) - (-60.0)) < Eps, "descent -> negative ft/min");
    CHECK(Absd(FFlightInstruments::FeetPerMin(0.0)) < Eps, "level -> 0 ft/min");

    if (g_fail == 0) { std::printf("flight_instruments_verify: ALL PASS\n"); return 0; }
    std::printf("flight_instruments_verify: %d FAILED\n", g_fail);
    return 1;
}
