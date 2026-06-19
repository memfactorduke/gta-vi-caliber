#include "SGTCLoadingScreen.h"

#include "GTCFrontEndPalette.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Math/UnrealMathUtility.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SThrobber.h"

namespace GTCLoadingStyle
{
	static const FLinearColor Black(0.0f, 0.0f, 0.0f, 1.0f);
	static const FLinearColor Wordmark = GTCFrontEndPalette::TextBright;
	static const FLinearColor WordmarkSub = GTCFrontEndPalette::Cyan;     // neon tie-in to the wordmark
	static const FLinearColor LoadingText(0.86f, 0.90f, 0.95f, 0.85f);
}

void SGTCLoadingScreen::Construct(const FArguments& InArgs)
{
	OnDismissed = InArgs._OnDismissed;

	for (UTexture2D* Tex : InArgs._ArtTextures)
	{
		if (Tex)
		{
			TSharedRef<FSlateBrush> Brush = MakeShared<FSlateBrush>();
			Brush->SetResourceObject(Tex);
			Brush->ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
			Brush->DrawAs = ESlateBrushDrawType::Image;
			ArtBrushes.Add(Brush);
		}
	}
	ArtCount = ArtBrushes.Num();
	if (ArtCount > 0)
	{
		const int32 First = NextArtIndex();
		SlideBrushA = *ArtBrushes[First];
		SlideBrushB = *ArtBrushes[First];
	}

	const FSlateFontInfo WordmarkFont = FCoreStyle::GetDefaultFontStyle("Black", 46);
	const FSlateFontInfo SubFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	const FSlateFontInfo LoadingFont = FCoreStyle::GetDefaultFontStyle("Bold", 15);

	ChildSlot
	[
		SNew(SOverlay)
		.Clipping(EWidgetClipping::ClipToBounds)

		// Pure black field.
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[ SNew(SColorBlock).Color(GTCLoadingStyle::Black) ]

		// Centered, floating key art (aspect-preserved, black margins around it).
		// Two layers so a new piece crossfades in over the previous one.
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill).Padding(150.0f, 70.0f, 150.0f, 118.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[ SNew(SImage).Image(&SlideBrushA) ]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[
					SNew(SImage).Image(&SlideBrushB)
					.ColorAndOpacity(this, &SGTCLoadingScreen::GetTopSlideColor)
				]
			]
		]

		// Bottom-left: game wordmark (original branding).
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(82.0f, 0.0f, 0.0f, 54.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(WordmarkFont).ColorAndOpacity(GTCLoadingStyle::Wordmark)
				.Text(NSLOCTEXT("GTC", "Wordmark", "CALIBER"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, -2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(SubFont).ColorAndOpacity(GTCLoadingStyle::WordmarkSub)
				.Text(NSLOCTEXT("GTC", "WordmarkSub", "ISLES"))
			]
		]

		// Bottom-right: LOADING label + animated spinner.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 84.0f, 60.0f)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SGTCLoadingScreen::GetSpinnerVisibility)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				SNew(STextBlock).Font(LoadingFont).ColorAndOpacity(GTCLoadingStyle::LoadingText)
				.Text(this, &SGTCLoadingScreen::GetLoadingText)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SCircularThrobber).Radius(13.0f)
			]
		]
	];
}

int32 SGTCLoadingScreen::NextArtIndex()
{
	if (ArtCount <= 0)
	{
		return 0;
	}
	if (ShufflePos >= ShuffleOrder.Num())
	{
		const int32 Last = ShuffleOrder.Num() > 0 ? ShuffleOrder.Last() : -1;
		ShuffleOrder.Reset();
		for (int32 i = 0; i < ArtCount; ++i)
		{
			ShuffleOrder.Add(i);
		}
		for (int32 i = ShuffleOrder.Num() - 1; i > 0; --i)
		{
			ShuffleOrder.Swap(i, FMath::RandRange(0, i));
		}
		ShufflePos = 0;
		if (ArtCount > 1 && ShuffleOrder[0] == Last)
		{
			ShuffleOrder.Add(ShuffleOrder[0]);
			ShuffleOrder.RemoveAt(0);
		}
	}
	return ShuffleOrder[ShufflePos++];
}

void SGTCLoadingScreen::AdvanceSlide()
{
	if (ArtCount <= 1)
	{
		return;
	}
	SlideBrushA = SlideBrushB;
	SlideBrushB = *ArtBrushes[NextArtIndex()];
	SlideTopAlpha = 0.0f;
	bSliding = true;
	SlideElapsed = 0.0f;
}

void SGTCLoadingScreen::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	DisplayProgress = FMath::FInterpConstantTo(DisplayProgress, TargetProgress, InDeltaTime, 1.4f);

	if (ArtCount > 1)
	{
		if (bSliding)
		{
			SlideElapsed += InDeltaTime;
			SlideTopAlpha = FMath::Clamp(SlideElapsed / SlideFade, 0.0f, 1.0f);
			if (SlideElapsed >= SlideFade)
			{
				bSliding = false;
				SlideTopAlpha = 1.0f;
			}
		}
		else
		{
			SlideTimer += InDeltaTime;
			if (SlideTimer >= SlideHold)
			{
				SlideTimer = 0.0f;
				AdvanceSlide();
			}
		}
	}

	if (bDismissing && !bDismissed)
	{
		DismissAlpha = FMath::Clamp(DismissAlpha + InDeltaTime / DismissTime, 0.0f, 1.0f);
		SetRenderOpacity(1.0f - DismissAlpha);
		if (DismissAlpha >= 1.0f)
		{
			bDismissed = true;
			OnDismissed.ExecuteIfBound();
		}
	}
}

FSlateColor SGTCLoadingScreen::GetTopSlideColor() const
{
	return FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, SlideTopAlpha));
}

FText SGTCLoadingScreen::GetLoadingText() const
{
	return (bDone && DisplayProgress >= 0.999f)
		? NSLOCTEXT("GTC", "LoadReady", "ENTERING CALIBER ISLES")
		: FText::FromString(FString::Printf(TEXT("LOADING   %d%%"), FMath::RoundToInt(DisplayProgress * 100.0f)));
}

EVisibility SGTCLoadingScreen::GetSpinnerVisibility() const
{
	return EVisibility::HitTestInvisible;
}
