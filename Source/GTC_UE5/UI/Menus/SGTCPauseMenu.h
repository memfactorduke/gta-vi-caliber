#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Framework/SlateDelegates.h"

class SButton;

/**
 * In-game Esc pause menu, styled to match the boot front-end (SGTCFrontEnd): a
 * dimmed world behind a left-anchored neon column — "PAUSED" kicker, a pulsing
 * magenta accent rail, and large keyboard/gamepad-navigable actions. Sharing the
 * shell palette keeps pausing visually continuous with the intro/menu/loading.
 *
 * Pure Slate, no authored assets. The owning controller binds the actions it
 * supports; unbound ones (e.g. view-mode toggles before the project has a view
 * system) are simply omitted from the column.
 */
class GTC_UE5_API SGTCPauseMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGTCPauseMenu) {}
		SLATE_EVENT(FSimpleDelegate, OnResume)
		SLATE_EVENT(FSimpleDelegate, OnFirstPerson)
		SLATE_EVENT(FSimpleDelegate, OnThirdPerson)
		SLATE_EVENT(FSimpleDelegate, OnMissionEditor)
		SLATE_EVENT(FSimpleDelegate, OnEditCharacter)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Focusable so it can drive keyboard/gamepad nav and catch Esc to resume.
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	FSimpleDelegate OnResume;
	FSimpleDelegate OnFirstPerson;
	FSimpleDelegate OnThirdPerson;
	FSimpleDelegate OnMissionEditor;
	FSimpleDelegate OnEditCharacter;

	/** Add a navigable row to the column; only bound actions get a row. */
	void AddAction(class SVerticalBox* Column, const FText& Label, const FSimpleDelegate& Action);
	void FocusButton(int32 Index);
	void ActivateFocused();

	FLinearColor GetAccentColor() const;

	TArray<TSharedPtr<SButton>> MenuButtons;
	TArray<FSimpleDelegate> MenuActions;
	int32 FocusedIndex = 0;
};
