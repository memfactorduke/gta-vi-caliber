// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GTCPauseMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture;
class UVerticalBox;
class UWidgetSwitcher;

/**
 * GT-Caliber pause menu — a GTA-style full-screen TABBED pause built entirely in C++
 * (no authored UMG asset). Layout:
 *
 *   ┌───────────────────────────────────────────────────────────┐
 *   │  GT-CALIBER                              $ 12,500   07:07   │  header
 *   │  MAP   STATS   SETTINGS   GAME                              │  tab strip
 *   │ ┌───────────────────────────────────────────────────────┐ │
 *   │ │  (active tab content — map / stats / settings / game)  │ │  switcher
 *   │ └───────────────────────────────────────────────────────┘ │
 *   │  ◄ ► NAVIGATE      ENTER SELECT      ESC RESUME            │  hint bar
 *   └───────────────────────────────────────────────────────────┘
 *
 * Built in C++ on purpose: at runtime the tab/action buttons are real clickable UButtons
 * with bound OnClicked handlers — UE 5.8 cannot script-create clickable widgets into a
 * WidgetBlueprint (the "GUID wall"), so a tabbed nav is only practical from code.
 *
 * Shown by the live controller via CreateWidget<UGTCPauseMenuWidget>() + AddToViewport()
 * (replacing WB_Pause_menu_widget). The widget owns resume / load / save / restart /
 * main-menu / quit and drives settings through UGameUserSettings, so it is self-contained:
 * the only thing the owner must do is create it, add it to the viewport, pause, and set a
 * UI input mode. Resume tears all of that back down itself.
 *
 * Palette: dark-glass fill + cyan outline (matches the polished WB_* pause family); type is
 * the ammo HUD font (/Game/UI/Fonts/DIN_Condensed_Bold_Font) with a graceful engine-font
 * fallback.
 */
UCLASS()
class GTC_UE5_API UGTCPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Optional MAP-tab image. Feed the live minimap render target (or any map texture) from
	 *  BP/owner; if null the MAP tab shows a styled placeholder + live player coords. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Pause")
	TObjectPtr<UTexture> MapTexture = nullptr;

	/** Level to travel to for "MAIN MENU". If None, the Main Menu action is hidden. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Pause")
	FName MainMenuLevelName = NAME_None;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// ---- handles to widgets we update at runtime ----
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> ContentSwitcher = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MoneyText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ClockText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UImage> MapImage = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MapHintText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatsMoneyText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatsTimeText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> QualityValueText = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FullscreenValueText = nullptr;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> TabButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TabLabels;

	int32 ActiveTab = 0;
	int32 QualityLevel = 3;   // 0..3 = Low/Medium/High/Epic (UGameUserSettings scalability)
	bool bFullscreen = true;

	// ---- builders ----
	void BuildUI();
	UVerticalBox* BuildMapTab();
	UVerticalBox* BuildStatsTab();
	UVerticalBox* BuildSettingsTab();
	UVerticalBox* BuildGameTab();
	UButton* AddTab(UHorizontalBox* Strip, const FText& Label, int32 Index);
	UButton* AddActionButton(UVerticalBox* Col, const FText& Label, bool bHero);
	UTextBlock* AddStatRow(UVerticalBox* Col, const FText& Caption);

	void SelectTab(int32 Index);
	void RefreshHeader();
	void RefreshSettingsLabels();
	void ApplyResolutionScaleSettings();

	// ---- click handlers (UFUNCTION so UButton::OnClicked can bind) ----
	UFUNCTION() void OnTabMap();
	UFUNCTION() void OnTabStats();
	UFUNCTION() void OnTabSettings();
	UFUNCTION() void OnTabGame();
	UFUNCTION() void OnResumeClicked();
	UFUNCTION() void OnLoadClicked();
	UFUNCTION() void OnSaveClicked();
	UFUNCTION() void OnRestartClicked();
	UFUNCTION() void OnMainMenuClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnQualityPrev();
	UFUNCTION() void OnQualityNext();
	UFUNCTION() void OnFullscreenToggle();
	UFUNCTION() void OnApplySettings();
};
