#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Framework/SlateDelegates.h"

class UMediaTexture;
class SButton;
class STextBlock;

/** Visual phase the front-end is currently presenting. Kept independent of the
 *  controller's enum so the widget has no dependency back on the controller. */
enum class EGTCFrontEndVisual : uint8
{
	Video, // full-screen boot video, menu hidden
	Menu,  // looping video + menu actions
	Loading
};

/**
 * Pure-Slate front-end: a full-screen video layer, a left-anchored neon menu
 * (Start / Settings / Exit), a loading panel, and a black fade overlay. It owns
 * no gameplay state — input and button presses are forwarded to the owning
 * controller through the bound delegates.
 */
class SGTCFrontEnd : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGTCFrontEnd) {}
		SLATE_ARGUMENT(UMediaTexture*, VideoTexture)
		SLATE_EVENT(FSimpleDelegate, OnSkip)
		SLATE_EVENT(FSimpleDelegate, OnStart)
		SLATE_EVENT(FSimpleDelegate, OnSettings)
		SLATE_EVENT(FSimpleDelegate, OnExit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Driven by the controller each frame / on phase changes.
	void SetVisual(EGTCFrontEndVisual InVisual);
	void SetFadeAlpha(float InAlpha) { FadeAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f); }
	void SetLoadingProgress(float InProgress01);

	/** Background world-load progress shown under the intro video. */
	void SetIntroProgress(float InProgress01) { IntroProgress = FMath::Clamp(InProgress01, 0.0f, 1.0f); }
	/** Once true, the intro prompt invites the player to skip. */
	void SetSkipReady(bool bInReady) { bSkipReady = bInReady; }

	// SWidget interface — keyboard/mouse for skip + menu navigation.
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	EVisibility GetMenuVisibility() const;
	EVisibility GetLoadingVisibility() const;
	EVisibility GetIntroVisibility() const;
	TOptional<float> GetIntroPercent() const;
	FText GetIntroPromptText() const;
	FLinearColor GetFadeColor() const;
	FLinearColor GetAccentColor() const;
	FLinearColor GetScrimColor() const;
	TOptional<float> GetLoadingPercent() const;
	FText GetLoadingText() const;

	TSharedRef<SButton> MakeMenuButton(const FText& Label, FSimpleDelegate Clicked, int32 Index);
	void FocusButton(int32 Index);
	void ActivateFocusedButton();

	FSlateBrush VideoBrush;

	FSimpleDelegate OnSkip;
	FSimpleDelegate OnStart;
	FSimpleDelegate OnSettings;
	FSimpleDelegate OnExit;

	TArray<TSharedPtr<SButton>> MenuButtons;
	TArray<FSimpleDelegate> MenuActions;
	int32 FocusedIndex = 0;

	EGTCFrontEndVisual Visual = EGTCFrontEndVisual::Video;
	float FadeAlpha = 0.0f;
	float LoadingProgress = 0.0f;
	double AccentSeconds = 0.0;

	float IntroProgress = 0.0f;
	bool bSkipReady = false;
};
