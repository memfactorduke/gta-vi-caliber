// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * Live bridge from the Forge panel to the world's UGTCCharacterForge. The console
 * toggle fills these with lambdas that re-resolve the subsystem each call, so the
 * panel survives a world change (mirrors FGTCCreatorServices / FGTCPhoneServices).
 * Each returns a human-readable result string the panel shows in its readout.
 */
struct FGTCForgeServices
{
	/** Probe the skeleton at Path for Role and return the wiring plan as text. */
	TFunction<FString(const FString& /*Path*/, const FString& /*Role*/)> Inspect;

	/** Wire the live player pawn from the skeleton at Path; return what changed. */
	TFunction<FString(const FString& /*Path*/, const FString& /*Role*/)> Apply;

	/** Spawn a fresh wired pawn from the skeleton at Path; return the result. */
	TFunction<FString(const FString& /*Path*/, const FString& /*Role*/)> Spawn;
};

/**
 * SGTCForgePanel — the "editor on the side" for the Character Forge. A creator
 * types a skeleton asset path and a role, then Inspect (see what auto-wires),
 * Apply (re-wire the player), or Spawn (drop a fresh wired NPC). The readout
 * shows the plan checklist returned by the services. Minimal Slate, matching
 * SGTCCharacterCreator / the C++-driven front-end.
 */
class GTC_UE5_API SGTCForgePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGTCForgePanel) {}
		SLATE_ARGUMENT(FGTCForgeServices, Services)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	FGTCForgeServices Services;

	FString CurrentPath;
	FString CurrentRole = TEXT("pedestrian");
	FString LastResult = TEXT("Type a skeletal mesh path and a role, then Inspect / Apply / Spawn.");

	/** Run one service (if bound) and capture its text into the readout. */
	FReply RunService(const TFunction<FString(const FString&, const FString&)>& Service);
};
