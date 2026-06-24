// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RadialMenu.h"

/**
 * Behavioural tests for GtcRadial: slices tile the circle evenly from the top
 * clockwise, layout and hit-testing round-trip (a label drawn at slice i's center
 * is the slice you select pointing there), the four cardinal directions land on
 * the expected slices of a 4-wheel, the dead zone neutralises the center hub, and
 * degenerate counts stay safe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcRadialMenuTest,
    "GTC.UI.Radial.RadialMenu",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGtcRadialMenuTest::RunTest(const FString& Parameters)
{
    const double Eps = 1e-6;

    // --- Slice span tiles the circle; single/empty wheels stay non-degenerate ---
    {
        TestEqual(TEXT("4 slices span a quarter-turn each"), GtcRadial::SliceSpan(4), UE_DOUBLE_TWO_PI / 4.0);
        TestEqual(TEXT("8 slices span an eighth-turn each"), GtcRadial::SliceSpan(8), UE_DOUBLE_TWO_PI / 8.0);
        TestEqual(TEXT("a 1-item wheel is one full ring"), GtcRadial::SliceSpan(1), UE_DOUBLE_TWO_PI);
        TestEqual(TEXT("a 0-item wheel is one full ring (no divide-by-zero)"),
            GtcRadial::SliceSpan(0), UE_DOUBLE_TWO_PI);
    }

    // --- Slice 0 sits at the top; slices run clockwise -------------------------
    {
        TestEqual(TEXT("slice 0 is at north (0 rad)"), GtcRadial::SliceCenterAngle(0, 4), 0.0);
        TestEqual(TEXT("slice 1 of 4 is due east (PI/2)"),
            GtcRadial::SliceCenterAngle(1, 4), UE_DOUBLE_PI / 2.0);
        TestEqual(TEXT("slice 2 of 4 is due south (PI)"),
            GtcRadial::SliceCenterAngle(2, 4), UE_DOUBLE_PI);
        TestEqual(TEXT("slice 3 of 4 is due west (3*PI/2)"),
            GtcRadial::SliceCenterAngle(3, 4), 3.0 * UE_DOUBLE_PI / 2.0);
        // Index wraps: -1 is the last slice, Count is slice 0 again.
        TestEqual(TEXT("index -1 wraps to the last slice"),
            GtcRadial::SliceCenterAngle(-1, 4), GtcRadial::SliceCenterAngle(3, 4));
        TestEqual(TEXT("index Count wraps to slice 0"),
            GtcRadial::SliceCenterAngle(4, 4), GtcRadial::SliceCenterAngle(0, 4));
    }

    // --- UnitDirection points where you'd expect (X right, Y down) -------------
    {
        const FVector2D North = GtcRadial::UnitDirection(0.0);
        TestTrue(TEXT("north points up"), FMath::IsNearlyZero(North.X, Eps) && North.Y < -0.99);
        const FVector2D East = GtcRadial::UnitDirection(UE_DOUBLE_PI / 2.0);
        TestTrue(TEXT("east points right"), East.X > 0.99 && FMath::IsNearlyZero(East.Y, Eps));
        const FVector2D South = GtcRadial::UnitDirection(UE_DOUBLE_PI);
        TestTrue(TEXT("south points down"), FMath::IsNearlyZero(South.X, Eps) && South.Y > 0.99);
        const FVector2D West = GtcRadial::UnitDirection(3.0 * UE_DOUBLE_PI / 2.0);
        TestTrue(TEXT("west points left"), West.X < -0.99 && FMath::IsNearlyZero(West.Y, Eps));
    }

    // --- AngleOf inverts UnitDirection -----------------------------------------
    {
        TestTrue(TEXT("up -> 0 (north)"),
            FMath::IsNearlyZero(GtcRadial::AngleOf(FVector2D(0.0, -1.0)), Eps));
        TestTrue(TEXT("right -> PI/2 (east)"),
            FMath::IsNearlyEqual(GtcRadial::AngleOf(FVector2D(1.0, 0.0)), UE_DOUBLE_PI / 2.0, Eps));
        TestTrue(TEXT("down -> PI (south)"),
            FMath::IsNearlyEqual(GtcRadial::AngleOf(FVector2D(0.0, 1.0)), UE_DOUBLE_PI, Eps));
        TestTrue(TEXT("left -> 3*PI/2 (west)"),
            FMath::IsNearlyEqual(GtcRadial::AngleOf(FVector2D(-1.0, 0.0)), 3.0 * UE_DOUBLE_PI / 2.0, Eps));
        // A zero vector is the documented 0, not a NaN.
        TestEqual(TEXT("zero vector -> 0"), GtcRadial::AngleOf(FVector2D::ZeroVector), 0.0);
    }

    // --- Layout <-> hit-test round-trip: drawing at slice i selects slice i -----
    {
        for (int32 Count = 1; Count <= 12; ++Count)
        {
            for (int32 i = 0; i < Count; ++i)
            {
                const double Center = GtcRadial::SliceCenterAngle(i, Count);
                const FVector2D At = GtcRadial::UnitDirection(Center) * 100.0;
                TestEqual(
                    *FString::Printf(TEXT("Count=%d: pointing at slice %d's center selects it"), Count, i),
                    GtcRadial::SelectionAt(At, Count, 10.0), i);
            }
        }
    }

    // --- Cardinal pointer offsets on a 4-wheel ---------------------------------
    {
        const double Far = 80.0; // well outside any sane dead zone
        TestEqual(TEXT("pointing up selects slice 0"),
            GtcRadial::SelectionAt(FVector2D(0.0, -Far), 4, 20.0), 0);
        TestEqual(TEXT("pointing right selects slice 1"),
            GtcRadial::SelectionAt(FVector2D(Far, 0.0), 4, 20.0), 1);
        TestEqual(TEXT("pointing down selects slice 2"),
            GtcRadial::SelectionAt(FVector2D(0.0, Far), 4, 20.0), 2);
        TestEqual(TEXT("pointing left selects slice 3"),
            GtcRadial::SelectionAt(FVector2D(-Far, 0.0), 4, 20.0), 3);
    }

    // --- Wrap-around seam at the top: a hair either side of north is slice 0 ----
    {
        const int32 Count = 6;
        const double Span = GtcRadial::SliceSpan(Count); // 60 deg
        // Just clockwise of north and just counter-clockwise of north both round to 0.
        TestEqual(TEXT("a touch CW of north is still slice 0"),
            GtcRadial::IndexAtAngle(Span * 0.4, Count), 0);
        TestEqual(TEXT("a touch CCW of north (wraps near 2*PI) is still slice 0"),
            GtcRadial::IndexAtAngle(UE_DOUBLE_TWO_PI - Span * 0.4, Count), 0);
        // Past the half-slice boundary it flips to the neighbour.
        TestEqual(TEXT("past the half-slice CW boundary is slice 1"),
            GtcRadial::IndexAtAngle(Span * 0.6, Count), 1);
        TestEqual(TEXT("past the half-slice CCW boundary is the last slice"),
            GtcRadial::IndexAtAngle(UE_DOUBLE_TWO_PI - Span * 0.6, Count), Count - 1);
    }

    // --- Dead zone: the center hub is neutral ----------------------------------
    {
        TestEqual(TEXT("inside the dead zone selects nothing"),
            GtcRadial::SelectionAt(FVector2D(5.0, 0.0), 4, 20.0), INDEX_NONE);
        TestEqual(TEXT("exactly on the dead-zone radius is still neutral (strict <)"),
            GtcRadial::SelectionAt(FVector2D(20.0, 0.0), 4, 20.0), 1);
        TestEqual(TEXT("a zero/negative dead zone disables the neutral hub"),
            GtcRadial::SelectionAt(FVector2D(0.5, 0.0), 4, 0.0), 1);
    }

    // --- Degenerate counts stay safe -------------------------------------------
    {
        TestEqual(TEXT("0-item wheel selects nothing"),
            GtcRadial::SelectionAt(FVector2D(50.0, 0.0), 0, 10.0), INDEX_NONE);
        TestEqual(TEXT("negative count selects nothing"),
            GtcRadial::IndexAtAngle(1.0, -3), INDEX_NONE);
        TestEqual(TEXT("a 1-item wheel always selects slice 0"),
            GtcRadial::SelectionAt(FVector2D(0.0, 50.0), 1, 10.0), 0);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
