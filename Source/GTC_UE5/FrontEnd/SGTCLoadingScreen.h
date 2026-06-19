#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Framework/SlateDelegates.h"

class UTexture2D;
class STextBlock;
class SImage;

/**
 * AAA front-end loading screen, styled after the clean GTA-V "story mode" cover:
 * the key art floats centered on a pure-black field (crossfading slowly between
 * pieces), with the game wordmark in the bottom-left and a "LOADING" label plus
 * an animated spinner in the bottom-right. No letterbox, scrims, or grain — just
 * the artwork, the brand, and the spinner.
 *
 * Animates from SWidget::Tick so it stays smooth on the movie-player thread during
 * a blocking world load. The owner feeds progress and flips it to "done"; on
 * dismiss it fades out and fires OnDismissed.
 */
class SGTCLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGTCLoadingScreen) {}
		SLATE_ARGUMENT(TArray<UTexture2D*>, ArtTextures)
		SLATE_EVENT(FSimpleDelegate, OnDismissed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

	void SetTargetProgress(float InTarget) { TargetProgress = FMath::Clamp(InTarget, 0.0f, 1.0f); }
	void SetDone() { bDone = true; TargetProgress = 1.0f; }
	void BeginDismiss() { bDismissing = true; }
	bool IsDismissing() const { return bDismissing; }

private:
	void AdvanceSlide();
	int32 NextArtIndex();

	FSlateColor GetTopSlideColor() const;
	FText GetLoadingText() const;
	EVisibility GetSpinnerVisibility() const;

	FSimpleDelegate OnDismissed;

	// Two stacked art layers; the top (B) crossfades in over the snapshot in A.
	TArray<TSharedPtr<FSlateBrush>> ArtBrushes;
	FSlateBrush SlideBrushA;
	FSlateBrush SlideBrushB;
	float SlideTopAlpha = 1.0f;

	TArray<int32> ShuffleOrder;
	int32 ShufflePos = 0;
	int32 ArtCount = 0;

	float DisplayProgress = 0.0f;
	float TargetProgress = 0.0f;
	bool bDone = false;

	bool bSliding = false;
	float SlideTimer = 0.0f;
	float SlideElapsed = 0.0f;

	bool bDismissing = false;
	bool bDismissed = false;
	float DismissAlpha = 0.0f;

	static constexpr float SlideHold = 8.0f;
	static constexpr float SlideFade = 1.4f;
	static constexpr float DismissTime = 0.6f;
};
