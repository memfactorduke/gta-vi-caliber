// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCMinimapWidget.h"

#include "GTCMinimapProjection.h"
#include "../../AI/Gps/GpsNavigation.h"

namespace
{
    /**
     * Adapt a native UE world point (Z-up, XY ground) into the frame FGpsNavigation
     * still reasons in: Godot XZ, where Ground() keeps X and Z and drops the middle
     * axis. Putting UE's Y into the nav Z slot makes the nav math operate on the UE XY
     * ground plane. Distances come out in the same units (cm). This is the local
     * face of the deferred "Wave-3 axis remap" — kept out of the shared pure-core.
     */
    FVector ToNavFrame(const FVector& UeWorld)
    {
        return FVector(UeWorld.X, 0.0, UeWorld.Y);
    }

    TArray<FVector> RouteToNavFrame(const TArray<FVector>& Route)
    {
        TArray<FVector> Out;
        Out.Reserve(Route.Num());
        for (const FVector& P : Route)
        {
            Out.Add(ToNavFrame(P));
        }
        return Out;
    }

    /** Seconds -> "M:SS", or "--" when not a finite positive time. */
    FString FormatEta(double Seconds)
    {
        if (!FMath::IsFinite(Seconds) || Seconds <= 0.0)
        {
            return TEXT("--");
        }
        const int32 Total = FMath::RoundToInt(Seconds);
        return FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60);
    }
}

void UGTCMinimapWidget::SetCenterAndHeading(const FVector& Center, float HeadingDeg)
{
    CenterWorld = Center;
    HeadingDegrees = HeadingDeg;
    HeadingRad = FMath::DegreesToRadians(HeadingDeg);
    PlayerWorld2D = FVector2D(Center.X, Center.Y);
}

void UGTCMinimapWidget::SetMarkers(const TArray<FGTCMinimapMarker>& InMarkers)
{
    Markers = InMarkers;
}

void UGTCMinimapWidget::SetRoute(const TArray<FVector>& WorldPolyline)
{
    RouteWorld = WorldPolyline;
}

void UGTCMinimapWidget::SetPlayerSpeed(float SpeedCmS)
{
    PlayerSpeedCmS = SpeedCmS;
}

void UGTCMinimapWidget::Refresh()
{
    FGTCMinimapProjection View;
    View.Center = CenterWorld;
    View.HeadingRadians = HeadingRad;
    View.RangeCm = RangeCm;

    // --- Markers: project, cull off-range non-edge markers, pin edge markers to rim --
    ProjectedMarkers.Reset(Markers.Num());
    for (const FGTCMinimapMarker& Marker : Markers)
    {
        const FVector2D Norm = View.WorldToNormalized(Marker.WorldLocation);
        const bool bOutside = Norm.SizeSquared() > 1.0;
        if (bOutside && !Marker.bClampToEdge)
        {
            continue; // off-radar and not sticky — don't draw it
        }

        FGTCMinimapProjectedMarker Projected;
        bool bClamped = false;
        Projected.Normalized = FGTCMinimapProjection::ClampToDisc(Norm, bClamped);
        Projected.Kind = Marker.Kind;
        Projected.bOnEdge = bClamped;
        ProjectedMarkers.Add(Projected);
    }

    // --- Route: project every waypoint (unclamped; the WBP clips the line to the disc) --
    ProjectedRoute.Reset(RouteWorld.Num());
    for (const FVector& P : RouteWorld)
    {
        ProjectedRoute.Add(View.WorldToNormalized(P));
    }

    // --- GPS readout from the route, via the UE->nav-frame adapter ------------------
    bHasRoute = RouteWorld.Num() >= 2;
    if (bHasRoute)
    {
        const FVector NavPos = ToNavFrame(CenterWorld);
        const TArray<FVector> NavRoute = RouteToNavFrame(RouteWorld);

        RouteDistanceM = static_cast<float>(FGpsNavigation::DistanceRemaining(NavPos, NavRoute) / 100.0);
        RouteEtaText = FormatEta(FGpsNavigation::EtaSeconds(NavPos, NavRoute, PlayerSpeedCmS));

        const FGpsNavigation::FNextTurn Turn =
            FGpsNavigation::NextTurn(NavPos, NavRoute, FMath::DegreesToRadians(TurnThresholdDegrees));
        if (Turn.bHasTurn)
        {
            const TCHAR* Dir = (Turn.Direction == ETurnDirection::Left) ? TEXT("Left") : TEXT("Right");
            NextTurnText = FString::Printf(TEXT("%s %d m"), Dir, FMath::RoundToInt(Turn.Distance / 100.0));
        }
        else
        {
            NextTurnText.Reset();
        }
    }
    else
    {
        RouteDistanceM = 0.0f;
        RouteEtaText = TEXT("--");
        NextTurnText.Reset();
    }

    // The WBP repaints from the cached output. Always safe to call (no-op when no BP override).
    OnMinimapUpdated();
}
