// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

#include "../../Player/Pawn/GTCPlayerState.h"
#include "../../Player/Stats/PlayerStats.h"
#include "../../Systems/Save/SaveSubsystem.h"
#include "../../World/Environment/GTCWeatherController.h"

#define LOCTEXT_NAMESPACE "GTCPauseMenu"

// ---- palette (dark-glass + cyan; matches the polished WB_* pause family) --------------------
namespace GTCPause
{
	static const FLinearColor DarkGlass    (0.02f, 0.05f, 0.06f, 0.82f);
	static const FLinearColor PanelGlass   (0.015f, 0.04f, 0.05f, 0.90f);
	static const FLinearColor Cyan         (0.22f, 0.86f, 1.00f, 1.00f);
	static const FLinearColor CyanDim      (0.22f, 0.86f, 1.00f, 0.45f);
	static const FLinearColor TextBright   (0.92f, 0.98f, 1.00f, 1.00f);
	static const FLinearColor TextDim      (0.55f, 0.70f, 0.78f, 1.00f);

	static const TCHAR* QualityNames[4] = { TEXT("LOW"), TEXT("MEDIUM"), TEXT("HIGH"), TEXT("EPIC") };

	// Ammo HUD font, with a graceful engine-font fallback if the asset isn't found.
	static FSlateFontInfo Font(int32 Size, bool bBold = true)
	{
		if (UObject* F = LoadObject<UObject>(nullptr, TEXT("/Game/UI/Fonts/DIN_Condensed_Bold_Font")))
		{
			return FSlateFontInfo(F, Size);
		}
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	static FSlateBrush GlassBrush(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		B.OutlineSettings.Color = Outline;
		B.OutlineSettings.Width = OutlineWidth;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	static FButtonStyle ButtonStyle(const FLinearColor& Normal, const FLinearColor& Hover,
		const FLinearColor& Outline, const FLinearColor& HoverOutline)
	{
		FButtonStyle S;
		S.SetNormal(GlassBrush(Normal, Outline, 10.f, 1.0f));
		S.SetHovered(GlassBrush(Hover, HoverOutline, 10.f, 1.5f));
		S.SetPressed(GlassBrush(Hover, HoverOutline, 10.f, 2.0f));
		S.SetNormalPadding(FMargin(0));
		S.SetPressedPadding(FMargin(0));
		return S;
	}
}

TSharedRef<SWidget> UGTCPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		BuildUI();
	}
	return Super::RebuildWidget();
}

void UGTCPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Self-contained: pause + UI input + cursor so any caller just needs CreateWidget +
	// AddToViewport. OnResumeClicked reverses all three.
	if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::SetGamePaused(PC, true);
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}

	// Seed the settings UI from the live engine settings.
	if (GEngine)
	{
		if (UGameUserSettings* GS = GEngine->GetGameUserSettings())
		{
			QualityLevel = FMath::Clamp(GS->GetOverallScalabilityLevel(), 0, 3);
			bFullscreen = (GS->GetFullscreenMode() != EWindowMode::Windowed);
		}
	}
	RefreshSettingsLabels();
	SelectTab(0);
	RefreshHeader();

	// Be focusable so left/right tab cycling + Esc-to-resume work on keyboard/gamepad.
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void UGTCPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshHeader();
}

FReply UGTCPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		OnResumeClicked();
		return FReply::Handled();
	}
	if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftShoulder)
	{
		SelectTab((ActiveTab + 3) % 4);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_RightShoulder)
	{
		SelectTab((ActiveTab + 1) % 4);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ----------------------------------------------------------------------------------------------
//  Build
// ----------------------------------------------------------------------------------------------

void UGTCPauseMenuWidget::BuildUI()
{
	using namespace GTCPause;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// 1. Blur the paused world.
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(14.f);
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Blur))
	{
		S->SetAnchors(FAnchors(0, 0, 1, 1));
		S->SetOffsets(FMargin(0));
	}

	// 2. Dim/cool the world behind the panel.
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
	Dim->SetBrushColor(FLinearColor(0.0f, 0.012f, 0.025f, 0.55f));
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Dim))
	{
		S->SetAnchors(FAnchors(0, 0, 1, 1));
		S->SetOffsets(FMargin(0));
	}

	// 3. Inset main column (header / tab strip / content / hint), GTA-style near-fullscreen.
	UVerticalBox* MainCol = WidgetTree->ConstructWidget<UVerticalBox>();
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(MainCol))
	{
		S->SetAnchors(FAnchors(0, 0, 1, 1));
		S->SetOffsets(FMargin(120.f, 70.f, 120.f, 60.f)); // L,T,R,B insets
	}

	// --- header ---
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	{
		UTextBlock* Word = WidgetTree->ConstructWidget<UTextBlock>();
		Word->SetText(LOCTEXT("Wordmark", "GT-CALIBER"));
		Word->SetFont(Font(36));
		Word->SetColorAndOpacity(FSlateColor(Cyan));
		if (UHorizontalBoxSlot* HS = Header->AddChildToHorizontalBox(Word))
		{
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HS->SetVerticalAlignment(VAlign_Center);
		}

		MoneyText = WidgetTree->ConstructWidget<UTextBlock>();
		MoneyText->SetFont(Font(26));
		MoneyText->SetColorAndOpacity(FSlateColor(TextBright));
		if (UHorizontalBoxSlot* HS = Header->AddChildToHorizontalBox(MoneyText))
		{
			HS->SetVerticalAlignment(VAlign_Center);
			HS->SetPadding(FMargin(0, 0, 28, 0));
		}

		ClockText = WidgetTree->ConstructWidget<UTextBlock>();
		ClockText->SetFont(Font(26));
		ClockText->SetColorAndOpacity(FSlateColor(TextDim));
		if (UHorizontalBoxSlot* HS = Header->AddChildToHorizontalBox(ClockText))
		{
			HS->SetVerticalAlignment(VAlign_Center);
		}
	}
	if (UVerticalBoxSlot* VS = MainCol->AddChildToVerticalBox(Header))
	{
		VS->SetPadding(FMargin(2, 0, 2, 14));
	}

	// --- tab strip ---
	UHorizontalBox* Strip = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* T0 = AddTab(Strip, LOCTEXT("TabMap", "MAP"), 0);
	UButton* T1 = AddTab(Strip, LOCTEXT("TabStats", "STATS"), 1);
	UButton* T2 = AddTab(Strip, LOCTEXT("TabSettings", "SETTINGS"), 2);
	UButton* T3 = AddTab(Strip, LOCTEXT("TabGame", "GAME"), 3);
	T0->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnTabMap);
	T1->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnTabStats);
	T2->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnTabSettings);
	T3->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnTabGame);
	if (UVerticalBoxSlot* VS = MainCol->AddChildToVerticalBox(Strip))
	{
		VS->SetPadding(FMargin(0, 0, 0, 4));
	}

	// thin accent rule under the strip (2px, full width)
	UBorder* Rule = WidgetTree->ConstructWidget<UBorder>();
	Rule->SetBrushColor(CyanDim);
	Rule->SetPadding(FMargin(0));
	USizeBox* RuleBox = WidgetTree->ConstructWidget<USizeBox>();
	RuleBox->SetHeightOverride(2.0f);
	RuleBox->AddChild(Rule);
	if (UVerticalBoxSlot* VS = MainCol->AddChildToVerticalBox(RuleBox))
	{
		VS->SetPadding(FMargin(0, 0, 0, 14));
		VS->SetHorizontalAlignment(HAlign_Fill);
	}

	// --- content card + switcher ---
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
	Card->SetBrush(GlassBrush(DarkGlass, CyanDim, 16.f, 1.5f));
	Card->SetPadding(FMargin(30.f));

	ContentSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	ContentSwitcher->AddChild(BuildMapTab());
	ContentSwitcher->AddChild(BuildStatsTab());
	ContentSwitcher->AddChild(BuildSettingsTab());
	ContentSwitcher->AddChild(BuildGameTab());
	Card->SetContent(ContentSwitcher);

	if (UVerticalBoxSlot* VS = MainCol->AddChildToVerticalBox(Card))
	{
		VS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// --- hint bar ---
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>();
	Hint->SetText(LOCTEXT("Hint", "◄ ►  NAVIGATE        ENTER  SELECT        ESC  RESUME"));
	Hint->SetFont(Font(16));
	Hint->SetColorAndOpacity(FSlateColor(TextDim));
	if (UVerticalBoxSlot* VS = MainCol->AddChildToVerticalBox(Hint))
	{
		VS->SetPadding(FMargin(2, 16, 2, 0));
		VS->SetHorizontalAlignment(HAlign_Center);
	}
}

UButton* UGTCPauseMenuWidget::AddTab(UHorizontalBox* Strip, const FText& Label, int32 Index)
{
	using namespace GTCPause;

	UButton* B = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	FSlateBrush Empty; Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
	Style.SetNormal(Empty);
	Style.SetHovered(GlassBrush(FLinearColor(0.10f, 0.40f, 0.50f, 0.25f), CyanDim, 8.f, 1.f));
	Style.SetPressed(GlassBrush(FLinearColor(0.12f, 0.45f, 0.55f, 0.35f), Cyan, 8.f, 1.f));
	B->SetStyle(Style);

	UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
	L->SetText(Label);
	L->SetFont(Font(22));
	L->SetColorAndOpacity(FSlateColor(TextDim));
	B->AddChild(L);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->GetContentSlot()))
	{
		BS->SetPadding(FMargin(24.f, 8.f));
	}

	if (UHorizontalBoxSlot* HS = Strip->AddChildToHorizontalBox(B))
	{
		HS->SetPadding(FMargin(0, 0, 10, 0));
	}

	TabButtons.Add(B);
	TabLabels.Add(L);
	return B;
}

UButton* UGTCPauseMenuWidget::AddActionButton(UVerticalBox* Col, const FText& Label, bool bHero)
{
	using namespace GTCPause;

	UButton* B = WidgetTree->ConstructWidget<UButton>();
	const FButtonStyle Style = bHero
		? ButtonStyle(FLinearColor(0.11f, 0.52f, 0.62f, 0.96f), FLinearColor(0.22f, 0.74f, 0.88f, 1.0f),
			FLinearColor(0.38f, 0.93f, 1.0f, 0.9f), FLinearColor(0.5f, 0.97f, 1.0f, 1.0f))
		: ButtonStyle(FLinearColor(0.05f, 0.12f, 0.14f, 0.55f), FLinearColor(0.16f, 0.55f, 0.66f, 0.85f),
			FLinearColor(0.28f, 0.46f, 0.52f, 0.5f), FLinearColor(0.22f, 0.86f, 1.0f, 0.95f));
	B->SetStyle(Style);

	UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
	L->SetText(Label);
	L->SetFont(Font(bHero ? 24 : 20));
	L->SetColorAndOpacity(FSlateColor(TextBright));
	L->SetJustification(ETextJustify::Center);
	B->AddChild(L);
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->GetContentSlot()))
	{
		BS->SetPadding(FMargin(0.f, 13.f));
		BS->SetHorizontalAlignment(HAlign_Center);
	}

	if (UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(B))
	{
		VS->SetPadding(FMargin(0, 6));
		VS->SetHorizontalAlignment(HAlign_Fill);
	}
	return B;
}

UTextBlock* UGTCPauseMenuWidget::AddStatRow(UVerticalBox* Col, const FText& Caption)
{
	using namespace GTCPause;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	UTextBlock* Cap = WidgetTree->ConstructWidget<UTextBlock>();
	Cap->SetText(Caption);
	Cap->SetFont(Font(20));
	Cap->SetColorAndOpacity(FSlateColor(TextDim));
	if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(Cap))
	{
		HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HS->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* Val = WidgetTree->ConstructWidget<UTextBlock>();
	Val->SetFont(Font(24));
	Val->SetColorAndOpacity(FSlateColor(TextBright));
	if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(Val))
	{
		HS->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(Row))
	{
		VS->SetPadding(FMargin(0, 12));
	}
	return Val;
}

UVerticalBox* UGTCPauseMenuWidget::BuildMapTab()
{
	using namespace GTCPause;

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();

	MapImage = WidgetTree->ConstructWidget<UImage>();
	{
		FSlateBrush MapBrush;
		if (MapTexture)
		{
			MapBrush.SetResourceObject(MapTexture);
			MapBrush.DrawAs = ESlateBrushDrawType::Image;
			MapBrush.ImageSize = FVector2D(1024.f, 1024.f);
		}
		else
		{
			MapBrush = GlassBrush(FLinearColor(0.04f, 0.08f, 0.10f, 1.0f), CyanDim, 12.f, 1.f);
		}
		MapImage->SetBrush(MapBrush);
	}
	if (UVerticalBoxSlot* VS = V->AddChildToVerticalBox(MapImage))
	{
		VS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		VS->SetHorizontalAlignment(HAlign_Fill);
		VS->SetVerticalAlignment(VAlign_Fill);
	}

	MapHintText = WidgetTree->ConstructWidget<UTextBlock>();
	MapHintText->SetText(LOCTEXT("MapHint", "MAP"));
	MapHintText->SetFont(Font(18));
	MapHintText->SetColorAndOpacity(FSlateColor(TextDim));
	if (UVerticalBoxSlot* VS = V->AddChildToVerticalBox(MapHintText))
	{
		VS->SetPadding(FMargin(0, 10, 0, 0));
		VS->SetHorizontalAlignment(HAlign_Center);
	}
	return V;
}

UVerticalBox* UGTCPauseMenuWidget::BuildStatsTab()
{
	using namespace GTCPause;

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
	StatsMoneyText = AddStatRow(V, LOCTEXT("StatCash", "CASH"));
	StatsTimeText = AddStatRow(V, LOCTEXT("StatTime", "IN-GAME TIME"));
	return V;
}

UVerticalBox* UGTCPauseMenuWidget::BuildSettingsTab()
{
	using namespace GTCPause;

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();

	// Quality row:  [<]   QUALITY   <value>   [>]
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		UButton* Prev = WidgetTree->ConstructWidget<UButton>();
		Prev->SetStyle(ButtonStyle(FLinearColor(0.05f, 0.12f, 0.14f, 0.55f), FLinearColor(0.16f, 0.55f, 0.66f, 0.85f),
			FLinearColor(0.28f, 0.46f, 0.52f, 0.5f), FLinearColor(0.22f, 0.86f, 1.0f, 0.95f)));
		UTextBlock* PrevL = WidgetTree->ConstructWidget<UTextBlock>();
		PrevL->SetText(FText::FromString(TEXT("◄")));
		PrevL->SetFont(Font(22));
		PrevL->SetColorAndOpacity(FSlateColor(TextBright));
		Prev->AddChild(PrevL);
		if (UButtonSlot* BS = Cast<UButtonSlot>(Prev->GetContentSlot())) { BS->SetPadding(FMargin(16, 8)); }
		Prev->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnQualityPrev);
		if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(Prev)) { HS->SetVerticalAlignment(VAlign_Center); }

		UTextBlock* Cap = WidgetTree->ConstructWidget<UTextBlock>();
		Cap->SetText(LOCTEXT("OptQuality", "QUALITY"));
		Cap->SetFont(Font(20));
		Cap->SetColorAndOpacity(FSlateColor(TextDim));
		if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(Cap))
		{
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HS->SetVerticalAlignment(VAlign_Center);
			HS->SetPadding(FMargin(20, 0));
		}

		QualityValueText = WidgetTree->ConstructWidget<UTextBlock>();
		QualityValueText->SetFont(Font(22));
		QualityValueText->SetColorAndOpacity(FSlateColor(Cyan));
		if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(QualityValueText))
		{
			HS->SetVerticalAlignment(VAlign_Center);
			HS->SetPadding(FMargin(20, 0));
		}

		UButton* Next = WidgetTree->ConstructWidget<UButton>();
		Next->SetStyle(ButtonStyle(FLinearColor(0.05f, 0.12f, 0.14f, 0.55f), FLinearColor(0.16f, 0.55f, 0.66f, 0.85f),
			FLinearColor(0.28f, 0.46f, 0.52f, 0.5f), FLinearColor(0.22f, 0.86f, 1.0f, 0.95f)));
		UTextBlock* NextL = WidgetTree->ConstructWidget<UTextBlock>();
		NextL->SetText(FText::FromString(TEXT("►")));
		NextL->SetFont(Font(22));
		NextL->SetColorAndOpacity(FSlateColor(TextBright));
		Next->AddChild(NextL);
		if (UButtonSlot* BS = Cast<UButtonSlot>(Next->GetContentSlot())) { BS->SetPadding(FMargin(16, 8)); }
		Next->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnQualityNext);
		if (UHorizontalBoxSlot* HS = Row->AddChildToHorizontalBox(Next)) { HS->SetVerticalAlignment(VAlign_Center); }

		if (UVerticalBoxSlot* VS = V->AddChildToVerticalBox(Row)) { VS->SetPadding(FMargin(0, 12)); }
	}

	// Fullscreen toggle row
	{
		UButton* Toggle = WidgetTree->ConstructWidget<UButton>();
		Toggle->SetStyle(ButtonStyle(FLinearColor(0.05f, 0.12f, 0.14f, 0.55f), FLinearColor(0.16f, 0.55f, 0.66f, 0.85f),
			FLinearColor(0.28f, 0.46f, 0.52f, 0.5f), FLinearColor(0.22f, 0.86f, 1.0f, 0.95f)));

		UHorizontalBox* Inner = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* Cap = WidgetTree->ConstructWidget<UTextBlock>();
		Cap->SetText(LOCTEXT("OptFullscreen", "FULLSCREEN"));
		Cap->SetFont(Font(20));
		Cap->SetColorAndOpacity(FSlateColor(TextDim));
		if (UHorizontalBoxSlot* HS = Inner->AddChildToHorizontalBox(Cap))
		{
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HS->SetVerticalAlignment(VAlign_Center);
		}
		FullscreenValueText = WidgetTree->ConstructWidget<UTextBlock>();
		FullscreenValueText->SetFont(Font(22));
		FullscreenValueText->SetColorAndOpacity(FSlateColor(Cyan));
		if (UHorizontalBoxSlot* HS = Inner->AddChildToHorizontalBox(FullscreenValueText))
		{
			HS->SetVerticalAlignment(VAlign_Center);
		}
		Toggle->AddChild(Inner);
		if (UButtonSlot* BS = Cast<UButtonSlot>(Toggle->GetContentSlot())) { BS->SetPadding(FMargin(20, 11)); }
		Toggle->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnFullscreenToggle);

		if (UVerticalBoxSlot* VS = V->AddChildToVerticalBox(Toggle)) { VS->SetPadding(FMargin(0, 12)); }
	}

	// Apply (hero)
	{
		UButton* Apply = AddActionButton(V, LOCTEXT("OptApply", "APPLY"), true);
		Apply->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnApplySettings);
	}
	return V;
}

UVerticalBox* UGTCPauseMenuWidget::BuildGameTab()
{
	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();

	UButton* Resume = AddActionButton(V, LOCTEXT("Resume", "RESUME"), true);
	Resume->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnResumeClicked);

	UButton* Load = AddActionButton(V, LOCTEXT("Load", "LOAD GAME"), false);
	Load->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnLoadClicked);

	UButton* Save = AddActionButton(V, LOCTEXT("Save", "SAVE GAME"), false);
	Save->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnSaveClicked);

	UButton* Restart = AddActionButton(V, LOCTEXT("Restart", "RESTART LEVEL"), false);
	Restart->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnRestartClicked);

	if (MainMenuLevelName != NAME_None)
	{
		UButton* Menu = AddActionButton(V, LOCTEXT("MainMenu", "MAIN MENU"), false);
		Menu->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnMainMenuClicked);
	}

	UButton* Quit = AddActionButton(V, LOCTEXT("Quit", "QUIT GAME"), false);
	Quit->OnClicked.AddDynamic(this, &UGTCPauseMenuWidget::OnQuitClicked);

	return V;
}

// ----------------------------------------------------------------------------------------------
//  State refresh
// ----------------------------------------------------------------------------------------------

void UGTCPauseMenuWidget::SelectTab(int32 Index)
{
	using namespace GTCPause;

	ActiveTab = FMath::Clamp(Index, 0, 3);
	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(ActiveTab);
	}
	for (int32 i = 0; i < TabLabels.Num(); ++i)
	{
		if (TabLabels[i])
		{
			TabLabels[i]->SetColorAndOpacity(FSlateColor(i == ActiveTab ? Cyan : TextDim));
		}
	}
}

void UGTCPauseMenuWidget::RefreshHeader()
{
	int32 Money = 0;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AGTCPlayerState* PS = PC->GetPlayerState<AGTCPlayerState>())
		{
			if (PS->StatsComponent)
			{
				Money = PS->StatsComponent->GetMoney();
			}
		}
	}
	const FText MoneyFmt = FText::FromString(FString::Printf(TEXT("$ %s"), *FText::AsNumber(Money).ToString()));
	if (MoneyText) { MoneyText->SetText(MoneyFmt); }
	if (StatsMoneyText) { StatsMoneyText->SetText(MoneyFmt); }

	float Hours = 12.f;
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AGTCWeatherController> It(W); It; ++It)
		{
			Hours = It->GetTimeOfDay();
			break;
		}
	}
	const int32 H = ((int32)Hours) % 24;
	const int32 M = FMath::Clamp((int32)((Hours - FMath::FloorToInt(Hours)) * 60.f), 0, 59);
	const FText TimeFmt = FText::FromString(FString::Printf(TEXT("%02d:%02d"), H, M));
	if (ClockText) { ClockText->SetText(TimeFmt); }
	if (StatsTimeText) { StatsTimeText->SetText(TimeFmt); }
}

void UGTCPauseMenuWidget::RefreshSettingsLabels()
{
	const int32 Q = FMath::Clamp(QualityLevel, 0, 3);
	if (QualityValueText)
	{
		QualityValueText->SetText(FText::FromString(GTCPause::QualityNames[Q]));
	}
	if (FullscreenValueText)
	{
		FullscreenValueText->SetText(FText::FromString(bFullscreen ? TEXT("ON") : TEXT("OFF")));
	}
}

void UGTCPauseMenuWidget::ApplyResolutionScaleSettings()
{
	if (!GEngine) { return; }
	if (UGameUserSettings* GS = GEngine->GetGameUserSettings())
	{
		GS->SetOverallScalabilityLevel(FMath::Clamp(QualityLevel, 0, 3));
		GS->SetFullscreenMode(bFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
		GS->ApplySettings(false);
		GS->SaveSettings();
	}
}

// ----------------------------------------------------------------------------------------------
//  Handlers
// ----------------------------------------------------------------------------------------------

void UGTCPauseMenuWidget::OnTabMap()      { SelectTab(0); }
void UGTCPauseMenuWidget::OnTabStats()    { SelectTab(1); }
void UGTCPauseMenuWidget::OnTabSettings() { SelectTab(2); }
void UGTCPauseMenuWidget::OnTabGame()     { SelectTab(3); }

void UGTCPauseMenuWidget::OnResumeClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::SetGamePaused(PC, false);
		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = false;
	}
	RemoveFromParent();
}

void UGTCPauseMenuWidget::OnLoadClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USaveSubsystem* Save = GI->GetSubsystem<USaveSubsystem>())
		{
			Save->LoadFromFile(USaveSubsystem::DefaultSavePath());
		}
	}
}

void UGTCPauseMenuWidget::OnSaveClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USaveSubsystem* Save = GI->GetSubsystem<USaveSubsystem>())
		{
			Save->SaveToFile(USaveSubsystem::DefaultSavePath());
		}
	}
}

void UGTCPauseMenuWidget::OnRestartClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::SetGamePaused(PC, false);
	}
	const FString Level = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!Level.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*Level));
	}
}

void UGTCPauseMenuWidget::OnMainMenuClicked()
{
	if (MainMenuLevelName == NAME_None) { return; }
	if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::SetGamePaused(PC, false);
	}
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void UGTCPauseMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UGTCPauseMenuWidget::OnQualityPrev()
{
	QualityLevel = (QualityLevel + 3) % 4;
	RefreshSettingsLabels();
}

void UGTCPauseMenuWidget::OnQualityNext()
{
	QualityLevel = (QualityLevel + 1) % 4;
	RefreshSettingsLabels();
}

void UGTCPauseMenuWidget::OnFullscreenToggle()
{
	bFullscreen = !bFullscreen;
	RefreshSettingsLabels();
}

void UGTCPauseMenuWidget::OnApplySettings()
{
	ApplyResolutionScaleSettings();
}

#undef LOCTEXT_NAMESPACE
