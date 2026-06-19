// Headless verifier for FOceanWake. Compiles the REAL core .cpp against the shim
// and re-asserts the GTC.World.Ocean.Wake invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Environment/OceanWake.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
	const double Eps = 1e-4;

	// Angle
	{
		double Hull = 4.0;
		double FrHalf = 0.5 * std::sqrt(9.81 * Hull);
		CHECK(Absd(FOceanWake::WakeHalfAngleDeg(0.0, Hull) - FOceanWake::KelvinHalfAngleDeg) < Eps, "slow wake is Kelvin angle");
		CHECK(Absd(FOceanWake::WakeHalfAngleDeg(FrHalf, Hull) - FOceanWake::KelvinHalfAngleDeg) < 1e-3, "threshold continuous");
		double A1 = FOceanWake::WakeHalfAngleDeg(2.0 * FrHalf, Hull);
		double A2 = FOceanWake::WakeHalfAngleDeg(4.0 * FrHalf, Hull);
		CHECK(A1 < FOceanWake::KelvinHalfAngleDeg - Eps, "fast wake narrower");
		CHECK(A2 < A1 - Eps, "faster even narrower");
		CHECK(A2 > 0.0, "angle positive");
		CHECK(Absd(FOceanWake::FroudeNumber(FrHalf, Hull) - 0.5) < Eps, "froude value");
		std::printf("  [info] half-angles: Kelvin %.3f, 2x %.3f, 4x %.3f deg\n", FOceanWake::KelvinHalfAngleDeg, A1, A2);
	}

	// Strength + foam rate
	{
		double P = 10.0;
		CHECK(FOceanWake::WakeStrength01(0.0, P) < Eps, "no wake at rest");
		CHECK(Absd(FOceanWake::WakeStrength01(P, P) - 1.0) < Eps, "full wake planing");
		bool mono = true, bounded = true; double prev = -1.0;
		for (int i = 0; i <= 40; ++i)
		{
			double S = FOceanWake::WakeStrength01(i * 0.5, P);
			if (S < prev - Eps) mono = false;
			if (S < -Eps || S > 1.0 + Eps) bounded = false;
			prev = S;
		}
		CHECK(mono, "wake strength monotone");
		CHECK(bounded, "wake strength bounded");
		CHECK(FOceanWake::BowFoamRate(0.0, P, 200.0) < Eps, "no foam at rest");
		bool rb = true;
		for (int i = 0; i <= 60; ++i) { double R = FOceanWake::BowFoamRate(i * 0.5, P, 200.0); if (R < -Eps || R > 200.0 + Eps) rb = false; }
		CHECK(rb, "foam rate bounded");
	}

	// Foam + trail
	{
		CHECK(FOceanWake::WhitecapFoam01(1.0) < Eps, "no foam calm");
		CHECK(Absd(FOceanWake::WhitecapFoam01(0.0) - 1.0) < Eps, "full foam fold");
		CHECK(Absd(FOceanWake::WhitecapFoam01(-0.5) - 1.0) < Eps, "over-fold clamps");
		bool mono = true; double prev = 2.0;
		for (int i = 0; i <= 30; ++i) { double F = FOceanWake::WhitecapFoam01(i / 30.0, 0.6); if (F > prev + Eps) mono = false; prev = F; }
		CHECK(mono, "foam decreases as sea smooths");
		CHECK(Absd(FOceanWake::TrailFade01(0.0, 3.0) - 1.0) < Eps, "fresh trail full");
		CHECK(FOceanWake::TrailFade01(3.0, 3.0) < Eps, "dead trail gone");
		CHECK(Absd(FOceanWake::TrailFade01(1.5, 3.0) - 0.5) < Eps, "half-aged half");
	}

	if (g_fail == 0) { std::printf("ocean_wake_verify: ALL PASS\n"); return 0; }
	std::printf("ocean_wake_verify: %d FAILED\n", g_fail);
	return 1;
}
