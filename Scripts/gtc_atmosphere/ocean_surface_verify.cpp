// Headless verifier for FOceanSurface. Compiles the REAL core .cpp against the
// shim and re-asserts the GTC.World.Ocean.Surface invariants with a host clang.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Environment/OceanSurface.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)

static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
	const double Eps = 1e-4;

	// --- Calm ------------------------------------------------------------
	{
		FOceanSample E = FOceanSurface::Sample(nullptr, 0, 10.0, -3.0, 7.0);
		CHECK(Absd(E.Height) < Eps, "empty height 0");
		CHECK(Absd(E.NormalZ - 1.0) < Eps, "empty normal up");
		CHECK(Absd(E.Jacobian - 1.0) < Eps, "empty jacobian 1");

		FGerstnerWave Flat; Flat.Amplitude = 0.0;
		FOceanSample S = FOceanSurface::Sample(&Flat, 1, 4.0, 4.0, 1.0);
		CHECK(Absd(S.Height) < Eps, "flat height 0");
		CHECK(Absd(S.DispX) + Absd(S.DispY) < Eps, "flat disp 0");
		CHECK(Absd(S.NormalZ - 1.0) < Eps, "flat normal up");
	}

	// --- Geometry / dispersion ------------------------------------------
	{
		FGerstnerWave W; W.DirX = 1.0; W.DirY = 0.0; W.Amplitude = 0.5;
		W.Wavelength = 8.0; W.Steepness = 0.3; W.Speed = 0.0;
		double K = FOceanSurface::Wavenumber(8.0);
		CHECK(Absd(FOceanSurface::AngularFrequency(W) - std::sqrt(9.81 * K)) < Eps, "dispersion deep water");
		W.Speed = 4.0;
		CHECK(Absd(FOceanSurface::AngularFrequency(W) - 4.0 * K) < Eps, "dispersion fixed speed");

		CHECK(Absd(FOceanSurface::HeightAt(&W, 1, 0.0, 0.0, 0.0)) < Eps, "trough at origin");
		CHECK(Absd(FOceanSurface::HeightAt(&W, 1, 2.0, 0.0, 0.0) - 0.5) < Eps, "crest at quarter wavelength");

		FGerstnerWave Round = W; Round.Steepness = 0.0;
		FOceanSample R = FOceanSurface::Sample(&Round, 1, 1.0, 0.0, 0.0);
		CHECK(Absd(R.DispX) + Absd(R.DispY) < Eps, "round wave no disp");
		FOceanSample P = FOceanSurface::Sample(&W, 1, 1.0, 0.0, 0.0);
		CHECK(Absd(P.DispX) > Eps, "pinched wave has disp");
	}

	// --- Well-formed across a field -------------------------------------
	{
		FGerstnerWave Waves[3];
		Waves[0].DirX = 1.0;  Waves[0].DirY = 0.1; Waves[0].Amplitude = 0.6; Waves[0].Wavelength = 18.0; Waves[0].Steepness = 0.5; Waves[0].Speed = 0.0;
		Waves[1].DirX = 0.6;  Waves[1].DirY = 0.8; Waves[1].Amplitude = 0.3; Waves[1].Wavelength = 9.0;  Waves[1].Steepness = 0.4; Waves[1].Speed = 0.0;
		Waves[2].DirX = -0.3; Waves[2].DirY = 1.0; Waves[2].Amplitude = 0.15;Waves[2].Wavelength = 4.0;  Waves[2].Steepness = 0.3; Waves[2].Speed = 0.0;
		double AmpSum = 0.6 + 0.3 + 0.15;

		bool bounded = true, unit = true, up = true;
		double minNz = 1.0, worstH = 0.0;
		for (int ix = 0; ix < 40; ++ix)
		for (int iy = 0; iy < 40; ++iy)
		{
			double X = ix * 0.7, Y = iy * 0.7, T = (ix + iy) * 0.05;
			FOceanSample S = FOceanSurface::Sample(Waves, 3, X, Y, T);
			worstH = std::fmax(worstH, Absd(S.Height));
			if (Absd(S.Height) > AmpSum + Eps) bounded = false;
			double NLen = std::sqrt(S.NormalX*S.NormalX + S.NormalY*S.NormalY + S.NormalZ*S.NormalZ);
			if (Absd(NLen - 1.0) > 1e-6) unit = false;
			minNz = std::fmin(minNz, S.NormalZ);
			if (S.NormalZ <= 0.0) up = false;
		}
		CHECK(bounded, "height bounded by amp sum");
		CHECK(unit, "normal is unit length");
		CHECK(up, "normal faces up");
		std::printf("  [info] worst height %.3f (sum %.3f), min Nz %.3f\n", worstH, AmpSum, minNz);
	}

	// --- Periodic + continuous ------------------------------------------
	{
		FGerstnerWave W; W.DirX = 1.0; W.DirY = 0.0; W.Amplitude = 0.5;
		W.Wavelength = 8.0; W.Steepness = 0.4; W.Speed = 4.0;
		double K = FOceanSurface::Wavenumber(8.0);
		double Omega = 4.0 * K;
		double Period = (2.0 * 3.14159265358979) / Omega;

		bool periodic = true;
		for (int i = 0; i < 16; ++i)
		{
			double T = i * 0.11;
			double H0 = FOceanSurface::HeightAt(&W, 1, 1.3, 0.0, T);
			double H1 = FOceanSurface::HeightAt(&W, 1, 1.3, 0.0, T + Period);
			if (Absd(H0 - H1) > 1e-6) periodic = false;
		}
		CHECK(periodic, "height is temporally periodic");

		double DX = 0.01, MaxStep = K * W.Amplitude * DX * 2.0, worst = 0.0;
		bool cont = true;
		double Prev = FOceanSurface::HeightAt(&W, 1, 0.0, 0.0, 1.0);
		for (int i = 1; i <= 2000; ++i)
		{
			double Cur = FOceanSurface::HeightAt(&W, 1, i * DX, 0.0, 1.0);
			double Step = Absd(Cur - Prev);
			worst = std::fmax(worst, Step);
			if (Step > MaxStep) cont = false;
			Prev = Cur;
		}
		CHECK(cont, "height is spatially continuous");
		std::printf("  [info] worst spatial step %.5f (bound %.5f)\n", worst, K * W.Amplitude * DX * 2.0);
	}

	if (g_fail == 0) { std::printf("ocean_surface_verify: ALL PASS\n"); return 0; }
	std::printf("ocean_surface_verify: %d FAILED\n", g_fail);
	return 1;
}
