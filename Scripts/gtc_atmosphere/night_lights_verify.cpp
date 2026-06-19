// Headless verifier for FNightLights. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.World.NightLights invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Environment/NightLights.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
	const double Eps = 1e-4;
	const double On = 0.25, Off = 0.35;

	// Hysteresis
	{
		CHECK(FNightLights::ShouldBeLit(false, 0.0, On, Off), "dark turns on");
		CHECK(!FNightLights::ShouldBeLit(true, 1.0, On, Off), "day turns off");
		CHECK(FNightLights::ShouldBeLit(true, 0.30, On, Off), "band holds lit");
		CHECK(!FNightLights::ShouldBeLit(false, 0.30, On, Off), "band holds unlit");

		bool lit = true; int flips = 0;
		double samples[8] = { 0.28, 0.33, 0.26, 0.34, 0.30, 0.27, 0.32, 0.29 };
		for (int i = 0; i < 8; ++i) { bool n = FNightLights::ShouldBeLit(lit, samples[i], On, Off); if (n != lit) ++flips; lit = n; }
		CHECK(flips == 0, "no chatter inside the band");

		lit = false; flips = 0;
		for (int i = 0; i <= 20; ++i) { double Day = 1.0 - i * 0.05; bool n = FNightLights::ShouldBeLit(lit, Day, On, Off); if (n != lit) ++flips; lit = n; }
		CHECK(flips == 1, "dusk turns on exactly once");
		CHECK(lit, "lit by deep night");
	}

	// Jitter
	{
		double Base = 0.25, Spread = 0.08;
		CHECK(Absd(FNightLights::JitteredThreshold(Base, 0.5, Spread) - Base) < Eps, "mid seed = base");
		CHECK(Absd(FNightLights::JitteredThreshold(Base, 0.0, Spread) - (Base - Spread * 0.5)) < Eps, "low seed earliest");
		CHECK(Absd(FNightLights::JitteredThreshold(Base, 1.0, Spread) - (Base + Spread * 0.5)) < Eps, "high seed latest");
		CHECK(FNightLights::JitteredThreshold(0.0, 0.0, 1.0) >= 0.0, "clamped low");
		CHECK(FNightLights::JitteredThreshold(1.0, 1.0, 1.0) <= 1.0, "clamped high");
	}

	// Fraction + population
	{
		CHECK(Absd(FNightLights::LitFraction(0.0, On, Off) - 1.0) < Eps, "night all lit");
		CHECK(FNightLights::LitFraction(1.0, On, Off) < Eps, "day all dark");
		bool mono = true, bounded = true; double prev = 2.0;
		for (int i = 0; i <= 50; ++i) { double F = FNightLights::LitFraction(i * 0.02, On, Off); if (F > prev + Eps) mono = false; if (F < -Eps || F > 1.0 + Eps) bounded = false; prev = F; }
		CHECK(mono, "fraction monotone");
		CHECK(bounded, "fraction bounded");

		int N = 400, on = 0; double Day = 0.30;
		for (int i = 0; i < N; ++i)
		{
			double Seed = (i + 0.5) / N;
			double LOn = FNightLights::JitteredThreshold(On, Seed, 0.20);
			double LOff = FNightLights::JitteredThreshold(Off, Seed, 0.20);
			if (FNightLights::ShouldBeLit(false, Day, LOn, LOff)) ++on;
		}
		double frac = (double)on / N;
		CHECK(frac > 0.05 && frac < 0.95, "population partially lit mid-band");
		std::printf("  [info] mid-band population lit fraction %.3f\n", frac);
	}

	if (g_fail == 0) { std::printf("night_lights_verify: ALL PASS\n"); return 0; }
	std::printf("night_lights_verify: %d FAILED\n", g_fail);
	return 1;
}
