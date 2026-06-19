// Headless verifier for FOceanBuoyancy. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.World.Ocean.Buoyancy invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Environment/OceanBuoyancy.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

static void MakeRaft(FBuoyancyPoint Out[4], double Zpp, double Zpm, double Zmp, double Zmm,
	double WaterH, double Span = 0.5, double Vol = 1.0)
{
	const double Xs[4] = { 1.0, 1.0, -1.0, -1.0 };
	const double Ys[4] = { 1.0, -1.0, 1.0, -1.0 };
	const double Zs[4] = { Zpp, Zpm, Zmp, Zmm };
	for (int i = 0; i < 4; ++i)
	{
		Out[i] = FBuoyancyPoint{};
		Out[i].X = Xs[i]; Out[i].Y = Ys[i]; Out[i].Z = Zs[i];
		Out[i].WaterHeight = WaterH; Out[i].Span = Span; Out[i].Volume = Vol;
	}
}

int main()
{
	const double Eps = 1e-4;
	const double RhoG = FOceanBuoyancy::SeaWaterDensity * FOceanBuoyancy::DefaultGravity;

	// Extremes
	{
		FBuoyancyPoint Dry[4]; MakeRaft(Dry, 0.5, 0.5, 0.5, 0.5, 0.0);
		FBuoyancyResult D = FOceanBuoyancy::Compute(Dry, 4, 0, 0, 0);
		CHECK(Absd(D.Force.Z) < Eps, "dry no force");
		CHECK(D.SubmergedFraction < Eps, "dry not submerged");

		FBuoyancyPoint Deep[4]; MakeRaft(Deep, -2, -2, -2, -2, 0.0);
		FBuoyancyResult S = FOceanBuoyancy::Compute(Deep, 4, 0, 0, 0);
		CHECK(Absd(S.SubmergedFraction - 1.0) < Eps, "submerged fraction 1");
		CHECK(Absd(S.Force.Z - RhoG * 4.0) < 0.1, "submerged force = displaced weight");
		CHECK(Absd(S.Torque.X) + Absd(S.Torque.Y) < 1e-6, "submerged symmetric no torque");

		FBuoyancyResult E = FOceanBuoyancy::Compute(nullptr, 0, 0, 0, 0);
		CHECK(Absd(E.Force.Z) + E.SubmergedFraction < Eps, "empty zero");
	}

	// Righting
	{
		FBuoyancyPoint Level[4]; MakeRaft(Level, 0, 0, 0, 0, 0.25);
		FBuoyancyResult L = FOceanBuoyancy::Compute(Level, 4, 0, 0, 0);
		CHECK(L.Force.Z > 0.0, "level pushes up");
		CHECK(Absd(L.SubmergedFraction - 0.5) < Eps, "level half submerged");
		CHECK(Absd(L.Torque.X) + Absd(L.Torque.Y) < 1e-6, "level no righting torque");

		FBuoyancyPoint Tilt[4]; MakeRaft(Tilt, -0.3, -0.3, 0.3, 0.3, 0.0);
		FBuoyancyResult Tq = FOceanBuoyancy::Compute(Tilt, 4, 0, 0, 0);
		CHECK(Tq.Force.Z > 0.0, "tilt has up force");
		CHECK(Tq.Torque.Y < -Eps, "tilt righting torque about Y is restoring");
		CHECK(Absd(Tq.Torque.X) < 1e-6, "tilt symmetric in Y -> no X torque");
	}

	// Depth curve
	{
		FBuoyancyPoint P{}; P.Volume = 1.0; P.Span = 0.5; P.WaterHeight = 0.0;
		P.Z = 0.1;   CHECK(FOceanBuoyancy::Submersion01(P) < Eps, "above water dry");
		P.Z = -0.25; CHECK(Absd(FOceanBuoyancy::Submersion01(P) - 0.5) < Eps, "half span -> 0.5");
		P.Z = -0.5;  CHECK(Absd(FOceanBuoyancy::Submersion01(P) - 1.0) < Eps, "full span -> 1");
		P.Z = -5.0;  CHECK(Absd(FOceanBuoyancy::Submersion01(P) - 1.0) < Eps, "deep clamps at 1");

		bool mono = true; double prev = -1.0, last = 0.0;
		for (int i = 0; i <= 40; ++i)
		{
			FBuoyancyPoint Q{}; Q.Volume = 1.0; Q.Span = 0.5; Q.WaterHeight = 0.0;
			Q.Z = 0.2 - i * 0.05;
			FBuoyancyResult Rr = FOceanBuoyancy::Compute(&Q, 1, 0, 0, 0);
			if (Rr.Force.Z < prev - Eps) mono = false;
			prev = Rr.Force.Z; last = Rr.Force.Z;
		}
		CHECK(mono, "force rises monotonically with depth");
		CHECK(Absd(last - RhoG * 1.0) < 0.1, "force caps at displaced weight");
	}

	// Drag
	{
		FBuoyVec3 V; V.X = 3.0; V.Y = -2.0; V.Z = 1.0;
		FBuoyVec3 Half = FOceanBuoyancy::LinearDrag(V, 0.5, 4.0);
		CHECK(Half.X < 0.0 && Half.Y > 0.0 && Half.Z < 0.0, "drag opposes velocity");
		CHECK(Absd(Half.X - (-4.0 * 0.5 * 3.0)) < Eps, "drag magnitude");
		FBuoyVec3 None = FOceanBuoyancy::LinearDrag(V, 0.0, 4.0);
		CHECK(Absd(None.X) + Absd(None.Y) + Absd(None.Z) < Eps, "dry no drag");
		FBuoyVec3 W; W.Z = 2.0;
		FBuoyVec3 Ad = FOceanBuoyancy::AngularDrag(W, 1.0, 0.7);
		CHECK(Ad.Z < 0.0 && Absd(Ad.Z - (-0.7 * 2.0)) < Eps, "angular drag opposes spin");
	}

	if (g_fail == 0) { std::printf("ocean_buoyancy_verify: ALL PASS\n"); return 0; }
	std::printf("ocean_buoyancy_verify: %d FAILED\n", g_fail);
	return 1;
}
