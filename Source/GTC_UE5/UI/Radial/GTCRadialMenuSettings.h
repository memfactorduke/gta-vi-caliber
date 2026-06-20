// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GTCRadialMenuSettings.generated.h"

/**
 * UGTCRadialMenuSettings — live, editor-tunable look & feel for the radial wheel
 * (UI/Menus/SGTCRadialMenu), edited from the Unreal editor (NOT from in-game): it
 * appears under Project Settings ▸ Game ▸ "GTC Radial Menu". The widget reads the
 * CDO (GetDefault) every frame, so dragging a slider in Project Settings updates an
 * open wheel live in PIE — no rebuild, no console.
 *
 * Everything stays DYNAMIC: the radius is a fraction of the smaller viewport side
 * (so it scales with resolution/DPI) and the fonts/offsets scale off that radius;
 * FontScale is just an extra multiplier on top. Values persist to DefaultGame.ini.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "GTC Radial Menu"))
class GTC_UE5_API UGTCRadialMenuSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** Groups the section under the "Game" list in Project Settings. */
    virtual FName GetCategoryName() const override { return TEXT("Game"); }

    // ---- Size (dynamic: radius = clamp(min(viewport W,H) * RadiusFraction, Min, Max)) ----

    /** Outer radius as a fraction of the smaller viewport dimension. Lower = smaller. */
    UPROPERTY(EditAnywhere, config, Category = "Size",
        meta = (ClampMin = "0.05", ClampMax = "0.5", UIMin = "0.08", UIMax = "0.32"))
    float RadiusFraction = 0.18f;

    /** Pixel floor for the outer radius (tiny viewports stay usable). */
    UPROPERTY(EditAnywhere, config, Category = "Size", meta = (ClampMin = "40.0"))
    float RadiusMin = 110.0f;

    /** Pixel cap for the outer radius (keeps it tight on big monitors). */
    UPROPERTY(EditAnywhere, config, Category = "Size", meta = (ClampMin = "80.0"))
    float RadiusMax = 240.0f;

    /** Inner (hub) radius as a fraction of the outer radius. Higher = thinner ring. */
    UPROPERTY(EditAnywhere, config, Category = "Size",
        meta = (ClampMin = "0.2", ClampMax = "0.85"))
    float HubFraction = 0.55f;

    /** Angular gap drawn between adjacent wedges, in degrees. */
    UPROPERTY(EditAnywhere, config, Category = "Size",
        meta = (ClampMin = "0.0", ClampMax = "20.0"))
    float SliceGapDegrees = 2.0f;

    /** Extra multiplier on every (already radius-relative) font. 1 = default. */
    UPROPERTY(EditAnywhere, config, Category = "Size",
        meta = (ClampMin = "0.3", ClampMax = "3.0"))
    float FontScale = 1.0f;

    // ---- Look ----

    /** Opacity of the full-screen darkening behind the wheel. */
    UPROPERTY(EditAnywhere, config, Category = "Look",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BackgroundDim = 0.6f;

    /** Opacity of the focal vignette that seats the wheel over the world. */
    UPROPERTY(EditAnywhere, config, Category = "Look",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VignetteOpacity = 0.45f;

    /** How far the highlighted wedge pops outward (1 = no pop). */
    UPROPERTY(EditAnywhere, config, Category = "Look",
        meta = (ClampMin = "1.0", ClampMax = "1.3"))
    float HotOuterScale = 1.06f;

    /** Base colour of an unselected wedge. */
    UPROPERTY(EditAnywhere, config, Category = "Look")
    FLinearColor SliceColor = FLinearColor(0.15f, 0.18f, 0.23f, 0.94f);

    // ---- Feel ----

    /** Seconds for the open ease-in (scale + fade). 0 = instant. */
    UPROPERTY(EditAnywhere, config, Category = "Feel",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IntroTime = 0.12f;

    /** Wheel scale at the start of the ease-in (grows to 1.0). */
    UPROPERTY(EditAnywhere, config, Category = "Feel",
        meta = (ClampMin = "0.5", ClampMax = "1.0"))
    float IntroScaleFrom = 0.86f;
};
