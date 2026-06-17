// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Projects real-world WGS84 (lat, lon) coordinates into the game's local metric
 * space, relative to a district origin. Equirectangular projection: accurate to
 * well under a metre across a city-scale district (a few km), which is far below
 * what a player can perceive, and it is cheap and exactly invertible.
 *
 * Ported 1:1 from the the reference RefCounted `GeoProjection`
 * (game/scripts/world/geo_projection.gd). Plain value type, no UObject — unit
 * tested headless via the reference behavior (World/Tests/GeoProjectionTest.cpp).
 *
 * Double precision throughout (LWC): the inverse round-trip is parity-tested to
 * 1e-6 degrees, which is only achievable with doubles.
 *
 * Axis convention (Godot, Y-up): +X = east, -Z = north, ground on the XZ plane.
 * The convention-free core `ProjectEastNorth` returns pure (East, North) scalars;
 * the `ToLocal*`/`ToGeo` accessors reproduce the the reference axis layout exactly.
 *
 * NOTE: no UE Z-up remap is baked in here — porting the axis convention to UE's
 * Z-up space is a deferred Wave 3 concern. These accessors are parity-literal
 * with the the reference source so the ported unit tests match the oracle bit-for-bit.
 */
class GTC_UE5_API FGeoProjection
{
public:
    /** Metres per degree of latitude (WGS84 mean). Longitude scales by cos(lat). */
    static constexpr double MetresPerDegLat = 111320.0;

    /** Constructs a projection about the given origin (degrees). */
    FGeoProjection(double InOriginLat, double InOriginLon)
        : Lat0(InOriginLat)
        , Lon0(InOriginLon)
        , MetresPerDegLon(MetresPerDegLat * FMath::Cos(FMath::DegreesToRadians(InOriginLat)))
    {
    }

    /**
     * Convention-free core projection: returns (East, North) in metres relative
     * to the origin. No axis remap — callers pick the convention.
     */
    FVector2D ProjectEastNorth(double Lat, double Lon) const
    {
        const double East = (Lon - Lon0) * MetresPerDegLon;
        const double North = (Lat - Lat0) * MetresPerDegLat;
        return FVector2D(East, North);
    }

    /** Project a geographic point to a ground-plane position (Godot: x=East, y=0, z=-North). */
    FVector ToLocal(double Lat, double Lon) const
    {
        const FVector2D EN = ProjectEastNorth(Lat, Lon);
        return FVector(EN.X, 0.0, -EN.Y);
    }

    /** Projection collapsed to the XZ plane (the reference Vector2: x=East, y=-North). */
    FVector2D ToLocal2D(double Lat, double Lon) const
    {
        const FVector2D EN = ProjectEastNorth(Lat, Lon);
        return FVector2D(EN.X, -EN.Y);
    }

    /** Inverse: local metric position back to (Lat, Lon). Preserves the Godot (lat, lon) ordering. */
    FVector2D ToGeo(const FVector& Local) const
    {
        const double Lon = Lon0 + Local.X / MetresPerDegLon;
        const double Lat = Lat0 - Local.Z / MetresPerDegLat;
        return FVector2D(Lat, Lon);
    }

private:
    double Lat0;
    double Lon0;
    double MetresPerDegLon;
};
