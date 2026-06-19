// Headless verifier for FOceanShore. Compiles the REAL core .cpp against the shim
// and re-asserts the GTC.World.Ocean.Shore invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Environment/OceanShore.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
	const double Eps = 1e-4;

	// Blend + depth + color fade
	{
		CHECK(Absd(FOceanShore::WaterDepth(0.3, -1.0) - 1.3) < Eps, "depth subtracts seabed");
		CHECK(FOceanShore::WaterDepth(-0.2, 0.5) < 0.0, "dry land negative depth");

		CHECK(FOceanShore::ShoreBlend01(-0.5, 2.0) < Eps, "blend dry 0");
		CHECK(FOceanShore::ShoreBlend01(0.0, 2.0) < Eps, "blend waterline 0");
		CHECK(Absd(FOceanShore::ShoreBlend01(2.0, 2.0) - 1.0) < Eps, "blend deep 1");
		bool mono = true, bounded = true; double prev = -1.0;
		for (int i = 0; i <= 40; ++i) { double B = FOceanShore::ShoreBlend01(i * 0.1, 2.0); if (B < prev - Eps) mono = false; if (B < -Eps || B > 1.0 + Eps) bounded = false; prev = B; }
		CHECK(mono, "blend monotone");
		CHECK(bounded, "blend bounded");

		CHECK(FOceanShore::DepthColorFade01(0.0, 3.0) < Eps, "color fade shore 0");
		CHECK(FOceanShore::DepthColorFade01(30.0, 3.0) > 0.99, "color fade deep near 1");
		CHECK(FOceanShore::DepthColorFade01(5.0, 3.0) > FOceanShore::DepthColorFade01(1.0, 3.0), "color fade monotone");
	}

	// Foam band
	{
		double FD = 1.0; double peak = 0.0; bool bounded = true;
		CHECK(FOceanShore::ShoreFoam01(-0.3, FD) < Eps, "foam dry 0");
		CHECK(FOceanShore::ShoreFoam01(0.0, FD) < Eps, "foam waterline 0");
		CHECK(FOceanShore::ShoreFoam01(2.0, FD) < Eps, "foam deep 0");
		for (int i = 0; i <= 40; ++i) { double F = FOceanShore::ShoreFoam01(i * 0.05, FD); peak = std::fmax(peak, F); if (F < -Eps || F > 1.0 + Eps) bounded = false; }
		CHECK(peak > 0.5, "foam has a peak");
		CHECK(bounded, "foam bounded");
		std::printf("  [info] shore foam peak %.3f\n", peak);
	}

	// Wet sand
	{
		double S = 0.5, B = 0.4;
		CHECK(Absd(FOceanShore::WetSand01(0.0, S, B) - 1.0) < Eps, "below swash wet");
		CHECK(Absd(FOceanShore::WetSand01(S, S, B) - 1.0) < Eps, "at swash wet");
		CHECK(FOceanShore::WetSand01(S + B, S, B) < Eps, "above band dry");
		bool mono = true; double prev = 2.0;
		for (int i = 0; i <= 40; ++i) { double W = FOceanShore::WetSand01(i * 0.05, S, B); if (W > prev + Eps) mono = false; prev = W; }
		CHECK(mono, "wetness falls up the beach");
		CHECK(FOceanShore::WetSand01(0.7, 0.9, B) > FOceanShore::WetSand01(0.7, 0.5, B) + Eps, "bigger run-up wets higher");
	}

	if (g_fail == 0) { std::printf("ocean_shore_verify: ALL PASS\n"); return 0; }
	std::printf("ocean_shore_verify: %d FAILED\n", g_fail);
	return 1;
}
