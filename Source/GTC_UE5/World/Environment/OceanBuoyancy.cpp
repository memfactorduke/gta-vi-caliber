// Copyright Epic Games, Inc. All Rights Reserved.

#include "OceanBuoyancy.h"
#include "Math/UnrealMathUtility.h"

double FOceanBuoyancy::Submersion01(const FBuoyancyPoint& Point)
{
	const double Depth = Point.WaterHeight - Point.Z; // >0 when the sample is underwater
	if (Depth <= 0.0)
	{
		return 0.0;
	}
	const double Span = Point.Span > 1e-6 ? Point.Span : 1e-6; // avoid div-by-zero
	return FMath::Clamp(Depth / Span, 0.0, 1.0);
}

FBuoyancyResult FOceanBuoyancy::Compute(const FBuoyancyPoint* Points, int32 Count,
	double CenterX, double CenterY, double CenterZ, double Gravity, double Density)
{
	FBuoyancyResult R;
	if (Points == nullptr || Count <= 0)
	{
		return R;
	}

	const double RhoG = FMath::Max(0.0, Density) * FMath::Max(0.0, Gravity);

	double TotalVolume = 0.0;
	double SubmergedVolume = 0.0;

	for (int32 i = 0; i < Count; ++i)
	{
		const FBuoyancyPoint& P = Points[i];
		TotalVolume += FMath::Max(0.0, P.Volume);

		const double Sub = Submersion01(P);
		if (Sub <= 0.0)
		{
			continue;
		}

		const double DispVol = FMath::Max(0.0, P.Volume) * Sub;
		SubmergedVolume += DispVol;

		// Buoyant force is straight up; magnitude is the displaced weight rho*g*V.
		const double Fz = RhoG * DispVol;
		R.Force.Z += Fz;

		// Righting torque about the centre: r x F with F = (0,0,Fz).
		// r x (0,0,Fz) = (ry*Fz, -rx*Fz, 0).
		const double Rx = P.X - CenterX;
		const double Ry = P.Y - CenterY;
		R.Torque.X += Ry * Fz;
		R.Torque.Y += -Rx * Fz;
	}

	R.SubmergedFraction = TotalVolume > 1e-9 ? FMath::Clamp(SubmergedVolume / TotalVolume, 0.0, 1.0) : 0.0;
	return R;
}

FBuoyVec3 FOceanBuoyancy::LinearDrag(const FBuoyVec3& Velocity, double SubmergedFraction, double Coeff)
{
	const double S = FMath::Clamp(SubmergedFraction, 0.0, 1.0) * FMath::Max(0.0, Coeff);
	FBuoyVec3 F;
	F.X = -S * Velocity.X;
	F.Y = -S * Velocity.Y;
	F.Z = -S * Velocity.Z;
	return F;
}

FBuoyVec3 FOceanBuoyancy::AngularDrag(const FBuoyVec3& AngularVelocity, double SubmergedFraction, double Coeff)
{
	const double S = FMath::Clamp(SubmergedFraction, 0.0, 1.0) * FMath::Max(0.0, Coeff);
	FBuoyVec3 T;
	T.X = -S * AngularVelocity.X;
	T.Y = -S * AngularVelocity.Y;
	T.Z = -S * AngularVelocity.Z;
	return T;
}
