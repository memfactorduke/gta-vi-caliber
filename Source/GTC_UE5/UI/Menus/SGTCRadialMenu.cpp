// Copyright Epic Games, Inc. All Rights Reserved.

#include "SGTCRadialMenu.h"

#include "../Radial/RadialMenu.h"
#include "../Radial/GTCRadialMenuSettings.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "GTCRadialMenu"

namespace
{
    // Knobs that are NOT worth exposing in the editor (pure rendering detail). The
    // user-facing look/size/feel lives in UGTCRadialMenuSettings (Project Settings),
    // read live every frame so editing it updates an open wheel instantly.
    constexpr double StickDeadZone = 0.35;  // analog magnitude below which the stick is neutral
    constexpr double SeatGlowScale = 1.75;  // focal vignette radius / outer radius

    // Font sizes + layout offsets are all fractions of the wheel radius, so text
    // scales WITH the wheel (and the viewport); the editor FontScale multiplies them.
    constexpr double FontGlyph = 0.115;  // slice glyph
    constexpr double FontSlice = 0.052;  // slice name
    constexpr double FontHubName = 0.088; // hub headline (highlighted item name)
    constexpr double FontHubCap = 0.040;  // hub caption (class/mood)
    constexpr double FontHubAmmo = 0.058; // hub ammo counter
    constexpr double FontHubDesc = 0.050; // hub description
    constexpr double FontTitle = 0.090;  // wheel title

    FColor Lin(const FLinearColor& C) { return C.ToFColor(/*bSRGB=*/true); }

    /** Scale a colour's alpha by the fade-in factor. */
    FLinearColor Fade(FLinearColor C, double A) { C.A *= (float)A; return C; }

    /** Radius-relative font, floored so it never collapses on a tiny viewport. */
    FSlateFontInfo RadialFont(const ANSICHAR* Typeface, double Radius, double Frac)
    {
        return FCoreStyle::GetDefaultFontStyle(Typeface, FMath::Max(7, FMath::RoundToInt(Radius * Frac)));
    }
}

void SGTCRadialMenu::Construct(const FArguments& InArgs)
{
    Services = InArgs._Services;
    Highlighted = Services.Items.IsValidIndex(Services.InitialIndex) ? Services.InitialIndex : INDEX_NONE;

    // Start with the initial slice already lit so the wheel opens "on" a selection.
    SegT.SetNumZeroed(Services.Items.Num());
    if (SegT.IsValidIndex(Highlighted))
    {
        SegT[Highlighted] = 1.f;
    }

    SetVisibility(EVisibility::Visible); // full-screen overlay; receives pointer + keys

    // Drive the open ease-in + the gliding selection, and force a repaint each frame.
    RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SGTCRadialMenu::ActiveTick));
}

EActiveTimerReturnType SGTCRadialMenu::ActiveTick(double /*InCurrentTime*/, float InDeltaTime)
{
    Elapsed += InDeltaTime;

    // Ease every slice's highlight toward its target (selected = 1, rest = 0). The
    // fast-but-finite interp is what makes the selection slide between segments.
    for (int32 i = 0; i < SegT.Num(); ++i)
    {
        const float Target = (i == Highlighted) ? 1.f : 0.f;
        SegT[i] = FMath::FInterpTo(SegT[i], Target, InDeltaTime, 16.f);
    }

    Invalidate(EInvalidateWidgetReason::Paint);
    return EActiveTimerReturnType::Continue;
}

void SGTCRadialMenu::Metrics(const FGeometry& Geo, FVector2D& OutCenter, double& OutInner, double& OutOuter) const
{
    const UGTCRadialMenuSettings& S = *GetDefault<UGTCRadialMenuSettings>();
    const FVector2D Size = Geo.GetLocalSize();
    OutCenter = FVector2D(Size.X * 0.5, Size.Y * 0.5);
    OutOuter = FMath::Clamp(FMath::Min(Size.X, Size.Y) * (double)S.RadiusFraction,
        (double)S.RadiusMin, (double)FMath::Max(S.RadiusMin, S.RadiusMax));
    OutInner = OutOuter * (double)S.HubFraction;
}

void SGTCRadialMenu::SetAnalogDirection(FVector2D StickXY)
{
    if (StickXY.Size() < StickDeadZone)
    {
        return; // resting stick: hold the current highlight
    }
    // Input stick: up is +Y; screen offset up is -Y. Scale past the spatial dead zone.
    const FVector2D Offset(StickXY.X * 100.0, -StickXY.Y * 100.0);
    const int32 Sel = GtcRadial::SelectionAt(Offset, Services.Items.Num(), /*DeadZoneRadius=*/0.0);
    if (Sel != INDEX_NONE)
    {
        Highlighted = Sel;
    }
}

void SGTCRadialMenu::Choose(int32 Index)
{
    if (Services.Items.IsValidIndex(Index) && Services.Items[Index].bEnabled && Services.Confirm)
    {
        Services.Confirm(Index);
    }
    Cancel(); // close either way (Confirm already committed if it was valid)
}

void SGTCRadialMenu::ConfirmHighlighted()
{
    Choose(Highlighted); // INDEX_NONE / disabled falls through to a plain close
}

void SGTCRadialMenu::Cancel()
{
    if (Services.Close)
    {
        Services.Close();
    }
}

FReply SGTCRadialMenu::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    FVector2D Center;
    double Inner, Outer;
    Metrics(MyGeometry, Center, Inner, Outer);

    const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    const int32 Sel = GtcRadial::SelectionAt(Local - Center, Services.Items.Num(), Inner);
    if (Sel != INDEX_NONE)
    {
        Highlighted = Sel;
    }
    return FReply::Handled();
}

FReply SGTCRadialMenu::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        Cancel();
        return FReply::Handled();
    }
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        FVector2D Center;
        double Inner, Outer;
        Metrics(MyGeometry, Center, Inner, Outer);
        const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        const int32 Sel = GtcRadial::SelectionAt(Local - Center, Services.Items.Num(), Inner);
        if (Sel != INDEX_NONE)
        {
            Choose(Sel); // click a slice -> equip it
        }
        else
        {
            Cancel(); // click the neutral hub -> close
        }
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

FReply SGTCRadialMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Escape)
    {
        Cancel();
        return FReply::Handled();
    }
    if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Virtual_Accept)
    {
        ConfirmHighlighted();
        return FReply::Handled();
    }

    // Number-row 1..9 picks the matching slice directly.
    static const FKey NumKeys[9] = {
        EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
        EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
    };
    for (int32 i = 0; i < 9; ++i)
    {
        if (Key == NumKeys[i] && Services.Items.IsValidIndex(i))
        {
            Choose(i);
            return FReply::Handled();
        }
    }

    return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

int32 SGTCRadialMenu::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");

    // Live editor-tunable look/size/feel (Project Settings ▸ Game ▸ GTC Radial Menu).
    const UGTCRadialMenuSettings& S = *GetDefault<UGTCRadialMenuSettings>();
    const double FontMul = FMath::Max(0.1f, S.FontScale);

    // --- Animation: ease the wheel in (scale + fade), ease-OUT cubic = snappy/classic ---
    const double Intro = (S.IntroTime > 0.f) ? FMath::Clamp(Elapsed / (double)S.IntroTime, 0.0, 1.0) : 1.0;
    const double A = 1.0 - FMath::Pow(1.0 - Intro, 3.0);      // ease-out cubic
    const double AnimScale = FMath::Lerp((double)S.IntroScaleFrom, 1.0, A);

    // Per-slice highlight amount (eased in ActiveTick); falls back to discrete if the
    // array hasn't sized yet. This drives the gliding pop/colour/rim cross-fade.
    auto SegHi = [&](int32 i) -> double
    {
        return SegT.IsValidIndex(i) ? (double)SegT[i] : (i == Highlighted ? 1.0 : 0.0);
    };

    // --- Dim the world behind the wheel (eases in with the wheel) --------------
    FSlateDrawElement::MakeBox(
        OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), White,
        ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, S.BackgroundDim * (float)A));

    if (!FSlateApplication::IsInitialized())
    {
        return LayerId; // no renderer (headless) — nothing to draw
    }

    FVector2D Center;
    double Inner0, Outer0;
    Metrics(AllottedGeometry, Center, Inner0, Outer0);
    const double Outer = Outer0 * AnimScale;
    const double Inner = Inner0 * AnimScale;

    const int32 N = Services.Items.Num();
    const FSlateRenderTransform& RT = AllottedGeometry.GetAccumulatedRenderTransform();
    const FSlateResourceHandle Handle =
        FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White);
    const TSharedRef<FSlateFontMeasure> FM =
        FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    auto MakeVert = [&](FVector2D P, FColor C)
    {
        return FSlateVertex::Make<ESlateVertexRounding::Disabled>(
            RT, FVector2f((float)P.X, (float)P.Y), FVector2f(0.5f, 0.5f), C);
    };
    auto Pt = [&](double Angle, double R) { return Center + GtcRadial::UnitDirection(Angle) * R; };

    const int32 SeatLayer = LayerId + 1;
    const int32 WedgeLayer = LayerId + 2;
    const int32 EdgeLayer = LayerId + 3;
    const int32 GlyphLayer = LayerId + 4;
    const int32 HubLayer = LayerId + 5;
    const int32 TextLayer = LayerId + 6;
    const int32 TopLayer = LayerId + 7;

    // --- Focal vignette: a soft dark disc that seats the wheel over the world ---
    {
        TArray<FSlateVertex> V;
        TArray<SlateIndex> I;
        const double R = Outer * SeatGlowScale;
        const FColor CenterC = Lin(FLinearColor(0.f, 0.f, 0.f, S.VignetteOpacity * (float)A));
        const FColor EdgeC = Lin(FLinearColor(0.f, 0.f, 0.f, 0.f));
        const int32 Ci = V.Num();
        V.Add(MakeVert(Center, CenterC));
        const int32 Steps = 64;
        for (int32 s = 0; s <= Steps; ++s)
        {
            const double Ang = (double)s / (double)Steps * UE_DOUBLE_TWO_PI;
            V.Add(MakeVert(Pt(Ang, R), EdgeC));
            if (s > 0)
            {
                I.Add(Ci); I.Add(Ci + s); I.Add(Ci + s + 1);
            }
        }
        FSlateDrawElement::MakeCustomVerts(OutDrawElements, SeatLayer, Handle, V, I, nullptr, 0, 0);
    }

    // --- Wedges (filled, with a soft inner->outer gradient) --------------------
    if (N > 0)
    {
        TArray<FSlateVertex> Verts;
        TArray<SlateIndex> Indices;
        const double Half = GtcRadial::SliceSpan(N) * 0.5;
        const double Gap = (N > 1) ? FMath::DegreesToRadians((double)S.SliceGapDegrees) : 0.0;
        const double HotScale = S.HotOuterScale;

        for (int32 i = 0; i < N; ++i)
        {
            const FGTCRadialItem& Item = Services.Items[i];
            const double t = Item.bEnabled ? SegHi(i) : 0.0; // disabled never lights
            const double Mid = GtcRadial::SliceCenterAngle(i, N);
            const double A0 = Mid - Half + Gap;
            const double A1 = Mid + Half - Gap;
            const double OuterR = Outer * FMath::Lerp(1.0, HotScale, t); // pop glides in

            FLinearColor OuterCol, InnerCol;
            if (!Item.bEnabled)
            {
                OuterCol = Fade(FLinearColor(0.12f, 0.13f, 0.16f, 0.60f), A);
                InnerCol = Fade(FLinearColor(0.07f, 0.08f, 0.10f, 0.50f), A);
            }
            else
            {
                // Cross-fade base -> accent by the eased highlight amount.
                const FLinearColor BaseO = S.SliceColor;
                const FLinearColor BaseI = (S.SliceColor * 0.42f).CopyWithNewOpacity(S.SliceColor.A * 0.95f);
                const FLinearColor HotO = Item.Accent.CopyWithNewOpacity(1.0f);
                const FLinearColor HotI = (Item.Accent * 0.40f).CopyWithNewOpacity(0.97f);
                OuterCol = Fade(FMath::Lerp(BaseO, HotO, (float)t), A);
                InnerCol = Fade(FMath::Lerp(BaseI, HotI, (float)t), A);
            }

            // Arc-length tessellation: ~16px per segment, so big wheels stay round.
            const int32 Segs = FMath::Clamp(FMath::RoundToInt((A1 - A0) * OuterR / 16.0), 6, 80);
            const int32 Base = Verts.Num();
            for (int32 s = 0; s <= Segs; ++s)
            {
                const double Ang = FMath::Lerp(A0, A1, (double)s / (double)Segs);
                const FVector2D Dir = GtcRadial::UnitDirection(Ang);
                Verts.Add(MakeVert(Center + Dir * Inner, Lin(InnerCol)));
                Verts.Add(MakeVert(Center + Dir * OuterR, Lin(OuterCol)));
                if (s > 0)
                {
                    const int32 P = Base + (s - 1) * 2;
                    const int32 Q = Base + s * 2;
                    Indices.Add(P);     Indices.Add(P + 1); Indices.Add(Q + 1);
                    Indices.Add(P);     Indices.Add(Q + 1); Indices.Add(Q);
                }
            }
        }

        if (Verts.Num() > 0 && Indices.Num() > 0)
        {
            FSlateDrawElement::MakeCustomVerts(
                OutDrawElements, WedgeLayer, Handle, Verts, Indices, nullptr, 0, 0);
        }

        // --- Anti-aliased segment borders (hide facets; the rim glides bright) -----
        for (int32 i = 0; i < N; ++i)
        {
            const FGTCRadialItem& Item = Services.Items[i];
            const double t = Item.bEnabled ? SegHi(i) : 0.0;
            const double Mid = GtcRadial::SliceCenterAngle(i, N);
            const double A0 = Mid - Half + Gap;
            const double A1 = Mid + Half - Gap;
            const double OuterR = Outer * FMath::Lerp(1.0, HotScale, t);
            const int32 Segs = FMath::Clamp(FMath::RoundToInt((A1 - A0) * OuterR / 16.0), 6, 80);

            TArray<FVector2D> Border;
            Border.Add(Pt(A0, Inner));
            Border.Add(Pt(A0, OuterR));
            for (int32 s = 0; s <= Segs; ++s)
            {
                Border.Add(Pt(FMath::Lerp(A0, A1, (double)s / (double)Segs), OuterR));
            }
            Border.Add(Pt(A1, Inner));
            for (int32 s = Segs; s >= 0; --s)
            {
                Border.Add(Pt(FMath::Lerp(A0, A1, (double)s / (double)Segs), Inner));
            }

            // Rest border is a faint hairline; the selected rim brightens + thickens as
            // t glides in, so the highlight sweeps smoothly between segments.
            const FLinearColor RestCol(1.f, 1.f, 1.f, Item.bEnabled ? 0.12f : 0.05f);
            const FLinearColor HotCol = (Item.Accent * 1.6f).CopyWithNewOpacity(1.0f);
            const FLinearColor LineCol = Fade(FMath::Lerp(RestCol, HotCol, (float)t), A);
            const float Thick = (float)FMath::Lerp(FMath::Max(1.0, Outer * 0.005),
                FMath::Max(2.0, Outer * 0.013), t);
            FSlateDrawElement::MakeLines(
                OutDrawElements, EdgeLayer, AllottedGeometry.ToPaintGeometry(), Border,
                ESlateDrawEffect::None, LineCol, true, Thick);
        }
    }

    // Centered-text helper with a drop shadow for legibility over any background.
    auto DrawCentered =
        [&](const FString& Str, const FSlateFontInfo& Font, FVector2D At, const FLinearColor& Tint, int32 Layer)
    {
        if (Str.IsEmpty())
        {
            return;
        }
        const FVector2D Ext = FM->Measure(Str, Font);
        const FVector2D TopLeft = At - Ext * 0.5;
        const double Off = FMath::Max(1.0, (double)Font.Size * 0.08);
        FSlateDrawElement::MakeText(
            OutDrawElements, Layer, AllottedGeometry.ToOffsetPaintGeometry(TopLeft + FVector2D(Off, Off)),
            Str, Font, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.65f * Tint.A));
        FSlateDrawElement::MakeText(
            OutDrawElements, Layer, AllottedGeometry.ToOffsetPaintGeometry(TopLeft),
            Str, Font, ESlateDrawEffect::None, Tint);
    };

    // --- Per-slice icon / glyph + short label ----------------------------------
    if (N > 0)
    {
        const FSlateFontInfo GlyphFont = RadialFont("Bold", Outer, FontGlyph * FontMul);
        const FSlateFontInfo NameFont = RadialFont("Bold", Outer, FontSlice * FontMul);
        const FSlateFontInfo NameHotFont = RadialFont("Bold", Outer, FontSlice * 1.18 * FontMul);
        const double Band = Outer - Inner;
        for (int32 i = 0; i < N; ++i)
        {
            const FGTCRadialItem& Item = Services.Items[i];
            const double t = Item.bEnabled ? SegHi(i) : 0.0;
            const bool bHot = t > 0.5;
            const double Mid = GtcRadial::SliceCenterAngle(i, N);
            const double MidR = (Inner + Outer * FMath::Lerp(1.0, S.HotOuterScale, t)) * 0.5;
            const FVector2D At = Center + GtcRadial::UnitDirection(Mid) * MidR;

            // Label brightens as it rides out with the popping wedge.
            const FLinearColor Tint = Fade(
                !Item.bEnabled ? FLinearColor(0.5f, 0.5f, 0.55f, 0.6f)
                               : FMath::Lerp(FLinearColor(0.84f, 0.87f, 0.92f, 0.95f), FLinearColor::White, (float)t),
                A);

            if (Item.Icon.GetResourceObject() != nullptr)
            {
                // Drop-in art: a square icon sized to the ring band.
                const double IconSize = Band * FMath::Lerp(0.54, 0.62, t);
                const FVector2D TL = At - FVector2D(IconSize * 0.5, IconSize * 0.5);
                FSlateDrawElement::MakeBox(
                    OutDrawElements, GlyphLayer,
                    AllottedGeometry.ToPaintGeometry(FVector2D(IconSize, IconSize), FSlateLayoutTransform(TL)),
                    &Item.Icon, ESlateDrawEffect::None, Tint);
            }
            else if (!Item.Glyph.IsEmpty())
            {
                DrawCentered(Item.Glyph, GlyphFont, At - FVector2D(0, Outer * 0.05), Tint, GlyphLayer);
                DrawCentered(Item.Name.ToString(), bHot ? NameHotFont : NameFont,
                    At + FVector2D(0, Outer * 0.072), Tint, GlyphLayer);
            }
            else
            {
                DrawCentered(Item.Name.ToString(), bHot ? NameHotFont : NameFont, At, Tint, GlyphLayer);
            }
        }
    }

    // --- Center hub: filled disc + accent rim + caption/name/ammo/description --
    {
        const FLinearColor Accent = Services.Items.IsValidIndex(Highlighted)
            ? Services.Items[Highlighted].Accent
            : FLinearColor(0.4f, 0.45f, 0.5f);

        // Hub disc.
        {
            TArray<FSlateVertex> Hub;
            TArray<SlateIndex> HubIdx;
            const double HubR = Inner * 0.97;
            const FColor Edge = Lin(Fade(FLinearColor(0.05f, 0.06f, 0.09f, 0.98f), A));
            const FColor CenterC = Lin(Fade(FLinearColor(0.02f, 0.03f, 0.05f, 0.99f), A));
            const int32 Ci = Hub.Num();
            Hub.Add(MakeVert(Center, CenterC));
            const int32 Steps = 56;
            for (int32 s = 0; s <= Steps; ++s)
            {
                Hub.Add(MakeVert(Pt((double)s / (double)Steps * UE_DOUBLE_TWO_PI, HubR), Edge));
                if (s > 0)
                {
                    HubIdx.Add(Ci); HubIdx.Add(Ci + s); HubIdx.Add(Ci + s + 1);
                }
            }
            FSlateDrawElement::MakeCustomVerts(OutDrawElements, HubLayer, Handle, Hub, HubIdx, nullptr, 0, 0);

            // Accent rim ring ties the hub to the current selection.
            TArray<FVector2D> Rim;
            const int32 RSteps = 72;
            for (int32 s = 0; s <= RSteps; ++s)
            {
                Rim.Add(Pt((double)s / (double)RSteps * UE_DOUBLE_TWO_PI, HubR));
            }
            FSlateDrawElement::MakeLines(
                OutDrawElements, HubLayer, AllottedGeometry.ToPaintGeometry(), Rim,
                ESlateDrawEffect::None, Fade((Accent * 1.3f).CopyWithNewOpacity(0.85f), A), true,
                (float)FMath::Max(1.5, Outer * 0.008));
        }

        const FSlateFontInfo CapFont = RadialFont("Bold", Outer, FontHubCap * FontMul);
        const FSlateFontInfo NameFont = RadialFont("Bold", Outer, FontHubName * FontMul);
        const FSlateFontInfo AmmoFont = RadialFont("Bold", Outer, FontHubAmmo * FontMul);
        const FSlateFontInfo DescFont = RadialFont("Regular", Outer, FontHubDesc * FontMul);

        if (Services.Items.IsValidIndex(Highlighted))
        {
            const FGTCRadialItem& Sel = Services.Items[Highlighted];

            if (!Sel.Caption.IsEmpty())
            {
                DrawCentered(Sel.Caption.ToUpper(), CapFont, Center - FVector2D(0, Outer * 0.165),
                    Fade((Accent * 1.4f).CopyWithNewOpacity(0.95f), A), TextLayer);
            }
            DrawCentered(Sel.Name.ToString(), NameFont, Center - FVector2D(0, Outer * 0.055),
                Fade(FLinearColor::White, A), TextLayer);

            // Thin divider under the name.
            {
                const double HalfW = Inner * 0.46;
                TArray<FVector2D> Div = { Center + FVector2D(-HalfW, Outer * 0.01),
                                          Center + FVector2D(HalfW, Outer * 0.01) };
                FSlateDrawElement::MakeLines(
                    OutDrawElements, TextLayer, AllottedGeometry.ToPaintGeometry(), Div,
                    ESlateDrawEffect::None, Fade(FLinearColor(1.f, 1.f, 1.f, 0.18f), A), true, 1.0f);
            }

            if (!Sel.Ammo.IsEmpty())
            {
                DrawCentered(Sel.Ammo, AmmoFont, Center + FVector2D(0, Outer * 0.075),
                    Fade((Accent * 1.2f).CopyWithNewOpacity(0.98f), A), TextLayer);
            }

            // Word-wrap the brief description to up to two lines inside the hub.
            const FString Desc = Sel.Description.ToString();
            if (!Desc.IsEmpty())
            {
                const double MaxW = Inner * 1.55;
                TArray<FString> Words;
                Desc.ParseIntoArray(Words, TEXT(" "), true);
                TArray<FString> Lines;
                FString Cur;
                for (const FString& W : Words)
                {
                    const FString Try = Cur.IsEmpty() ? W : Cur + TEXT(" ") + W;
                    if (FM->Measure(Try, DescFont).X > MaxW && !Cur.IsEmpty())
                    {
                        Lines.Add(Cur);
                        Cur = W;
                    }
                    else
                    {
                        Cur = Try;
                    }
                    if (Lines.Num() >= 2)
                    {
                        break;
                    }
                }
                if (!Cur.IsEmpty() && Lines.Num() < 2)
                {
                    Lines.Add(Cur);
                }
                const FLinearColor DescTint = Fade(FLinearColor(0.72f, 0.78f, 0.85f, 0.95f), A);
                const double LineH = FM->Measure(TEXT("Ag"), DescFont).Y;
                const double Start = Sel.Ammo.IsEmpty() ? Outer * 0.085 : Outer * 0.17;
                for (int32 L = 0; L < Lines.Num(); ++L)
                {
                    DrawCentered(Lines[L], DescFont, Center + FVector2D(0, Start + L * LineH),
                        DescTint, TextLayer);
                }
            }
        }
        else
        {
            DrawCentered(LOCTEXT("PickHint", "Select").ToString(), DescFont, Center,
                Fade(FLinearColor(0.6f, 0.6f, 0.65f, 0.9f), A), TextLayer);
        }
    }

    // --- Title (top) — clean and restrained, no tutorial hint (classic GTA) -----
    {
        const FSlateFontInfo TitleFont = RadialFont("Bold", Outer, FontTitle * FontMul);
        DrawCentered(Services.Title.ToString(), TitleFont,
            FVector2D(Center.X, Center.Y - Outer - Outer * 0.17),
            Fade(FLinearColor(0.92f, 0.93f, 0.96f, 0.92f), A), TopLayer);
    }

    return TopLayer;
}

#undef LOCTEXT_NAMESPACE
