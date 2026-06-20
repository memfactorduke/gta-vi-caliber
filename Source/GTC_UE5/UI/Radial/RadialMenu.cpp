// Copyright Epic Games, Inc. All Rights Reserved.

#include "RadialMenu.h"

namespace GtcRadial
{
    namespace
    {
        /** Wrap an angle into [0, 2*PI). Handles arbitrary (incl. negative) inputs. */
        double NormalizeAngle(double AngleCW)
        {
            double A = FMath::Fmod(AngleCW, UE_DOUBLE_TWO_PI);
            if (A < 0.0)
            {
                A += UE_DOUBLE_TWO_PI;
            }
            return A;
        }
    }

    double SliceSpan(int32 Count)
    {
        if (Count <= 1)
        {
            return UE_DOUBLE_TWO_PI;
        }
        return UE_DOUBLE_TWO_PI / static_cast<double>(Count);
    }

    double SliceCenterAngle(int32 Index, int32 Count)
    {
        if (Count <= 0)
        {
            return 0.0;
        }
        // Godot-style positive modulo so a negative Index (e.g. -1 == last) wraps.
        const int32 Wrapped = ((Index % Count) + Count) % Count;
        return NormalizeAngle(static_cast<double>(Wrapped) * SliceSpan(Count));
    }

    FVector2D UnitDirection(double AngleCW)
    {
        // CW from north: north (a=0) -> (0,-1); east (a=PI/2) -> (1,0). With Y down,
        // that is (sin a, -cos a).
        return FVector2D(FMath::Sin(AngleCW), -FMath::Cos(AngleCW));
    }

    double AngleOf(FVector2D Dir)
    {
        if (Dir.X == 0.0 && Dir.Y == 0.0)
        {
            return 0.0;
        }
        // Inverse of UnitDirection: a = atan2(x, -y) gives 0 at north, +PI/2 at east.
        return NormalizeAngle(FMath::Atan2(Dir.X, -Dir.Y));
    }

    int32 IndexAtAngle(double AngleCW, int32 Count)
    {
        if (Count <= 0)
        {
            return INDEX_NONE;
        }
        const double Span = SliceSpan(Count);
        const double A = NormalizeAngle(AngleCW);
        // Round to the nearest slice center; the +Count modulo folds the wrap-around
        // at the top (an angle just shy of 2*PI rounds up to Count, i.e. slice 0).
        const int32 Raw = FMath::RoundToInt(A / Span);
        return ((Raw % Count) + Count) % Count;
    }

    int32 SelectionAt(FVector2D Offset, int32 Count, double DeadZoneRadius)
    {
        if (Count <= 0)
        {
            return INDEX_NONE;
        }
        if (DeadZoneRadius > 0.0 && Offset.Size() < DeadZoneRadius)
        {
            return INDEX_NONE; // neutral center hub: no change / cancel-safe
        }
        return IndexAtAngle(AngleOf(Offset), Count);
    }
}
