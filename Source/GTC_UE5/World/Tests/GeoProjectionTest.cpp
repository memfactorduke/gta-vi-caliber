// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../GeoProjection.h"

// Parity oracle: game/tests/unit/test_geo_projection.gd. Each It(...) maps 1:1 to
// one Godot test function, using the SAME numeric tolerances as the oracle —
// including its deliberate sub-metre equirectangular allowances and the 1e-6
// degree inverse round-trip (the round-trip that mandates double precision).
BEGIN_DEFINE_SPEC(FGeoProjectionSpec, "GTC.World.GeoProjection",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FGeoProjectionSpec)

void FGeoProjectionSpec::Define()
{
    // test_origin_maps_to_zero
    It("maps the origin to zero", [this]()
    {
        const FGeoProjection Proj(34.05, -118.25);
        const FVector P = Proj.ToLocal(34.05, -118.25);
        TestTrue(TEXT("origin projects to (near) zero"), P.Length() < 0.001);
    });

    // test_one_degree_latitude_is_111km_north
    It("maps one degree of latitude to ~111 km north", [this]()
    {
        const FGeoProjection Proj(0.0, 0.0);
        // One degree north of the equator origin -> -Z (north), ~111.32 km.
        const FVector P = Proj.ToLocal(1.0, 0.0);
        const bool bOkAxis = FMath::Abs(P.X) < 0.001 && FMath::Abs(P.Y) < 0.001;
        const bool bOkDist = FMath::Abs(-P.Z - FGeoProjection::MetresPerDegLat) < 1.0;
        TestTrue(TEXT("axis clean and distance within 1 m"), bOkAxis && bOkDist);
    });

    // test_longitude_shrinks_with_latitude
    It("shrinks a degree of longitude with latitude", [this]()
    {
        // A degree of longitude is ~111 km at the equator but only ~91 km at LA's
        // latitude (cos 34 deg ~ 0.829). East is +X.
        const FGeoProjection Proj(34.05, -118.25);
        const FVector P = Proj.ToLocal(34.05, -118.25 + 1.0);
        const double Expected = FGeoProjection::MetresPerDegLat * FMath::Cos(FMath::DegreesToRadians(34.05));
        TestTrue(TEXT("on-axis, near expected, and shorter than a lat degree"),
            P.Z == 0.0 && FMath::Abs(P.X - Expected) < 1.0 && P.X < FGeoProjection::MetresPerDegLat);
    });

    // test_east_is_positive_x_north_is_negative_z
    It("places east at +X and north at -Z", [this]()
    {
        const FGeoProjection Proj(34.05, -118.25);
        const FVector North = Proj.ToLocal(34.06, -118.25);
        const FVector East = Proj.ToLocal(34.05, -118.24);
        TestTrue(TEXT("north is -Z on-axis, east is +X on-axis"),
            North.Z < 0.0 && FMath::Abs(North.X) < 0.001 && East.X > 0.0 && FMath::Abs(East.Z) < 0.001);
    });

    // test_inverse_round_trips
    It("round-trips the inverse to 1e-6 degrees", [this]()
    {
        const FGeoProjection Proj(34.0503, -118.2523);
        const FVector2D Geo(34.0541, -118.2487);
        const FVector Local = Proj.ToLocal(Geo.X, Geo.Y);
        const FVector2D Back = Proj.ToGeo(Local);
        TestTrue(TEXT("lat and lon recovered within 1e-6 degrees"),
            FMath::Abs(Back.X - Geo.X) < 1e-6 && FMath::Abs(Back.Y - Geo.Y) < 1e-6);
    });

    // test_to_local_2d_matches_3d
    It("keeps ToLocal2D consistent with ToLocal", [this]()
    {
        const FGeoProjection Proj(34.05, -118.25);
        const FVector P3 = Proj.ToLocal(34.0541, -118.2487);
        const FVector2D P2 = Proj.ToLocal2D(34.0541, -118.2487);
        TestTrue(TEXT("2D x matches 3D x, 2D y matches 3D z"),
            FMath::Abs(P2.X - P3.X) < 0.001 && FMath::Abs(P2.Y - P3.Z) < 0.001);
    });
}

#endif // WITH_AUTOMATION_TESTS
