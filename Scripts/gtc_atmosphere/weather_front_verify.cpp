// Headless verifier for FWeatherFront. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.World.WeatherFront invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Environment/WeatherFront.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
	const double Eps = 1e-4;

	// Sweep
	{
		double Start = 0.0, Speed = 10.0;
		CHECK(Absd(FWeatherFront::FrontLineAt(Start, Speed, 5.0) - 50.0) < Eps, "front advances");
		CHECK(FWeatherFront::SignedDistance(100.0, Start, Speed, 0.0) > 0.0, "ahead positive");
		CHECK(FWeatherFront::SignedDistance(100.0, Start, Speed, 20.0) < 0.0, "behind negative");
		double Width = 30.0; bool mono = true; double prev = -1.0, first = 0.0, last = 0.0;
		for (int i = 0; i <= 40; ++i)
		{
			double T = i * 0.5;
			double S = FWeatherFront::SignedDistance(100.0, Start, Speed, T);
			double I = FWeatherFront::Intensity01(S, Width);
			if (I < prev - Eps) mono = false;
			if (i == 0) first = I;
			last = I; prev = I;
		}
		CHECK(mono, "intensity rises as front arrives");
		CHECK(first < 0.05, "starts clear");
		CHECK(last > 0.95, "ends weathered");
	}

	// Shape
	{
		double Width = 20.0;
		CHECK(FWeatherFront::Intensity01(100.0, Width) < Eps, "far ahead clear");
		CHECK(Absd(FWeatherFront::Intensity01(-100.0, Width) - 1.0) < Eps, "far behind full");
		CHECK(Absd(FWeatherFront::Intensity01(0.0, Width) - 0.5) < Eps, "line is half");
		bool mono = true; double prev = 2.0;
		for (int i = 0; i <= 40; ++i) { double S = -40.0 + i * 2.0; double I = FWeatherFront::Intensity01(S, Width); if (I > prev + Eps) mono = false; prev = I; }
		CHECK(mono, "intensity monotone in distance");
	}

	// Globals
	{
		FWorldGlobals Clear = FWeatherFront::Globals(0.0, 1.0);
		CHECK(Clear.CloudCoverage < Eps && Clear.RainIntensity < Eps && Clear.Wetness < Eps && Clear.NightAmount < Eps, "clear noon all zero");
		FWorldGlobals Storm = FWeatherFront::Globals(1.0, 1.0, 0.5);
		CHECK(Absd(Storm.CloudCoverage - 1.0) < Eps, "storm full cloud");
		CHECK(Storm.RainIntensity > 0.9 && Storm.Wetness > 0.9, "storm raining wet");
		CHECK(Absd(Storm.NightAmount - 0.5) < Eps, "storm darkens midday");
		FWorldGlobals Overcast = FWeatherFront::Globals(0.5, 1.0);
		CHECK(Overcast.CloudCoverage > 0.4 && Overcast.RainIntensity < Eps, "overcast precedes rain");
		FWorldGlobals Night = FWeatherFront::Globals(0.0, 0.0);
		CHECK(Absd(Night.NightAmount - 1.0) < Eps, "clear night dark");
	}

	if (g_fail == 0) { std::printf("weather_front_verify: ALL PASS\n"); return 0; }
	std::printf("weather_front_verify: %d FAILED\n", g_fail);
	return 1;
}
