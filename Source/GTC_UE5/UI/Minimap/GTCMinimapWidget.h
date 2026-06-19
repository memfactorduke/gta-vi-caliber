// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GTCMinimapWidget.generated.h"

/**
 * One world marker to draw on the radar (a POI from the place registry, an NPC, the
 * active mission objective). The widget projects WorldLocation onto the disc each
 * Refresh.
 */
USTRUCT(BlueprintType)
struct FGTCMinimapMarker
{
    GENERATED_BODY()

    /** World location of the thing this marker represents. */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Minimap")
    FVector WorldLocation = FVector::ZeroVector;

    /** What kind it is ("objective", "diner", "npc", ...) — drives the icon in the WBP. */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Minimap")
    FName Kind;

    /** When true, keep this marker on the rim if it falls off-range (objective arrow).
     *  When false, an off-range marker is culled (not drawn). */
    UPROPERTY(BlueprintReadWrite, Category = "GTC|Minimap")
    bool bClampToEdge = false;
};

/** A marker after projection — what the WBP iterates to place icons on the disc. */
USTRUCT(BlueprintType)
struct FGTCMinimapProjectedMarker
{
    GENERATED_BODY()

    /** Disc position, X = right / Y = forward(up), in [-1, 1]. WBP flips Y for screen. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    FVector2D Normalized = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    FName Kind;

    /** True when the marker was off-range and pinned to the rim (off-radar arrow). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    bool bOnEdge = false;
};

/**
 * UGTCMinimapWidget — the C++ BASE for the GTA-style bottom-of-screen radar, sibling
 * to UGTCHudWidget and built on the same "C++ caches, WBP renders" contract.
 *
 * Responsibility split:
 *  - This C++ base owns the dynamic STATE: the heading-up projection (FGTCMinimapProjection),
 *    the projected marker list, the projected route polyline, and the derived GPS readout
 *    (distance / ETA / next turn from FGpsNavigation). The caller (the player controller's
 *    tick, or the WBP's NativeTick glue) feeds it the player center+heading, the markers,
 *    the route, and the current speed, then calls Refresh(); the base recomputes everything
 *    and fires OnMinimapUpdated.
 *  - The WBP (deferred to editor-open) renders it: a rotating street image (panned/rotated by
 *    PlayerWorld2D + HeadingDegrees over the SceneCapture render target), one icon per
 *    ProjectedMarker, the route line from ProjectedRoute, the fixed center arrow, and the
 *    RouteEtaText / NextTurnText labels.
 *
 * Frame note: markers and the route arrive as native UE world FVectors (Z-up, XY ground).
 * The projection is native UE. The route math (FGpsNavigation) still lives in the Godot XZ
 * frame, so Refresh feeds it through a local UE->nav adapter — the one place the deferred
 * "Wave-3 axis remap" actually bites, handled here rather than in the shared pure-core.
 *
 * DEFERRED-TO-EDITOR (not part of this headless base):
 *  - the visual WBP, the rotating street material over the SceneCapture render target, the
 *    marker-icon pool, and the route-line spline.
 *  - the SceneCapture2D / render-target manager that draws the actual city streets (the
 *    "streets from world geometry" layer) — a runtime/actor concern, not this base's.
 */
UCLASS()
class GTC_UE5_API UGTCMinimapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // --- Tuning ----------------------------------------------------------------

    /** World half-extent (cm) the radar radius covers. 12000 = 120 m radius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Minimap")
    float RangeCm = 12000.0f;

    /** Turn the route must bend by to count as a "next turn", in degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Minimap")
    float TurnThresholdDegrees = 25.0f;

    // --- Drive it (caller feeds state, then Refresh) ---------------------------

    /** Set the radar center (player world pos) and heading (UE control yaw, degrees). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Minimap")
    void SetCenterAndHeading(const FVector& Center, float HeadingDeg);

    /** Replace the marker set to draw next Refresh (POIs, NPCs, objective). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Minimap")
    void SetMarkers(const TArray<FGTCMinimapMarker>& InMarkers);

    /** Set the active route polyline (world waypoints, last = destination). Empty clears it. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Minimap")
    void SetRoute(const TArray<FVector>& WorldPolyline);

    /** Current player ground speed (cm/s) — drives the ETA. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Minimap")
    void SetPlayerSpeed(float SpeedCmS);

    /** Recompute projected markers + route + GPS readout from the fed state, then fire
     *  OnMinimapUpdated. Cheap and idempotent — safe to call every frame. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Minimap")
    void Refresh();

    // --- Cached output (the WBP renders these) ---------------------------------

    /** Player world XY — pan offset for the rotating street material. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    FVector2D PlayerWorld2D = FVector2D::ZeroVector;

    /** Heading in degrees — rotation for the street material and the north tick. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    float HeadingDegrees = 0.0f;

    /** Markers projected onto the disc this Refresh (off-range non-edge markers culled). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    TArray<FGTCMinimapProjectedMarker> ProjectedMarkers;

    /** The active route as normalized disc points (unclamped; WBP clips to the disc). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    TArray<FVector2D> ProjectedRoute;

    /** True when a drawable route (>= 2 waypoints) is set. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    bool bHasRoute = false;

    /** Along-route distance remaining to the destination, in meters. 0 with no route. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    float RouteDistanceM = 0.0f;

    /** Formatted ETA, "M:SS" (or "--" when stopped / no route). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    FString RouteEtaText;

    /** Next-turn cue, e.g. "Left 120 m" (empty when the route runs straight from here). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Minimap")
    FString NextTurnText;

    // --- WBP render hook -------------------------------------------------------

    /** Fired after Refresh recomputes the cached output. The WBP repaints from it. */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Minimap")
    void OnMinimapUpdated();

private:
    /** Fed state, consumed by Refresh. */
    FVector CenterWorld = FVector::ZeroVector;
    float HeadingRad = 0.0f;
    float PlayerSpeedCmS = 0.0f;
    TArray<FGTCMinimapMarker> Markers;
    TArray<FVector> RouteWorld;
};
