// Copyright (c) 2026 GTC contributors

#include "PoliceDispatch.h"
#include "../../World/Police/PoliceResponse.h"

int32 FPoliceDispatch::DesiredUnits(int32 Stars, int32 MaxAlive)
{
    // Godot: mini(PoliceResponse.units_for(stars), maxi(max_alive, 0)).
    return FMath::Min(FPoliceResponse::UnitsFor(Stars), FMath::Max(MaxAlive, 0));
}

int32 FPoliceDispatch::SpawnCount(int32 Stars, int32 Alive, int32 MaxAlive, int32 MaxPerWave)
{
    // Godot: clampi(desired_units(stars, max_alive) - alive, 0, maxi(max_per_wave, 0)).
    const int32 Deficit = DesiredUnits(Stars, MaxAlive) - Alive;
    return FMath::Clamp(Deficit, 0, FMath::Max(MaxPerWave, 0));
}

double FPoliceDispatch::RingAngle(int32 Index, int32 Total, double UJitter, double JitterFrac)
{
    // Godot: slice = TAU / float(maxi(total, 1));
    //        index * slice + (clampf(u_jitter,0,1) - 0.5) * slice * clampf(jitter_frac,0,1)
    const double Slice = UE_DOUBLE_TWO_PI / double(FMath::Max(Total, 1));
    return double(Index) * Slice
        + (FMath::Clamp(UJitter, 0.0, 1.0) - 0.5) * Slice * FMath::Clamp(JitterFrac, 0.0, 1.0);
}

FVector FPoliceDispatch::RingPosition(
    const FVector& Center, double Radius, double Angle, double URadius, double RadialJitter)
{
    // Godot: r = maxf(radius + (clampf(u_radius,0,1) - 0.5) * 2.0 * radial_jitter, 0.0);
    //        center + Vector3(cos(angle) * r, 0.0, sin(angle) * r)
    // Planar XZ, Y carried from Center. NO Z-up remap (verbatim Vector3 math).
    const double R = FMath::Max(
        Radius + (FMath::Clamp(URadius, 0.0, 1.0) - 0.5) * 2.0 * RadialJitter, 0.0);
    return Center + FVector(FMath::Cos(Angle) * R, 0.0, FMath::Sin(Angle) * R);
}

bool FPoliceDispatch::ShouldDespawn(int32 Stars, double DistanceToPlayer, double DespawnRadius)
{
    // Godot: if stars <= 0: return true; return distance_to_player > despawn_radius.
    if (Stars <= 0)
    {
        return true;
    }
    return DistanceToPlayer > DespawnRadius;
}
