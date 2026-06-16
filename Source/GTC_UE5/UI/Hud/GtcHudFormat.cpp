// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcHudFormat.h"

namespace GtcHud
{
    FString FormatMoney(int32 Amount)
    {
        // Work on the magnitude so the sign never lands between "$" and the digits.
        // Use int64 for the magnitude so INT32_MIN (whose |value| overflows int32) is safe.
        const bool bNegative = Amount < 0;
        const int64 Magnitude = bNegative ? -static_cast<int64>(Amount) : static_cast<int64>(Amount);

        // Build the digit string, then insert comma group separators from the right.
        FString Digits = FString::Printf(TEXT("%lld"), Magnitude);

        FString Grouped;
        const int32 Len = Digits.Len();
        for (int32 i = 0; i < Len; ++i)
        {
            // Insert a comma before every group of three counted from the right edge,
            // except at the very start of the number.
            if (i > 0 && ((Len - i) % 3) == 0)
            {
                Grouped.AppendChar(TEXT(','));
            }
            Grouped.AppendChar(Digits[i]);
        }

        return FString::Printf(TEXT("%s$%s"), bNegative ? TEXT("-") : TEXT(""), *Grouped);
    }

    FString FormatStars(int32 Stars, int32 MaxStars)
    {
        const int32 Clamped = FMath::Clamp(Stars, 0, FMath::Max(0, MaxStars));
        FString Out;
        for (int32 i = 0; i < Clamped; ++i)
        {
            Out.AppendChar(TEXT('*'));
        }
        return Out;
    }

    double ClampFraction(double Fraction)
    {
        if (FMath::IsNaN(Fraction))
        {
            return 0.0;
        }
        return FMath::Clamp(Fraction, 0.0, 1.0);
    }

    double SafeFraction(double Value, double Maximum)
    {
        if (Maximum <= 0.0)
        {
            return 0.0;
        }
        return ClampFraction(Value / Maximum);
    }
}
