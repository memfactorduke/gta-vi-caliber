// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * GtcHudFormat — pure, world-free formatting/clamp helpers for the GTC HUD.
 *
 * These are the *assertable* HUD logic: no UObject, no Slate, no engine context,
 * headless-testable. The UGTCHudWidget C++ base and the future WBP both call into
 * these so the display strings (wallet, wanted stars) and bar fractions are
 * produced in exactly one place and unit-tested there. All functions are static.
 *
 * Conventions:
 *  - `double` for fraction maths, `int32` for money/star counts (matching the W2
 *    components' GetMoney()/int32 wanted level).
 *  - GTC-prefixed namespace (`GtcHud`) so nothing collides with engine symbols.
 */
namespace GtcHud
{
    /**
     * Format a wallet balance as a grouped dollar string, e.g.
     *   1500   -> "$1,500"
     *   0      -> "$0"
     *   -250   -> "-$250"  (negative sign before the dollar mark)
     *   1000000-> "$1,000,000"
     * Thousands are grouped with commas. The sign is applied to the magnitude so a
     * negative wallet reads "-$250", never "$-250".
     */
    GTC_UE5_API FString FormatMoney(int32 Amount);

    /**
     * Format a wanted level as a star string, e.g.
     *   0 -> ""            (no stars shown when not wanted)
     *   3 -> "***"
     *   5 -> "*****"
     * Negative inputs clamp to 0 (empty). The cap is MaxStars (default 5) so a
     * runaway heat value can't print an unbounded row.
     */
    GTC_UE5_API FString FormatStars(int32 Stars, int32 MaxStars = 5);

    /**
     * Clamp an already-computed bar fraction into [0,1]. NaN maps to 0. This is the
     * final safety net the widget applies before handing a fraction to a ProgressBar
     * (which expects [0,1]); the components already divide, this just guards the edge.
     */
    GTC_UE5_API double ClampFraction(double Fraction);

    /**
     * Compute a bar fill fraction Value/Maximum, clamped to [0,1] and safe against a
     * zero/negative maximum (returns 0). Mirrors the components' own Fraction() so the
     * HUD can derive a fraction from raw value+max when it only has the pair.
     */
    GTC_UE5_API double SafeFraction(double Value, double Maximum);
}
