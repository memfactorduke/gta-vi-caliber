// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure heading-up world->minimap projection — the geometry half of the GTA-style
 * radar at the bottom of the HUD. Given the player's world position (the radar
 * center), the player's heading, and how much world the radar covers, it maps any
 * world point onto the radar disc so a UMG icon can be placed.
 *
 * "Heading-up": the player arrow is pinned at the disc center pointing UP, and the
 * world rotates under it. Forward (where the player faces) is +Y on the disc; the
 * player's right is +X. North-up is just this with Heading == 0.
 *
 * Frame: UE world, Z-up. We reason on the XY ground plane (Z/height is dropped).
 * UE yaw convention: forward = (cos Yaw, sin Yaw) in XY, so yaw 0 faces +X and a
 * positive yaw turns toward +Y. NOTE: this is the UE XY ground plane, NOT the
 * Godot XZ frame that FGpsNavigation (AI/Gps) still lives in — the route math is
 * fed through a local adapter in the widget, this projection stays native UE.
 *
 * Output is NORMALIZED to the disc: X/Y in roughly [-1, 1], where |.| <= 1 is on
 * the radar and > 1 is off it. Pixels are the WBP's concern (multiply by the disc
 * radius). Screen-Y points down in UMG, so the WBP places an icon at
 * (centerPx + N.X * radius, centerPx - N.Y * radius) — the final flip is a render
 * detail kept out of this pure math.
 *
 * All static / value-typed / no UObject — unit-tested headless like FGpsNavigation
 * (Tests/GTCMinimapProjectionTest.cpp, prefix GTC.UI.Minimap).
 */
struct GTC_UE5_API FGTCMinimapProjection
{
    /** Radar center in world space — normally the player pawn location. */
    FVector Center = FVector::ZeroVector;

    /** Player heading in radians (UE yaw): forward = (cos, sin) in the XY plane. */
    double HeadingRadians = 0.0;

    /** World half-extent (cm) the radius of the disc covers. ~12000 = 120 m radius. */
    double RangeCm = 12000.0;

    /**
     * Rotated planar offset of World from Center, in world cm, in the heading-up
     * radar frame: X = player-right, Y = player-forward (up). Height (Z) is dropped.
     */
    FVector2D WorldToLocal(const FVector& World) const;

    /**
     * WorldToLocal divided by RangeCm — normalized to the disc. Unclamped: |.| > 1
     * means the point is beyond the radar range. A non-positive RangeCm yields zero
     * (degenerate radar, never a divide-by-zero).
     */
    FVector2D WorldToNormalized(const FVector& World) const;

    /**
     * Heading-up normalized position with off-range points pulled to the rim (the
     * GTA "objective arrow stuck to the edge" behavior). bOnEdge is set true when the
     * point was outside the disc and got clamped.
     */
    FVector2D Project(const FVector& World, bool& bOnEdge) const;

    /**
     * Clamp a normalized point to the unit disc. Points already inside pass through
     * (bClamped false); points outside are scaled back onto the rim (bClamped true).
     */
    static FVector2D ClampToDisc(const FVector2D& Normalized, bool& bClamped);
};
