// Copyright Epic Games, Inc. All Rights Reserved.

#include "OceanSurface.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	constexpr double TwoPi = 2.0 * UE_DOUBLE_PI;
	constexpr double OceanEps = 1e-9;
}

double FOceanSurface::Wavenumber(double Wavelength)
{
	if (Wavelength <= OceanEps)
	{
		return 0.0;
	}
	return TwoPi / Wavelength;
}

double FOceanSurface::AngularFrequency(const FGerstnerWave& Wave, double Gravity)
{
	const double K = Wavenumber(Wave.Wavelength);
	if (K <= OceanEps)
	{
		return 0.0;
	}
	if (Wave.Speed > 0.0)
	{
		return Wave.Speed * K;
	}
	// Deep-water dispersion: omega = sqrt(g*k). Long waves run faster than short.
	return FMath::Sqrt(FMath::Max(0.0, Gravity) * K);
}

FOceanSample FOceanSurface::Sample(const FGerstnerWave* Waves, int32 Count,
	double X, double Y, double Time, double Gravity)
{
	FOceanSample S; // calm defaults: height 0, disp 0, normal +Z, Jacobian 1

	if (Waves == nullptr || Count <= 0)
	{
		return S;
	}

	// Normal accumulators (GPU-Gems analytic normal): the XY slope sums and the
	// vertical reduction Sum(Q*k*A*sin); Jacobian partials track horizontal folding.
	double SlopeX = 0.0;
	double SlopeY = 0.0;
	double NzReduce = 0.0;
	double Jxx = 0.0; // Sum QA*k*dx*dx*sin
	double Jxy = 0.0; // Sum QA*k*dx*dy*sin
	double Jyy = 0.0; // Sum QA*k*dy*dy*sin

	for (int32 i = 0; i < Count; ++i)
	{
		const FGerstnerWave& W = Waves[i];

		// Normalise the travel direction; skip a degenerate (zero-direction) wave.
		const double DirLen = FMath::Sqrt(W.DirX * W.DirX + W.DirY * W.DirY);
		if (DirLen <= OceanEps)
		{
			continue;
		}
		const double Dx = W.DirX / DirLen;
		const double Dy = W.DirY / DirLen;

		const double K = Wavenumber(W.Wavelength);
		if (K <= OceanEps)
		{
			continue;
		}

		const double A = W.Amplitude;
		const double Q = FMath::Clamp(W.Steepness, 0.0, 1.0);
		const double QA = Q * A;       // horizontal displacement amplitude
		const double WA = K * A;        // slope/normal weight
		const double Omega = AngularFrequency(W, Gravity);

		const double Phase = K * (Dx * X + Dy * Y) - Omega * Time;
		const double C = FMath::Cos(Phase);
		const double Sn = FMath::Sin(Phase);

		// Height (vertical) and the circular horizontal displacement.
		S.Height += A * Sn;
		S.DispX += QA * Dx * C;
		S.DispY += QA * Dy * C;

		// Analytic surface normal: slope grows with k*A*cos along the direction.
		SlopeX += Dx * WA * C;
		SlopeY += Dy * WA * C;
		NzReduce += Q * WA * Sn;

		// Horizontal-map Jacobian partials (d disp / d position).
		const double QAK = QA * K;
		Jxx += QAK * Dx * Dx * Sn;
		Jxy += QAK * Dx * Dy * Sn;
		Jyy += QAK * Dy * Dy * Sn;
	}

	// Assemble and normalise the surface normal (guard the degenerate length).
	double Nx = -SlopeX;
	double Ny = -SlopeY;
	double Nz = 1.0 - NzReduce;
	const double NLen = FMath::Sqrt(Nx * Nx + Ny * Ny + Nz * Nz);
	if (NLen > OceanEps)
	{
		Nx /= NLen;
		Ny /= NLen;
		Nz /= NLen;
	}
	else
	{
		Nx = 0.0;
		Ny = 0.0;
		Nz = 1.0;
	}
	S.NormalX = Nx;
	S.NormalY = Ny;
	S.NormalZ = Nz;

	// Folding determinant of the horizontal map (x,y) -> (x+dispX, y+dispY).
	const double dXdx = 1.0 - Jxx;
	const double dXdy = -Jxy;
	const double dYdx = -Jxy; // symmetric: d dispY / dx == d dispX / dy
	const double dYdy = 1.0 - Jyy;
	S.Jacobian = dXdx * dYdy - dXdy * dYdx;

	return S;
}

double FOceanSurface::HeightAt(const FGerstnerWave* Waves, int32 Count,
	double X, double Y, double Time, double Gravity)
{
	return Sample(Waves, Count, X, Y, Time, Gravity).Height;
}
