// Headless verifier for FSurfaceWetness. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.World.Wetness invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Environment/SurfaceWetness.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
	const double Eps = 1e-4;
	const double Dt = 0.1;

	// Rise / decay
	{
		double OneStep = FSurfaceWetness::Step(0.0, 1.0, Dt);
		CHECK(OneStep > 0.0 && OneStep < 1.0, "wetness lags rain (no snap)");

		double W = 0.0; bool rising = true; double prev = -1.0;
		for (int i = 0; i < 200; ++i) { W = FSurfaceWetness::Step(W, 1.0, Dt); if (W < prev - Eps) rising = false; prev = W; }
		CHECK(rising, "rises under rain");
		CHECK(W > 0.95, "approaches soaked");

		bool falling = true; prev = 2.0;
		for (int i = 0; i < 400; ++i) { W = FSurfaceWetness::Step(W, 0.0, Dt); if (W > prev + Eps) falling = false; prev = W; }
		CHECK(falling, "dries when rain stops");
		CHECK(W < 0.05, "approaches dry");
	}

	// Asymmetry
	{
		int wet = 0; double W = 0.0;
		while (W < 0.5 && wet < 100000) { W = FSurfaceWetness::Step(W, 1.0, Dt); ++wet; }
		int dry = 0; W = 1.0;
		while (W > 0.5 && dry < 100000) { W = FSurfaceWetness::Step(W, 0.0, Dt); ++dry; }
		CHECK(dry > wet, "drying slower than wetting");
		std::printf("  [info] wet steps %d, dry steps %d\n", wet, dry);
	}

	// Converge
	{
		CHECK(Absd(FSurfaceWetness::Equilibrium(0.5) - 0.5) < Eps, "equilibrium tracks rain");
		double W = 0.0;
		for (int i = 0; i < 2000; ++i) W = FSurfaceWetness::Step(W, 0.5, Dt);
		CHECK(Absd(W - 0.5) < 1e-2, "converges to drizzle level");
		bool bounded = true; W = 0.3;
		for (int i = 0; i < 500; ++i) { double Rain = (i % 2 == 0) ? 1.0 : 0.0; W = FSurfaceWetness::Step(W, Rain, Dt); if (W < -Eps || W > 1.0 + Eps) bounded = false; }
		CHECK(bounded, "stays in [0,1]");
	}

	if (g_fail == 0) { std::printf("surface_wetness_verify: ALL PASS\n"); return 0; }
	std::printf("surface_wetness_verify: %d FAILED\n", g_fail);
	return 1;
}
