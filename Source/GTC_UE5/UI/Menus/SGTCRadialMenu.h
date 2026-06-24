// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateBrush.h"

/**
 * One entry on a radial wheel. Generic enough for any picker — the weapon wheel
 * fills these from FWeaponStats, the emote wheel from the emote table — so a single
 * widget serves both (and anything later: vehicles, outfits, targets).
 */
struct FGTCRadialItem
{
    /** Big label shown in the hub when this slice is highlighted. */
    FText Name;

    /** Muted blurb shown in the hub, word-wrapped (the "center description"). */
    FText Description;

    /** Tiny uppercase caption above the hub name — the class/mood ("AUTOMATIC"). */
    FString Caption;

    /** Styled counter under the hub name (weapons: "30 / 90"); empty hides it. */
    FString Ammo;

    /** Short glyph/emoji/letter drawn on the slice when there is no Icon (optional). */
    FString Glyph;

    /** Optional slice icon; drawn on the slice when its resource object is set,
     *  otherwise the Glyph/Name text is used. Lets weapon/emote art drop in later. */
    FSlateBrush Icon;

    /** Slice highlight tint — lets weapon classes / emote moods read at a glance. */
    FLinearColor Accent = FLinearColor(0.16f, 0.55f, 1.0f);

    /** A disabled slice is dimmed and cannot be chosen (e.g. a weapon with no ammo). */
    bool bEnabled = true;
};

/**
 * Live bridge from the wheel to whatever owns the choices. The controller fills
 * these with lambdas that re-resolve the pawn each call (so the wheel survives a
 * respawn), mirroring FGTCEmoteServices / FGTCPhoneServices / FGTCCreatorServices.
 */
struct FGTCRadialServices
{
    /** Heading drawn above the wheel ("WEAPONS", "EMOTES"). */
    FText Title;

    /** The slices, in wheel order (slice 0 sits at 12 o'clock, running clockwise). */
    TArray<FGTCRadialItem> Items;

    /** Slice highlighted when the wheel opens (e.g. the currently-equipped weapon). */
    int32 InitialIndex = 0;

    /** Commit a choice: the chosen slice index. The wheel calls Close itself after. */
    TFunction<void(int32 /*Index*/)> Confirm;

    /** Close the wheel without committing (cancel), and after a confirmed pick. */
    TFunction<void()> Close;
};

/**
 * SGTCRadialMenu — a GTA-style radial (wheel) picker, drawn from scratch so it
 * needs no UMG assets and compiles in the headless, editor-closed build.
 *
 * Look: a focal vignette seats the wheel over the world; filled donut wedges fan
 * out from a center hub with anti-aliased segment borders; the highlighted wedge
 * pops outward in its accent colour with a bright rim and an enlarged label; the
 * hub shows the item's caption, name, ammo and a wrapped description. Everything —
 * radius, fonts, offsets — scales with the viewport, and the wheel eases in on open.
 *
 * Two ways to drive it, both supported so the controller can pick the feel:
 *   - HOLD-and-RELEASE (the console-shooter default): the controller holds the
 *     wheel open while a key/bumper is down, the player points (mouse or right
 *     stick) to highlight, and the controller calls ConfirmHighlighted() on release.
 *   - CLICK / NUMBER KEY: left-click a slice, or press its number (1..9); Esc or
 *     right-click cancels.
 *
 * The wheel is purely cosmetic + input glue: all slice geometry / hit-testing is
 * the unit-tested pure core GtcRadial (UI/Radial/RadialMenu), so layout and
 * selection can never disagree. The hub copy comes straight from the services, so
 * the same widget reskins to any picker with zero new Slate.
 */
class GTC_UE5_API SGTCRadialMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGTCRadialMenu) {}
        SLATE_ARGUMENT(FGTCRadialServices, Services)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Focusable so number keys / Enter / Esc land while the wheel is up.
    virtual bool SupportsKeyboardFocus() const override { return true; }

    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

    /** The slice the pointer is currently over (INDEX_NONE in the neutral hub). */
    int32 GetHighlightedIndex() const { return Highlighted; }

    /** Steer the highlight from an analog axis (-1..1 each), e.g. the right stick.
     *  Ignored inside the stick dead zone so a resting stick doesn't twitch the
     *  selection. Lets a gamepad point the wheel without a mouse cursor. */
    void SetAnalogDirection(FVector2D StickXY);

    /** Commit the current highlight (hold-and-release flow). Cancels if the hub is
     *  neutral or the highlighted slice is disabled. */
    void ConfirmHighlighted();

private:
    FGTCRadialServices Services;

    /** Currently highlighted slice, or INDEX_NONE for the neutral center hub. */
    int32 Highlighted = INDEX_NONE;

    /** Seconds since the wheel opened — drives the ease-in. Advanced by an active
     *  timer (which also forces a repaint each frame). */
    double Elapsed = 0.0;

    /** Per-slice highlight amount in [0,1], eased toward 1 for the selected slice and
     *  0 for the rest every frame. This is what makes selection GLIDE (brightness,
     *  pop and rim all cross-fade) instead of snapping — the classic GTA feel. */
    TArray<float> SegT;

    /** Per-frame animation tick (active timer): advance Elapsed + ease SegT, repaint. */
    EActiveTimerReturnType ActiveTick(double InCurrentTime, float InDeltaTime);

    /** Commit slice Index (if enabled), then close. Out-of-range/disabled cancels. */
    void Choose(int32 Index);

    /** Close without committing. */
    void Cancel();

    /** Geometry the paint + hit-test share: wheel center + inner/outer radii. */
    void Metrics(const FGeometry& Geo, FVector2D& OutCenter, double& OutInner, double& OutOuter) const;
};
