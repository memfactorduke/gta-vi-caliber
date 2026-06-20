// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * GtcRadial — pure, world-free geometry/selection math for a GTA-style radial
 * (wheel) menu, shared by the weapon wheel and the emote wheel.
 *
 * This is the *assertable* part of the wheel: no UObject, no Slate, no engine
 * context, headless-testable. The Slate widget (UI/Menus/SGTCRadialMenu) calls
 * into these for BOTH where to draw each slice/label AND which slice the pointer
 * (or analog stick) is selecting — so layout and hit-testing can never drift out
 * of agreement (round-trip: UnitDirection(SliceCenterAngle(i)) selects back to i).
 *
 * Angle convention (matches screen space and how players read a clock):
 *  - Angles are measured CLOCKWISE from straight up (12 o'clock / "north").
 *    north = 0, east = PI/2, south = PI, west = 3*PI/2; normalized to [0, 2*PI).
 *  - Screen-space directions use X = right, Y = DOWN (Slate's local convention),
 *    so "up" is (0,-1).
 *  - Slice 0 is centered at the top and slices run clockwise, the canonical
 *    weapon-wheel layout.
 *
 * All functions are static and GTC-namespaced (`GtcRadial`) so nothing collides
 * with engine symbols. Unit-tested headless (Tests/RadialMenuTest.cpp, prefix
 * GTC.UI.Radial.RadialMenu) and host-verified (Scripts/gtc_radial).
 */
namespace GtcRadial
{
    /**
     * Angular span of one slice (radians) for a wheel of Count items. A wheel of
     * Count<=1 is one full ring (TWO_PI), so a single-item wheel still has a sane,
     * non-degenerate slice.
     */
    GTC_UE5_API double SliceSpan(int32 Count);

    /**
     * Center angle (CW from north, in [0,2*PI)) of slice Index in a Count-slice
     * wheel. Index wraps by Count (so -1 is the last slice); Count<=0 returns 0.
     */
    GTC_UE5_API double SliceCenterAngle(int32 Index, int32 Count);

    /**
     * Unit screen-space direction (X right, Y DOWN) pointing at a CW-from-north
     * angle. Used to place a slice's label/icon: Center + Radius * UnitDirection(a).
     */
    GTC_UE5_API FVector2D UnitDirection(double AngleCW);

    /**
     * CW-from-north angle (normalized [0,2*PI)) of a screen-space direction
     * (X right, Y down). A zero vector returns 0 (north) rather than a NaN.
     */
    GTC_UE5_API double AngleOf(FVector2D Dir);

    /**
     * Nearest slice index for a CW-from-north angle in a Count-slice wheel. The
     * angle is normalized first, so any real value is accepted. Count<=0 returns
     * INDEX_NONE.
     */
    GTC_UE5_API int32 IndexAtAngle(double AngleCW, int32 Count);

    /**
     * Selection from a pointer/stick OFFSET relative to the wheel center (X right,
     * Y down). Returns INDEX_NONE inside the dead zone (|Offset| < DeadZoneRadius)
     * so the center hub is a neutral "no change" area — releasing there cancels
     * without re-equipping, exactly like GTA's wheel. Outside the dead zone it
     * returns the nearest slice (IndexAtAngle). Count<=0 returns INDEX_NONE.
     * A DeadZoneRadius<=0 disables the neutral zone (every offset selects).
     */
    GTC_UE5_API int32 SelectionAt(FVector2D Offset, int32 Count, double DeadZoneRadius);
}
