#include "SGTCPhone.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/DateTime.h"

#define LOCTEXT_NAMESPACE "GTCPhone"

namespace
{
	// Device geometry (Slate units). Aspect ~ a 6.x" iPhone.
	constexpr float DeviceW = 300.f;
	constexpr float DeviceH = 648.f;
	constexpr float FrameT  = 11.f;
	constexpr float RFrame  = 52.f;
	constexpr float RScreen = 43.f;
	constexpr float ScreenW = DeviceW - 2.f * FrameT;
	constexpr float TopInset = 40.f;   // status-bar band
	constexpr float SidePad  = 14.f;

	FLinearColor Hx(uint32 RGB, float A = 1.f)
	{
		return FLinearColor(((RGB >> 16) & 0xFF) / 255.f, ((RGB >> 8) & 0xFF) / 255.f, (RGB & 0xFF) / 255.f, A);
	}
	FLinearColor Lighten(const FLinearColor& C, float f = 0.12f)
	{
		return FLinearColor(FMath::Min(C.R + f, 1.f), FMath::Min(C.G + f, 1.f), FMath::Min(C.B + f, 1.f), C.A);
	}
	FLinearColor Darken(const FLinearColor& C, float f = 0.12f)
	{
		return FLinearColor(FMath::Max(C.R - f, 0.f), FMath::Max(C.G - f, 0.f), FMath::Max(C.B - f, 0.f), C.A);
	}

	const FLinearColor ColFrame   = Hx(0x202329);
	const FLinearColor ColEdge    = Hx(0x4A4E57);
	const FLinearColor ColAppBg   = Hx(0x0C0D11);
	const FLinearColor ColText    = Hx(0xFFFFFF);
	const FLinearColor ColDim     = Hx(0xFFFFFF, 0.55f);
	const FLinearColor ColCard    = Hx(0xFFFFFF, 0.07f);
	const FLinearColor ColAccent  = Hx(0x2F8BFF);
	const FLinearColor ColGreen   = Hx(0x32D158);
	const FLinearColor ColRed     = Hx(0xFF453A);
	const FLinearColor ColOrange  = Hx(0xFF9F0A);

	FSlateFontInfo Font(int32 Size, const TCHAR* Style = TEXT("Regular"))
	{
		return FCoreStyle::GetDefaultFontStyle(Style, Size);
	}
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

void SGTCPhone::Construct(const FArguments& InArgs)
{
	Services = InArgs._Services;
	OnFullyClosed = InArgs._OnFullyClosed;
	OnCloseRequested = InArgs._OnCloseRequested;

	// Generate all artwork and keep it alive.
	FGTCPhoneArt Art = GTCPhoneArt::Generate();
	AppIconTex = Art.AppIcons;
	WallpaperTex = Art.Wallpaper;
	MapTex = Art.MapBackdrop;
	for (UTexture2D* T : Art.AppIcons) { if (T) OwnedTextures.Add(TStrongObjectPtr<UTexture2D>(T)); }
	if (Art.Wallpaper) OwnedTextures.Add(TStrongObjectPtr<UTexture2D>(Art.Wallpaper));
	if (Art.MapBackdrop) OwnedTextures.Add(TStrongObjectPtr<UTexture2D>(Art.MapBackdrop));

	// Seed some believable starting state.
	Notes = { TEXT("Pick up car from Pay'n'Spray"), TEXT("Meet Lena @ the marina, 9pm") };
	Ledger = { TEXT("Paycheck            +$2,400"), TEXT("Ammu-Nation          -$640") };
	Portfolio = { {TEXT("CALI"), 4}, {TEXT("VICE"), 0}, {TEXT("LFKR"), 12}, {TEXT("BANG"), 0} };

	// Status bar (top), white so it reads on the wallpaper and on dark app screens.
	TSharedRef<SHorizontalBox> Signal = SNew(SHorizontalBox);
	const float barH[4] = { 4.f, 6.f, 8.f, 10.f };
	for (int i = 0; i < 4; ++i)
	{
		Signal->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(0.8f, 0.f)
		[ SNew(SBox).WidthOverride(3.f).HeightOverride(barH[i]) [ SNew(SImage).Image(Rounded(ColText, 1.f)) ] ];
	}

	// Battery glyph.
	TSharedRef<SWidget> Battery =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(22.f).HeightOverride(11.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[ SNew(SImage).Image(RoundedOutline(Hx(0,0), 3.f, ColText, 1.f)) ]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(1.6f,0.f,0.f,0.f)
				[ SNew(SBox).WidthOverride(15.f).HeightOverride(6.5f)[ SNew(SImage).Image(Rounded(ColGreen, 1.5f)) ] ]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1.f,0.f,0.f,0.f)
		[ SNew(SBox).WidthOverride(2.f).HeightOverride(4.f)[ SNew(SImage).Image(Rounded(ColText, 1.f)) ] ];

	TSharedRef<SWidget> StatusBar =
		SNew(SBox).HeightOverride(TopInset).Padding(FMargin(20.f, 12.f, 18.f, 0.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[ SNew(STextBlock).Font(Font(14, TEXT("Bold"))).ColorAndOpacity(ColText).Text(this, &SGTCPhone::GetStatusClock) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Signal ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f,0.f)
			[ SNew(STextBlock).Font(Font(12, TEXT("Bold"))).ColorAndOpacity(ColText).Text(LOCTEXT("5g","5G")) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Battery ]
		];

	ContentHost = SNew(SBox);
	RefreshContent();

	// The screen stack: black base, swappable content, status bar, Dynamic Island, home bar.
	TSharedRef<SWidget> Screen =
		SNew(SOverlay)
		+ SOverlay::Slot()[ SNew(SImage).Image(Rounded(Hx(0x000000), RScreen)) ]
		+ SOverlay::Slot()[ ContentHost.ToSharedRef() ]
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Top)[ StatusBar ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 8.f, 0.f, 0.f)
		[ SNew(SBox).WidthOverride(92.f).HeightOverride(28.f)[ SNew(SImage).Image(Rounded(Hx(0x000000), 14.f)) ] ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 7.f)
		[ SNew(SBox).WidthOverride(118.f).HeightOverride(5.f)[ SNew(SImage).Image(Rounded(Hx(0xFFFFFF, 0.85f), 2.5f)) ] ];

	// The handset: a rounded frame that also swallows stray clicks so tapping the screen
	// never falls through to the dismiss scrim.
	TSharedRef<SWidget> Handset =
		SNew(SBox).WidthOverride(DeviceW).HeightOverride(DeviceH)
		[
			SNew(SBorder)
			.BorderImage(RoundedOutline(ColFrame, RFrame, ColEdge, 1.5f))
			.Padding(FrameT)
			.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&){ return FReply::Handled(); })
			[ Screen ]
		];
	Device = Handset;

	ChildSlot
	[
		SNew(SOverlay)
		// Dismiss scrim: a faint dim that also closes the phone when tapped outside it.
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([this](){ return FLinearColor(0.f,0.f,0.f, 0.28f * FMath::Clamp(Anim,0.f,1.f)); })
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&){ RequestDismiss(); return FReply::Handled(); })
		]
		// The phone, lower-right, scaled responsively to the viewport so it reads the same on
		// 720p and 4K (SDPIScaler scales layout AND fonts crisply — no fixed-pixel handset).
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 30.f, 22.f)
		[ SNew(SDPIScaler).DPIScale_Lambda([this]{ return UIScale; })[ Handset ] ]
	];

	SetCanTick(true);
}

// ---------------------------------------------------------------------------
// Brush pool
// ---------------------------------------------------------------------------

const FSlateBrush* SGTCPhone::Rounded(const FLinearColor& Fill, float Radius)
{
	TSharedRef<FSlateBrush> B = MakeShared<FSlateRoundedBoxBrush>(Fill, Radius);
	BrushPool.Add(B);
	return &B.Get();
}

const FSlateBrush* SGTCPhone::RoundedOutline(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineW)
{
	TSharedRef<FSlateBrush> B = MakeShared<FSlateRoundedBoxBrush>(Fill, Radius, Outline, OutlineW);
	BrushPool.Add(B);
	return &B.Get();
}

const FSlateBrush* SGTCPhone::Image(UTexture2D* Tex, FVector2f Size)
{
	TSharedRef<FSlateBrush> B = MakeShared<FSlateBrush>();
	B->SetResourceObject(Tex);
	B->ImageSize = FVector2D(Size.X, Size.Y);
	B->DrawAs = ESlateBrushDrawType::Image;
	B->TintColor = FLinearColor::White;
	BrushPool.Add(B);
	return &B.Get();
}

const FSlateBrush* SGTCPhone::IconBrush(EGTCApp App, float Size)
{
	UTexture2D* Tex = AppIconTex.IsValidIndex((int32)App) ? AppIconTex[(int32)App] : nullptr;
	return Image(Tex, FVector2f(Size, Size));
}

const FButtonStyle* SGTCPhone::RoundedButtonStyle(const FLinearColor& Fill, float Radius)
{
	// SButton caches raw pointers into this style's brushes, so the style (and its brushes)
	// must outlive the button — keep it in a pool, never on the stack.
	TSharedRef<FButtonStyle> S = MakeShared<FButtonStyle>(FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"));
	S->SetNormal(*Rounded(Fill, Radius));
	S->SetHovered(*Rounded(Lighten(Fill), Radius));
	S->SetPressed(*Rounded(Darken(Fill), Radius));
	S->SetNormalPadding(FMargin(0));
	S->SetPressedPadding(FMargin(0));
	StylePool.Add(S);
	return &S.Get();
}

// ---------------------------------------------------------------------------
// Animation + input
// ---------------------------------------------------------------------------

void SGTCPhone::AnimateOpen()
{
	bTargetOpen = true;
	bClosedNotified = false;
	GoHome();
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::SetDirectly);
}

void SGTCPhone::AnimateClose()
{
	bTargetOpen = false;
	bClosedNotified = false;
}

void SGTCPhone::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	const float Target = bTargetOpen ? 1.f : 0.f;
	Anim = FMath::FInterpTo(Anim, Target, InDeltaTime, 11.f);
	if (FMath::Abs(Anim - Target) < 0.004f) Anim = Target;

	// Responsive sizing: scale the whole handset (and thus every font) to the viewport height,
	// so the phone occupies a consistent fraction of the screen at any resolution. 760 is the
	// design height at which scale == 1.
	const float ViewportH = AllottedGeometry.GetLocalSize().Y;
	if (ViewportH > 1.f) UIScale = FMath::Clamp(ViewportH / 760.f, 0.55f, 2.6f);

	// Seamless app<->home transition: ease the screen content in with a subtle scale + fade.
	if (ContentAnim < 1.f)
	{
		ContentAnim = FMath::Min(1.f, ContentAnim + InDeltaTime * 6.5f);
		if (ContentHost.IsValid())
		{
			const float e = ContentAnim * ContentAnim * (3.f - 2.f * ContentAnim); // smoothstep
			ContentHost->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			ContentHost->SetRenderTransform(FSlateRenderTransform(FScale2D(FMath::Lerp(0.96f, 1.f, e), FMath::Lerp(0.96f, 1.f, e))));
			ContentHost->SetRenderOpacity(e);
		}
	}

	if (Device.IsValid())
	{
		const float e = FMath::Clamp(Anim, 0.f, 1.f);
		const float Scale = FMath::Lerp(0.9f, 1.f, e);
		const float TransY = FMath::Lerp(DeviceH * 0.55f, 0.f, e);
		const float TransX = FMath::Lerp(36.f, 0.f, e);
		Device->SetRenderTransformPivot(FVector2D(1.f, 1.f));
		Device->SetRenderTransform(FSlateRenderTransform(FScale2D(Scale, Scale), FVector2f(TransX, TransY)));
		Device->SetRenderOpacity(e);
	}

	if (!bTargetOpen && Anim <= 0.f && !bClosedNotified)
	{
		bClosedNotified = true;
		OnFullyClosed.ExecuteIfBound();
	}
}

FReply SGTCPhone::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::BackSpace || K == EKeys::Gamepad_FaceButton_Right)
	{
		if (!HandleBack()) RequestDismiss();
		return FReply::Handled();
	}
	if (K == EKeys::Up || K == EKeys::P || K == EKeys::Gamepad_DPad_Up)
	{
		RequestDismiss();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SGTCPhone::RequestDismiss()
{
	// Let the owner (player controller) drive the close so its open/closed flag and input
	// focus stay authoritative — otherwise self-closing here leaves bPhoneOpen stuck true and
	// the phone can never be reopened. Fall back to a local animation if nobody's listening.
	if (OnCloseRequested.IsBound()) OnCloseRequested.Execute();
	else AnimateClose();
}

bool SGTCPhone::HandleBack()
{
	if (bInApp) { GoHome(); return true; }
	return false;
}

void SGTCPhone::OpenApp(EGTCApp App)
{
	bInApp = true;
	CurrentApp = App;
	CallStatus.Reset();
	OpenThread = -1;
	ContentAnim = 0.f; // trigger the seamless scale+fade in
	if (Services.OnAppAction) Services.OnAppAction();
	RefreshContent();
}

void SGTCPhone::GoHome()
{
	bInApp = false;
	ContentAnim = 0.f;
	RefreshContent();
}

void SGTCPhone::NavigateToAppIndex(int32 Index)
{
	if (Index <= 0 || Index > (int32)EGTCApp::MAX) { GoHome(); }
	else { OpenApp((EGTCApp)(Index - 1)); }
}

void SGTCPhone::SettleTransition()
{
	ContentAnim = 1.f;
	if (ContentHost.IsValid())
	{
		ContentHost->SetRenderTransform(FSlateRenderTransform(FScale2D(1.f, 1.f)));
		ContentHost->SetRenderOpacity(1.f);
	}
}

void SGTCPhone::RefreshContent()
{
	if (!ContentHost.IsValid()) return;
	ContentHost->SetContent(bInApp ? BuildAppScreen(CurrentApp) : BuildHome());
}

// ---------------------------------------------------------------------------
// Shared widget helpers
// ---------------------------------------------------------------------------

TSharedRef<STextBlock> SGTCPhone::Label(const FText& Text, int32 Size, const FLinearColor& Color, const TCHAR* Style)
{
	return SNew(STextBlock).Font(Font(Size, Style)).ColorAndOpacity(Color).Text(Text);
}

TSharedRef<SWidget> SGTCPhone::AppIconButton(EGTCApp App, float IconSize, bool bShowLabel)
{
	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
	Content->AddSlot().AutoHeight().HAlign(HAlign_Center)
	[ SNew(SBox).WidthOverride(IconSize).HeightOverride(IconSize)[ SNew(SImage).Image(IconBrush(App, IconSize)) ] ];
	if (bShowLabel)
	{
		Content->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
		[ SNew(STextBlock).Font(Font(9)).ColorAndOpacity(Hx(0xFFFFFF, 0.92f)).Text(FText::FromString(AppDisplayName(App))) ];
	}

	return SNew(SButton)
		.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
		.ContentPadding(FMargin(0))
		.OnClicked_Lambda([this, App]() { OpenApp(App); return FReply::Handled(); })
		[ Content ];
}

TSharedRef<SWidget> SGTCPhone::PillButton(const FText& LabelText, const FLinearColor& Tint, TFunction<void()> OnClick, float Width)
{
	// Factor in alpha: a near-transparent fill (e.g. ColCard) must keep light text, or the
	// label renders dark on nothing and disappears (the old back-button "Home" bug).
	const bool bLight = (Tint.R + Tint.G + Tint.B) * Tint.A > 1.6f;
	TSharedRef<SWidget> Inner =
		SNew(STextBlock).Font(Font(12, TEXT("Bold")))
		.ColorAndOpacity(bLight ? Hx(0x111111) : ColText)
		.Justification(ETextJustify::Center)
		.Text(LabelText);

	TSharedRef<SButton> B = SNew(SButton)
		.ButtonStyle(RoundedButtonStyle(Tint, 10.f))
		.ContentPadding(FMargin(10.f, 8.f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		.OnClicked_Lambda([OnClick]() { if (OnClick) OnClick(); return FReply::Handled(); })
		[ Inner ];

	if (Width > 0.f)
	{
		return SNew(SBox).WidthOverride(Width)[ B ];
	}
	return B;
}

TSharedRef<SWidget> SGTCPhone::Card(TSharedRef<SWidget> Content, const FLinearColor& Fill)
{
	return SNew(SBorder).BorderImage(Rounded(Fill, 16.f)).Padding(FMargin(14.f, 12.f))[ Content ];
}

TSharedRef<SWidget> SGTCPhone::ListRow(const FText& Left, const FText& Right, const FLinearColor& RightTint)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColText).Text(Left) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[ SNew(STextBlock).Font(Font(13, TEXT("Bold"))).ColorAndOpacity(RightTint).Text(Right) ];
}

// ---------------------------------------------------------------------------
// Home screen
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SGTCPhone::BuildHome()
{
	auto IconRow = [this](const TArray<EGTCApp>& Apps, float Size, bool bLabels) -> TSharedRef<SWidget>
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (EGTCApp A : Apps)
		{
			Row->AddSlot().FillWidth(1.f).HAlign(HAlign_Center)[ AppIconButton(A, Size, bLabels) ];
		}
		return Row;
	};

	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
	const TArray<TArray<EGTCApp>> Rows = {
		{ EGTCApp::Maps, EGTCApp::Weather, EGTCApp::Clock, EGTCApp::Wallet },
		{ EGTCApp::Photos, EGTCApp::Settings, EGTCApp::Calculator, EGTCApp::Notes },
		{ EGTCApp::Browser, EGTCApp::Fitness, EGTCApp::Stocks, EGTCApp::Contacts },
	};
	for (const TArray<EGTCApp>& R : Rows)
	{
		Grid->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)[ IconRow(R, 52.f, true) ];
	}

	// Dock: a frosted bar holding the four primary apps.
	TSharedRef<SWidget> Dock =
		SNew(SBox).WidthOverride(ScreenW - 8.f).HeightOverride(80.f)
		[
			SNew(SBorder).BorderImage(Rounded(Hx(0xFFFFFF, 0.14f), 26.f)).Padding(FMargin(14.f, 10.f))
			[ IconRow({ EGTCApp::Phone, EGTCApp::Messages, EGTCApp::Camera, EGTCApp::Music }, 56.f, false) ]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()[ SNew(SImage).Image(Image(WallpaperTex, FVector2f(ScreenW, DeviceH))) ]
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ SNew(SSpacer).Size(FVector2D(0.f, TopInset + 14.f)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(SidePad, 0.f))[ Grid ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 18.f)[ Dock ]
		];
}

// ---------------------------------------------------------------------------
// App screen scaffold
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SGTCPhone::BuildAppHeader(const FText& Title, const FLinearColor& Tint)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[ PillButton(LOCTEXT("back", "‹ Home"), ColCard, [this]() { GoHome(); }) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[ SNew(STextBlock).Font(Font(18, TEXT("Black"))).ColorAndOpacity(Tint).Text(Title) ]
		+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(64.f) ];
}

TSharedRef<SWidget> SGTCPhone::BuildAppScreen(EGTCApp App)
{
	FLinearColor Tint = ColText;
	switch (App)
	{
	case EGTCApp::Maps: Tint = Hx(0x49D6B0); break;
	case EGTCApp::Weather: Tint = Hx(0x6FB7FF); break;
	case EGTCApp::Wallet: Tint = ColGreen; break;
	case EGTCApp::Stocks: Tint = Hx(0x4ED88A); break;
	case EGTCApp::Music: Tint = Hx(0xFF5E7E); break;
	case EGTCApp::Notes: Tint = Hx(0xFFD23B); break;
	default: break;
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()[ SNew(SImage).Image(Rounded(ColAppBg, RScreen)) ]
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ SNew(SSpacer).Size(FVector2D(0.f, TopInset)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(12.f, 2.f, 12.f, 6.f))
			[ BuildAppHeader(FText::FromString(AppDisplayName(App)), Tint) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(FMargin(14.f, 2.f, 14.f, 22.f))[ BuildAppBody(App) ]
			]
		];
}

TSharedRef<SWidget> SGTCPhone::BuildAppBody(EGTCApp App)
{
	switch (App)
	{
	case EGTCApp::Maps:       return BodyMaps();
	case EGTCApp::Weather:    return BodyWeather();
	case EGTCApp::Clock:      return BodyClock();
	case EGTCApp::Wallet:     return BodyWallet();
	case EGTCApp::Stocks:     return BodyStocks();
	case EGTCApp::Settings:   return BodySettings();
	case EGTCApp::Calculator: return BodyCalculator();
	case EGTCApp::Notes:      return BodyNotes();
	case EGTCApp::Browser:    return BodyBrowser();
	case EGTCApp::Music:      return BodyMusic();
	case EGTCApp::Phone:      return BodyPhone();
	case EGTCApp::Messages:   return BodyMessages();
	case EGTCApp::Camera:     return BodyCamera();
	case EGTCApp::Photos:     return BodyPhotos();
	case EGTCApp::Fitness:    return BodyFitness();
	case EGTCApp::Contacts:   return BodyContacts();
	default:                  return SNew(SBox);
	}
}

// ---------------------------------------------------------------------------
// Apps
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SGTCPhone::BodyMaps()
{
	auto LocText = [this]() { return FText::FromString(Services.GetLocationText ? Services.GetLocationText() : TEXT("Caliber Isles")); };

	// Stylised top-down city map (generated backdrop) with a centred player blip + waypoint.
	TSharedRef<SOverlay> MapStack = SNew(SOverlay);
	if (MapTex)
	{
		TSharedRef<FSlateBrush> B = MakeShared<FSlateBrush>();
		B->SetResourceObject(MapTex);
		B->ImageSize = FVector2D(280.f, 190.f);
		B->DrawAs = ESlateBrushDrawType::Image;
		B->TintColor = FLinearColor::White;
		BrushPool.Add(B);
		MapStack->AddSlot()[ SNew(SImage).Image(&B.Get()) ];
	}
	else
	{
		MapStack->AddSlot()[ SNew(SImage).Image(Rounded(Hx(0x16352B), 16.f)) ];
	}
	// Heading-oriented player blip (a glowing dot with a white ring), dead-centre.
	const float Heading = Services.GetHeadingDeg ? Services.GetHeadingDeg() : 0.f;
	MapStack->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[ SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)[ SNew(SImage).Image(RoundedOutline(ColAccent, 10.f, ColText, 2.5f)) ] ];
	// Waypoint pin (offset up-right) when set.
	if (bWaypointSet)
	{
		MapStack->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(70.f, 0.f, 0.f, 50.f)
			[ SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)[ SNew(SImage).Image(RoundedOutline(ColRed, 8.f, ColText, 2.f)) ] ];
	}
	MapStack->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(10.f)
		[
			SNew(SBorder).BorderImage(Rounded(Hx(0x000000, 0.45f), 8.f)).Padding(FMargin(8.f, 4.f))
			[ SNew(STextBlock).Font(Font(11, TEXT("Bold"))).ColorAndOpacity(ColText).Text_Lambda(LocText) ]
		];

	(void)Heading;
	TSharedRef<SWidget> MapCard = SNew(SBox).HeightOverride(190.f)[ MapStack ];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,10.f)[ MapCard ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,10.f)
		[ Card(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ ListRow(LOCTEXT("loc","Location"), FText::FromString(Services.GetLocationText?Services.GetLocationText():TEXT("—")), ColDim) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f,8.f,0.f,0.f)
			[ ListRow(LOCTEXT("wpt","Waypoint"), bWaypointSet?LOCTEXT("set","Active"):LOCTEXT("none","None"), bWaypointSet?ColGreen:ColDim) ])
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f,0.f,5.f,0.f)
			[ PillButton(bWaypointSet?LOCTEXT("clrwp","Clear Waypoint"):LOCTEXT("setwp","Set Waypoint"), bWaypointSet?ColCard:ColAccent,
				[this]{ bWaypointSet=!bWaypointSet; if(Services.OnAppAction)Services.OnAppAction(); RefreshContent(); }) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(5.f,0.f,0.f,0.f)
			[ PillButton(LOCTEXT("recenter","Recenter"), ColCard, [this]{ if(Services.OnAppAction)Services.OnAppAction(); }) ]
		];
}

TSharedRef<SWidget> SGTCPhone::BodyWeather()
{
	const float Hour = Services.GetTimeHour ? Services.GetTimeHour() : 12.f;
	auto Cond = [](float h)->FString {
		if (h >= 6.f && h < 11.f) return TEXT("Sunny");
		if (h >= 11.f && h < 17.f) return TEXT("Hot & Clear");
		if (h >= 17.f && h < 20.f) return TEXT("Golden Hour");
		if (h >= 20.f || h < 5.f) return TEXT("Clear Night");
		return TEXT("Hazy Dawn");
	};
	const int32 Temp = 27 + FMath::RoundToInt(5.f * FMath::Sin((Hour - 9.f) / 24.f * 2.f * PI));

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,6.f,0.f,2.f)
	[ SNew(STextBlock).Font(Font(15)).ColorAndOpacity(ColDim).Text(LOCTEXT("cbi","Caliber Isles")) ];
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center)
	[ SNew(STextBlock).Font(Font(64, TEXT("Light"))).ColorAndOpacity(ColText).Text(FText::FromString(FString::Printf(TEXT("%d°"), Temp))) ];
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,0.f,0.f,14.f)
	[ SNew(STextBlock).Font(Font(16, TEXT("Bold"))).ColorAndOpacity(Hx(0x6FB7FF)).Text(FText::FromString(Cond(Hour))) ];

	// Hourly strip — tapping an hour jumps the in-game clock there.
	TSharedRef<SHorizontalBox> Hourly = SNew(SHorizontalBox);
	for (int i = 0; i < 5; ++i)
	{
		const int hh = (FMath::FloorToInt(Hour) + i * 3) % 24;
		const int t = 27 + FMath::RoundToInt(5.f * FMath::Sin((hh - 9.f) / 24.f * 2.f * PI));
		Hourly->AddSlot().FillWidth(1.f).HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[ SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
				.OnClicked_Lambda([this,hh]{ if(Services.SetTimeHour)Services.SetTimeHour((float)hh); RefreshContent(); return FReply::Handled(); })
				[ SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ SNew(STextBlock).Font(Font(11)).ColorAndOpacity(ColDim).Text(FText::FromString(FString::Printf(TEXT("%02d"), hh))) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,6.f)[ SNew(SBox).WidthOverride(10.f).HeightOverride(10.f)[ SNew(SImage).Image(Rounded(Hx(0xFFD23F), 5.f)) ] ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ SNew(STextBlock).Font(Font(12, TEXT("Bold"))).ColorAndOpacity(ColText).Text(FText::FromString(FString::Printf(TEXT("%d°"), t))) ]
				]
			]
		];
	}
	Box->AddSlot().AutoHeight()[ Card(Hourly) ];
	Box->AddSlot().AutoHeight().Padding(0.f,12.f,0.f,0.f).HAlign(HAlign_Center)
	[ SNew(STextBlock).Font(Font(11)).ColorAndOpacity(ColDim).Text(LOCTEXT("tapfc","Tap an hour to change the time of day")) ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyClock()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,10.f,0.f,0.f)
	[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColDim).Text(LOCTEXT("ingame","IN-GAME TIME")) ];
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center)
	[ SNew(STextBlock).Font(Font(58, TEXT("Light"))).ColorAndOpacity(ColText).Text(this, &SGTCPhone::GetGameClock) ];
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,2.f,0.f,16.f)
	[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColDim).Text_Lambda([this]{ return FText::Format(LOCTEXT("wall","Local: {0}"), GetStatusClock()); }) ];

	auto TimeBtn = [this](const FText& L, float h){ return PillButton(L, ColCard, [this,h]{ if(Services.SetTimeHour)Services.SetTimeHour(h); RefreshContent(); }); };
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
	[ SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f,0.f,4.f,0.f)[ TimeBtn(LOCTEXT("dawn","Dawn"), 6.f) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f,0.f,4.f,0.f)[ TimeBtn(LOCTEXT("noon","Noon"), 12.f) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f,0.f,4.f,0.f)[ TimeBtn(LOCTEXT("dusk","Dusk"), 19.f) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f,0.f,0.f,0.f)[ TimeBtn(LOCTEXT("night","Night"), 0.f) ]
	];

	const TCHAR* Cities[] = { TEXT("Caliber Isles"), TEXT("Bayshore"), TEXT("Northpoint"), TEXT("London") };
	const int Off[] = { 0, -3, 0, 5 };
	TSharedRef<SVerticalBox> World = SNew(SVerticalBox);
	for (int i = 0; i < 4; ++i)
	{
		const int idx=i;
		World->AddSlot().AutoHeight().Padding(0.f,5.f)
		[ ListRow(FText::FromString(Cities[i]),
			TAttribute<FText>::CreateLambda([this,idx,Off]{ FDateTime n=FDateTime::Now(); n+=FTimespan(Off[idx],0,0); return FText::FromString(n.ToString(TEXT("%H:%M"))); }).Get(), ColDim) ];
	}
	Box->AddSlot().AutoHeight().Padding(0.f,8.f,0.f,0.f)[ Card(World) ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyWallet()
{
	const int64 Cash = Services.GetCash ? Services.GetCash() : 0;

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight()
	[ Card(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(12)).ColorAndOpacity(Hx(0x111111,0.7f)).Text(LOCTEXT("bal","BALANCE")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(38, TEXT("Black"))).ColorAndOpacity(Hx(0x0A0A0A))
			.Text_Lambda([this]{ const int64 c=Services.GetCash?Services.GetCash():0; return FText::FromString(FString::Printf(TEXT("$%lld"), c)); }) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f,4.f,0.f,0.f)[ SNew(STextBlock).Font(Font(12, TEXT("Bold"))).ColorAndOpacity(Hx(0x0A0A0A,0.6f)).Text(LOCTEXT("debit","CALIBER ISLES DEBIT  ••89")) ]
		, Hx(0x32D158)) ];

	auto Money = [this](const FText& L, int64 delta, const FLinearColor& c){
		return PillButton(L, c, [this,delta]{
			if (delta >= 0) { if (Services.AddCash) Services.AddCash(delta); Ledger.Insert(FString::Printf(TEXT("Deposit            +$%lld"), delta), 0); }
			else { const bool ok = Services.TrySpend ? Services.TrySpend(-delta) : false; Ledger.Insert(FString::Printf(TEXT("%s            -$%lld"), ok?TEXT("Withdrawal"):TEXT("DECLINED"), -delta), 0); }
			RefreshContent();
		});
	};
	Box->AddSlot().AutoHeight().Padding(0.f,10.f,0.f,0.f)
	[ SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f,0.f,4.f,0.f)[ Money(LOCTEXT("dep","Deposit $500"), 500, ColAccent) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f,0.f,0.f,0.f)[ Money(LOCTEXT("wd","Withdraw $200"), -200, ColCard) ]
	];

	TSharedRef<SVerticalBox> Tx = SNew(SVerticalBox);
	Tx->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,6.f)[ SNew(STextBlock).Font(Font(13, TEXT("Bold"))).ColorAndOpacity(ColText).Text(LOCTEXT("recent","Recent")) ];
	for (const FString& S : Ledger)
	{
		const bool plus = S.Contains(TEXT("+"));
		Tx->AddSlot().AutoHeight().Padding(0.f,4.f)
		[ SNew(STextBlock).Font(Font(12)).ColorAndOpacity(plus?ColGreen:Hx(0xFF8A80)).Text(FText::FromString(S)) ];
	}
	Box->AddSlot().AutoHeight().Padding(0.f,12.f,0.f,0.f)[ Card(Tx) ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyStocks()
{
	const float Hour = Services.GetTimeHour ? Services.GetTimeHour() : 12.f;
	auto Price = [Hour](int32 i)->float {
		const float base[] = { 142.5f, 88.2f, 19.7f, 305.0f };
		return base[i] * (1.f + 0.12f * FMath::Sin(Hour * 0.6f + i * 1.7f));
	};
	auto Chg = [Hour](int32 i)->float { return 12.f * FMath::Sin(Hour * 0.6f + i * 1.7f); };

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	for (int i = 0; i < Portfolio.Num(); ++i)
	{
		const int idx = i;
		const float p = Price(i);
		const float ch = Chg(i);
		Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
		[ Card(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[ SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(15, TEXT("Black"))).ColorAndOpacity(ColText).Text(FText::FromString(Portfolio[i].Ticker)) ]
				+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(11)).ColorAndOpacity(ColDim)
					.Text(FText::FromString(FString::Printf(TEXT("%d shares"), Portfolio[i].Shares))) ] ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f,0.f)
			[ SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[ SNew(STextBlock).Font(Font(14, TEXT("Bold"))).ColorAndOpacity(ColText).Text(FText::FromString(FString::Printf(TEXT("$%.2f"), p))) ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[ SNew(STextBlock).Font(Font(11, TEXT("Bold"))).ColorAndOpacity(ch>=0?ColGreen:ColRed).Text(FText::FromString(FString::Printf(TEXT("%+.1f%%"), ch))) ] ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[ SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f,0.f)[ PillButton(LOCTEXT("buy","Buy"), ColGreen, [this,idx,p]{ const int64 c=FMath::RoundToInt(p); if(Services.TrySpend && Services.TrySpend(c)) Portfolio[idx].Shares++; RefreshContent(); }) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f,0.f)[ PillButton(LOCTEXT("sell","Sell"), ColCard, [this,idx,p]{ if(Portfolio[idx].Shares>0){ Portfolio[idx].Shares--; if(Services.AddCash)Services.AddCash(FMath::RoundToInt(p)); } RefreshContent(); }) ] ]
		) ];
	}
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,4.f,0.f,0.f)
	[ SNew(STextBlock).Font(Font(11)).ColorAndOpacity(ColDim).Text(LOCTEXT("mktnote","Prices drift with the time of day — buy low at dawn")) ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodySettings()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
	[ Card(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ SNew(STextBlock).Font(Font(14)).ColorAndOpacity(ColText).Text(LOCTEXT("camview","Camera View")) ]
		+ SHorizontalBox::Slot().AutoWidth()
		[ PillButton((Services.IsFirstPerson && Services.IsFirstPerson())?LOCTEXT("fp","First-Person"):LOCTEXT("tp","Third-Person"), ColAccent,
			[this]{ if(Services.SetFirstPerson && Services.IsFirstPerson) Services.SetFirstPerson(!Services.IsFirstPerson()); RefreshContent(); }) ]
	) ];

	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
	[ Card(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
		[ SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ SNew(STextBlock).Font(Font(14)).ColorAndOpacity(ColText).Text(LOCTEXT("tod","Time of Day")) ]
			+ SHorizontalBox::Slot().AutoWidth()[ SNew(STextBlock).Font(Font(14, TEXT("Bold"))).ColorAndOpacity(ColDim).Text(this, &SGTCPhone::GetGameClock) ] ]
		+ SVerticalBox::Slot().AutoHeight()
		[ SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f,0.f,4.f,0.f)[ PillButton(LOCTEXT("dawn2","06:00"), ColCard, [this]{ if(Services.SetTimeHour)Services.SetTimeHour(6.f); RefreshContent(); }) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f,0.f,4.f,0.f)[ PillButton(LOCTEXT("noon2","12:00"), ColCard, [this]{ if(Services.SetTimeHour)Services.SetTimeHour(12.f); RefreshContent(); }) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f,0.f,0.f,0.f)[ PillButton(LOCTEXT("dusk2","19:00"), ColCard, [this]{ if(Services.SetTimeHour)Services.SetTimeHour(19.f); RefreshContent(); }) ] ]
	) ];

	const int32 Wanted = Services.GetWantedLevel ? Services.GetWantedLevel() : 0;
	TSharedRef<SHorizontalBox> Stars = SNew(SHorizontalBox);
	for (int i = 0; i < 5; ++i)
	{
		Stars->AddSlot().AutoWidth().Padding(2.f,0.f)
		[ SNew(SBox).WidthOverride(12.f).HeightOverride(12.f)[ SNew(SImage).Image(Rounded(i<Wanted?ColOrange:Hx(0xFFFFFF,0.18f), 6.f)) ] ];
	}
	Box->AddSlot().AutoHeight()
	[ Card(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ SNew(STextBlock).Font(Font(14)).ColorAndOpacity(ColText).Text(LOCTEXT("wl","Wanted Level")) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Stars ]
	) ];

	Box->AddSlot().AutoHeight().Padding(0.f,16.f,0.f,0.f).HAlign(HAlign_Center)
	[ SNew(STextBlock).Font(Font(10)).ColorAndOpacity(ColDim).Text(LOCTEXT("ver","Caliber OS 17.1  ·  GTC")) ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyCalculator()
{
	auto Compute = [this]() {
		const double cur = FCString::Atod(*CalcDisplay);
		double res = cur;
		if (CalcOp == TEXT("+")) res = CalcAccum + cur;
		else if (CalcOp == TEXT("-")) res = CalcAccum - cur;
		else if (CalcOp == TEXT("×")) res = CalcAccum * cur;
		else if (CalcOp == TEXT("÷")) res = (cur != 0.0) ? CalcAccum / cur : 0.0;
		CalcDisplay = FString::SanitizeFloat(res);
		if (CalcDisplay.EndsWith(TEXT(".0"))) CalcDisplay = CalcDisplay.LeftChop(2);
	};
	auto Digit = [this](const FString& d) {
		if (CalcFresh) { CalcDisplay = (d == TEXT(".")) ? TEXT("0.") : d; CalcFresh = false; }
		else if (!(d == TEXT(".") && CalcDisplay.Contains(TEXT(".")))) CalcDisplay += d;
	};
	auto Op = [this, Compute](const FString& o) {
		if (!CalcOp.IsEmpty() && !CalcFresh) Compute();
		CalcAccum = FCString::Atod(*CalcDisplay); CalcOp = o; CalcFresh = true;
	};

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight().Padding(0.f,4.f,0.f,12.f)
	[ SNew(SBox).HeightOverride(70.f)
		[ SNew(SBorder).BorderImage(Rounded(Hx(0x000000,0.0f),0.f)).HAlign(HAlign_Right).VAlign(VAlign_Bottom)
			[ SNew(STextBlock).Font(Font(48, TEXT("Light"))).ColorAndOpacity(ColText).Text_Lambda([this]{ return FText::FromString(CalcDisplay); }) ] ] ];

	struct FK { FString L; FLinearColor C; int type; }; // 0 digit, 1 op, 2 clear, 3 equals
	const TArray<TArray<FK>> Keys = {
		{ {TEXT("C"),Hx(0xA5A5A5),2}, {TEXT("±"),Hx(0xA5A5A5),2}, {TEXT("%"),Hx(0xA5A5A5),2}, {TEXT("÷"),ColOrange,1} },
		{ {TEXT("7"),Hx(0x333333),0}, {TEXT("8"),Hx(0x333333),0}, {TEXT("9"),Hx(0x333333),0}, {TEXT("×"),ColOrange,1} },
		{ {TEXT("4"),Hx(0x333333),0}, {TEXT("5"),Hx(0x333333),0}, {TEXT("6"),Hx(0x333333),0}, {TEXT("-"),ColOrange,1} },
		{ {TEXT("1"),Hx(0x333333),0}, {TEXT("2"),Hx(0x333333),0}, {TEXT("3"),Hx(0x333333),0}, {TEXT("+"),ColOrange,1} },
		{ {TEXT("0"),Hx(0x333333),0}, {TEXT("."),Hx(0x333333),0}, {TEXT("="),ColOrange,3} },
	};
	for (const TArray<FK>& Row : Keys)
	{
		TSharedRef<SHorizontalBox> H = SNew(SHorizontalBox);
		for (const FK& K : Row)
		{
			const FK Key = K;
			H->AddSlot().FillWidth(Key.L==TEXT("0")?2.f:1.f).Padding(3.f)
			[ SNew(SBox).HeightOverride(46.f)
				[ PillButton(FText::FromString(Key.L), Key.C, [this,Key,Digit,Op,Compute]{
					if (Key.type==0) Digit(Key.L);
					else if (Key.type==1) Op(Key.L);
					else if (Key.type==3) { if(!CalcOp.IsEmpty()){ Compute(); CalcOp.Reset(); CalcFresh=true; } }
					else { if(Key.L==TEXT("C")){ CalcDisplay=TEXT("0"); CalcAccum=0; CalcOp.Reset(); CalcFresh=true; }
						   else if(Key.L==TEXT("±")){ double v=-FCString::Atod(*CalcDisplay); CalcDisplay=FString::SanitizeFloat(v); if(CalcDisplay.EndsWith(TEXT(".0")))CalcDisplay=CalcDisplay.LeftChop(2);}
						   else if(Key.L==TEXT("%")){ double v=FCString::Atod(*CalcDisplay)/100.0; CalcDisplay=FString::SanitizeFloat(v);} }
					RefreshContent();
				}) ] ];
		}
		Box->AddSlot().AutoHeight()[ H ];
	}
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyNotes()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,10.f)
	[ SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f,0.f,6.f,0.f)
		[ SAssignNew(NoteInput, SEditableTextBox).HintText(LOCTEXT("newnote","New note…")).Font(Font(13)) ]
		+ SHorizontalBox::Slot().AutoWidth()
		[ PillButton(LOCTEXT("add","Add"), ColAccent, [this]{
			if(NoteInput.IsValid()){ const FString T=NoteInput->GetText().ToString().TrimStartAndEnd(); if(!T.IsEmpty()) Notes.Insert(T,0); }
			RefreshContent(); }) ]
	];
	for (const FString& N : Notes)
	{
		Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
		[ Card(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f,0.f,10.f,0.f)[ SNew(SBox).WidthOverride(8.f).HeightOverride(8.f)[ SNew(SImage).Image(Rounded(Hx(0xFFD23B),4.f)) ] ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColText).AutoWrapText(true).Text(FText::FromString(N)) ]
		) ];
	}
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyBrowser()
{
	struct FPage { FString Url; FString Title; TArray<FString> Lines; };
	static const TArray<FPage> Pages = {
		{ TEXT("findwave.io"), TEXT("Findwave"), { TEXT("The web, the way you like it."), TEXT("• Caliber Isles news"), TEXT("• Caliber Isles weather"), TEXT("• Maps & traffic") } },
		{ TEXT("bayline.news"), TEXT("Bayline News"), { TEXT("BREAKING: Marina heist suspects at large."), TEXT("City council debates new boardwalk."), TEXT("Heatwave grips the Isles this weekend.") } },
		{ TEXT("calibermotors.com"), TEXT("Caliber Motors"), { TEXT("The all-new Caliber GT."), TEXT("0-60 in 3.1s. Lease from $899/mo."), TEXT("Book a test drive today.") } },
		{ TEXT("squawk.com"), TEXT("Squawk"), { TEXT("@baylife: sunset at the pier 🌅"), TEXT("@calibrpd: drive safe out there."), TEXT("@lena_m: who's at the marina tonight?") } },
	};
	BrowserPage = FMath::Clamp(BrowserPage, 0, Pages.Num()-1);
	const FPage& P = Pages[BrowserPage];

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,10.f)
	[ SNew(SBorder).BorderImage(Rounded(Hx(0xFFFFFF,0.10f), 10.f)).Padding(FMargin(12.f,8.f))
		[ SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f,0.f,8.f,0.f)[ SNew(SBox).WidthOverride(10.f).HeightOverride(10.f)[ SNew(SImage).Image(Rounded(ColGreen,5.f)) ] ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColDim).Text(FText::FromString(P.Url)) ] ] ];

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)[ SNew(STextBlock).Font(Font(22, TEXT("Black"))).ColorAndOpacity(ColText).Text(FText::FromString(P.Title)) ];
	for (const FString& L : P.Lines)
		Body->AddSlot().AutoHeight().Padding(0.f,3.f)[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(Hx(0xFFFFFF,0.82f)).AutoWrapText(true).Text(FText::FromString(L)) ];
	Box->AddSlot().AutoHeight()[ Card(Body) ];

	TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
	for (int i = 0; i < Pages.Num(); ++i)
	{
		const int idx=i;
		Tabs->AddSlot().FillWidth(1.f).Padding(3.f)
		[ PillButton(FText::FromString(Pages[i].Title), idx==BrowserPage?ColAccent:ColCard, [this,idx]{ BrowserPage=idx; RefreshContent(); }) ];
	}
	Box->AddSlot().AutoHeight().Padding(0.f,12.f,0.f,0.f)[ Tabs ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyMusic()
{
	const TCHAR* Stations[] = { TEXT("Sunfire FM"), TEXT("Caliber 99"), TEXT("Riptide Radio"), TEXT("Boardwalk FM"), TEXT("Neon Pop") };
	const TCHAR* NowPlaying[] = { TEXT("Neon Tides — The Calibers"), TEXT("Sunset Drive — Marisol"), TEXT("Static Bloom — VHS Kid"), TEXT("Palm Fever — Lena M."), TEXT("Hold The Line — Dusklight") };
	MusicStation = FMath::Clamp(MusicStation, 0, 4);

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	// Now-playing card with an animated equaliser.
	TSharedRef<SHorizontalBox> EQ = SNew(SHorizontalBox);
	for (int i = 0; i < 7; ++i)
	{
		const int bi = i;
		EQ->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(2.f,0.f)
		[ SNew(SBox).WidthOverride(7.f)
			.HeightOverride_Lambda([this,bi]{ const float t=(float)FPlatformTime::Seconds(); const float h = bMusicPlaying ? (14.f + 18.f*FMath::Abs(FMath::Sin(t*4.f + bi*0.7f))) : 6.f; return FOptionalSize(h); })
			[ SNew(SImage).Image(Rounded(Hx(0xFF5E7E), 3.f)) ] ];
	}

	Box->AddSlot().AutoHeight()
	[ Card(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,4.f,0.f,8.f)[ SNew(SBox).HeightOverride(44.f)[ EQ ] ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ SNew(STextBlock).Font(Font(15, TEXT("Black"))).ColorAndOpacity(ColText).Text_Lambda([this,NowPlaying]{ return FText::FromString(NowPlaying[MusicStation]); }) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ SNew(STextBlock).Font(Font(12)).ColorAndOpacity(Hx(0xFF5E7E)).Text_Lambda([this,Stations]{ return FText::FromString(Stations[MusicStation]); }) ]
		, Hx(0xFFFFFF,0.06f)) ];

	Box->AddSlot().AutoHeight().Padding(0.f,12.f,0.f,12.f)
	[ SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f,0.f,5.f,0.f)[ PillButton(LOCTEXT("prev","‹‹ Prev"), ColCard, [this]{ MusicStation=(MusicStation+4)%5; if(Services.OnAppAction)Services.OnAppAction(); RefreshContent(); }) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(5.f,0.f,5.f,0.f)[ PillButton(bMusicPlaying?LOCTEXT("pause","❚❚ Pause"):LOCTEXT("play","▶ Play"), ColAccent, [this]{ bMusicPlaying=!bMusicPlaying; RefreshContent(); }) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(5.f,0.f,0.f,0.f)[ PillButton(LOCTEXT("next","Next ››"), ColCard, [this]{ MusicStation=(MusicStation+1)%5; if(Services.OnAppAction)Services.OnAppAction(); RefreshContent(); }) ]
	];

	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int i = 0; i < 5; ++i)
	{
		const int idx=i;
		List->AddSlot().AutoHeight().Padding(0.f,3.f)
		[ SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder")).OnClicked_Lambda([this,idx]{ MusicStation=idx; bMusicPlaying=true; RefreshContent(); return FReply::Handled(); })
			[ ListRow(FText::FromString(Stations[i]), idx==MusicStation?LOCTEXT("onair","● On Air"):FText::GetEmpty(), ColGreen) ] ];
	}
	Box->AddSlot().AutoHeight()[ Card(List) ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyPhone()
{
	return BodyContacts();
}

TSharedRef<SWidget> SGTCPhone::BodyContacts()
{
	const TCHAR* Names[] = { TEXT("Lena Moreno"), TEXT("Pay'n'Spray"), TEXT("Caliber Cabs"), TEXT("Mechanic — Sal"), TEXT("Emergency 911") };
	const TCHAR* Sub[] = { TEXT("mobile"), TEXT("repairs"), TEXT("dispatch"), TEXT("garage"), TEXT("services") };

	if (!CallStatus.IsEmpty() && ActiveContact >= 0)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,30.f,0.f,0.f)
			[ SNew(SBox).WidthOverride(96.f).HeightOverride(96.f)[ SNew(SImage).Image(Rounded(Hx(0x6C7480),48.f)) ] ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,16.f,0.f,2.f)
			[ SNew(STextBlock).Font(Font(24, TEXT("Black"))).ColorAndOpacity(ColText).Text(FText::FromString(Names[ActiveContact])) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[ SNew(STextBlock).Font(Font(14)).ColorAndOpacity(ColGreen).Text(FText::FromString(CallStatus)) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,40.f,0.f,0.f)
			[ PillButton(LOCTEXT("end","End Call"), ColRed, [this]{ CallStatus.Reset(); ActiveContact=-1; RefreshContent(); }, 160.f) ];
	}

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	for (int i = 0; i < 5; ++i)
	{
		const int idx = i;
		Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
		[ Card(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f,0.f,12.f,0.f)
			[ SNew(SBox).WidthOverride(38.f).HeightOverride(38.f)[ SNew(SImage).Image(Rounded(Hx(0x4A515C), 19.f)) ] ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[ SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(14, TEXT("Bold"))).ColorAndOpacity(ColText).Text(FText::FromString(Names[i])) ]
				+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(11)).ColorAndOpacity(ColDim).Text(FText::FromString(Sub[i])) ] ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[ PillButton(LOCTEXT("call","Call"), ColGreen, [this,idx]{ ActiveContact=idx; CallStatus=TEXT("calling…"); if(Services.OnAppAction)Services.OnAppAction(); RefreshContent(); }) ]
		) ];
	}
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyMessages()
{
	const TCHAR* Threads[] = { TEXT("Lena"), TEXT("Sal (Mechanic)"), TEXT("Unknown"), TEXT("Caliber Cabs") };
	const TCHAR* Preview[] = { TEXT("see you at the marina 🌴"), TEXT("car's ready for pickup"), TEXT("you didn't see anything."), TEXT("your ride is 2 min away") };

	if (OpenThread >= 0)
	{
		TSharedRef<SVerticalBox> Thread = SNew(SVerticalBox);
		auto Bubble = [this](const FString& Txt, bool bMine){
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(bMine?HAlign_Right:HAlign_Left)
				[ SNew(SBox).MaxDesiredWidth(190.f)
					[ SNew(SBorder).BorderImage(Rounded(bMine?ColAccent:Hx(0x2A2C31), 14.f)).Padding(FMargin(11.f,7.f))
						[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColText).AutoWrapText(true).Text(FText::FromString(Txt)) ] ] ];
		};
		Thread->AddSlot().AutoHeight().Padding(0.f,3.f)[ Bubble(FString(Preview[OpenThread]), false) ];
		for (const FString& L : MessageLog)
			Thread->AddSlot().AutoHeight().Padding(0.f,3.f)[ Bubble(L, true) ];

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
			[ SNew(STextBlock).Font(Font(16, TEXT("Black"))).ColorAndOpacity(ColText).Text(FText::FromString(Threads[OpenThread])) ]
			+ SVerticalBox::Slot().AutoHeight()[ Thread ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f,12.f,0.f,0.f)
			[ SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f,0.f,6.f,0.f)[ SAssignNew(MsgInput, SEditableTextBox).HintText(LOCTEXT("imsg","iMessage…")).Font(Font(13)) ]
				+ SHorizontalBox::Slot().AutoWidth()[ PillButton(LOCTEXT("send","Send"), ColAccent, [this]{ if(MsgInput.IsValid()){ const FString T=MsgInput->GetText().ToString().TrimStartAndEnd(); if(!T.IsEmpty()) MessageLog.Add(T);} RefreshContent(); }) ] ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f,10.f,0.f,0.f)[ PillButton(LOCTEXT("backthreads","‹ All Messages"), ColCard, [this]{ OpenThread=-1; MessageLog.Reset(); RefreshContent(); }) ];
	}

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	for (int i = 0; i < 4; ++i)
	{
		const int idx=i;
		Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,8.f)
		[ SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder")).OnClicked_Lambda([this,idx]{ OpenThread=idx; MessageLog.Reset(); RefreshContent(); return FReply::Handled(); })
			[ Card(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(14, TEXT("Bold"))).ColorAndOpacity(ColText).Text(FText::FromString(Threads[i])) ]
				+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Font(Font(12)).ColorAndOpacity(ColDim).Text(FText::FromString(Preview[i])) ]) ] ];
	}
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyCamera()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,14.f)
		[ SNew(SBox).HeightOverride(300.f)
			[ SNew(SOverlay)
				+ SOverlay::Slot()[ SNew(SImage).Image(Image(WallpaperTex, FVector2f(ScreenW, 300.f))) ]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.f)[ SNew(STextBlock).Font(Font(11, TEXT("Bold"))).ColorAndOpacity(Hx(0xFFFFFF,0.85f)).Text(LOCTEXT("rec","◉ GTC CAM")) ]
				// rule-of-thirds guides
				+ SOverlay::Slot().HAlign(HAlign_Center)[ SNew(SBox).WidthOverride(1.f)[ SNew(SImage).Image(Rounded(Hx(0xFFFFFF,0.25f),0.f)) ] ]
				+ SOverlay::Slot().VAlign(VAlign_Center)[ SNew(SBox).HeightOverride(1.f)[ SNew(SImage).Image(Rounded(Hx(0xFFFFFF,0.25f),0.f)) ] ]
			] ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[ SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.OnClicked_Lambda([this]{ if(Services.TakePhoto)Services.TakePhoto(); return FReply::Handled(); })
			[ SNew(SBox).WidthOverride(66.f).HeightOverride(66.f)
				[ SNew(SOverlay)
					+ SOverlay::Slot()[ SNew(SImage).Image(RoundedOutline(Hx(0xFFFFFF,0.0f),33.f,ColText,3.f)) ]
					+ SOverlay::Slot().Padding(5.f)[ SNew(SImage).Image(Rounded(ColText,28.f)) ] ] ] ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,12.f,0.f,0.f)
		[ SNew(STextBlock).Font(Font(11)).ColorAndOpacity(ColDim).Text(LOCTEXT("shutter","Tap the shutter — photos save to the Photos app")) ];
}

TSharedRef<SWidget> SGTCPhone::BodyPhotos()
{
	PhotoTextures.Reset();
	TArray<FString> Files = Services.ListPhotos ? Services.ListPhotos() : TArray<FString>();

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,10.f)
	[ SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColDim).Text_Lambda([this,Files]{ return FText::FromString(FString::Printf(TEXT("%d photos"), Files.Num())); }) ]
		+ SHorizontalBox::Slot().AutoWidth()[ PillButton(LOCTEXT("refresh","Refresh"), ColCard, [this]{ RefreshContent(); }) ] ];

	if (Files.Num() == 0)
	{
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,40.f,0.f,0.f)
		[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColDim).Text(LOCTEXT("noph","No photos yet — open Camera and tap the shutter")) ];
		return Box;
	}

	const int32 Cols = 3;
	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
	TSharedPtr<SHorizontalBox> Row;
	int32 shown = FMath::Min(Files.Num(), 9);
	for (int32 i = 0; i < shown; ++i)
	{
		if (i % Cols == 0) { Row = SNew(SHorizontalBox); Grid->AddSlot().AutoHeight().Padding(0.f,0.f,0.f,5.f)[ Row.ToSharedRef() ]; }
		UTexture2D* T = FImageUtils::ImportFileAsTexture2D(Files[i]);
		if (T) PhotoTextures.Add(TStrongObjectPtr<UTexture2D>(T));
		Row->AddSlot().FillWidth(1.f).Padding(2.5f)
		[ SNew(SBox).HeightOverride(78.f)
			[ SNew(SBorder).BorderImage(Rounded(Hx(0x222222),8.f)).Padding(0.f)
				[ T ? SNew(SImage).Image(Image(T, FVector2f(78.f,78.f))) : SNew(SImage).Image(Rounded(Hx(0x333333),8.f)) ] ] ];
	}
	Box->AddSlot().AutoHeight()[ Grid ];
	return Box;
}

TSharedRef<SWidget> SGTCPhone::BodyFitness()
{
	auto Bar = [this](const FText& L, const FLinearColor& C, TFunction<float()> Val, TFunction<FString()> Disp){
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[ SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(STextBlock).Font(Font(13, TEXT("Bold"))).ColorAndOpacity(C).Text(L) ]
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ColText).Text_Lambda([Disp]{ return FText::FromString(Disp()); }) ] ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f,5.f,0.f,0.f)
			[ SNew(SBox).HeightOverride(8.f)[ SNew(SProgressBar).Percent_Lambda([Val]{ return FMath::Clamp(Val(),0.f,1.f); }).FillColorAndOpacity(C) ] ];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
		[ Card(Bar(LOCTEXT("energy","Energy"), ColGreen,
			[this]{ return Services.GetStaminaPercent?Services.GetStaminaPercent():1.f; },
			[this]{ const float p=Services.GetStaminaPercent?Services.GetStaminaPercent():1.f; return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(p*100.f)); })) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,12.f)
		[ Card(Bar(LOCTEXT("speed","Speed"), Hx(0x14D9E6),
			[this]{ const float s=Services.GetSpeedKmh?Services.GetSpeedKmh():0.f; return s/40.f; },
			[this]{ const float s=Services.GetSpeedKmh?Services.GetSpeedKmh():0.f; return FString::Printf(TEXT("%.0f km/h"), s); })) ]
		+ SVerticalBox::Slot().AutoHeight()
		[ Card(Bar(LOCTEXT("heat","Heat"), ColRed,
			[this]{ const int w=Services.GetWantedLevel?Services.GetWantedLevel():0; return w/5.f; },
			[this]{ const int w=Services.GetWantedLevel?Services.GetWantedLevel():0; return FString::Printf(TEXT("%d / 5"), w); })) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f,16.f,0.f,0.f)
		[ SNew(STextBlock).Font(Font(11)).ColorAndOpacity(ColDim).Text(LOCTEXT("live","Live — updates as you move")) ];
}

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------

FText SGTCPhone::GetStatusClock() const
{
	return FText::FromString(FDateTime::Now().ToString(TEXT("%H:%M")));
}

FText SGTCPhone::GetGameClock() const
{
	const float h = Services.GetTimeHour ? Services.GetTimeHour() : 12.f;
	const int hh = FMath::Clamp(FMath::FloorToInt(h), 0, 23);
	const int mm = FMath::Clamp(FMath::FloorToInt((h - hh) * 60.f), 0, 59);
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), hh, mm));
}

FString SGTCPhone::AppDisplayName(EGTCApp App)
{
	switch (App)
	{
	case EGTCApp::Phone: return TEXT("Phone");
	case EGTCApp::Messages: return TEXT("Messages");
	case EGTCApp::Camera: return TEXT("Camera");
	case EGTCApp::Music: return TEXT("Music");
	case EGTCApp::Maps: return TEXT("Maps");
	case EGTCApp::Weather: return TEXT("Weather");
	case EGTCApp::Clock: return TEXT("Clock");
	case EGTCApp::Wallet: return TEXT("Wallet");
	case EGTCApp::Photos: return TEXT("Photos");
	case EGTCApp::Settings: return TEXT("Settings");
	case EGTCApp::Calculator: return TEXT("Calculator");
	case EGTCApp::Notes: return TEXT("Notes");
	case EGTCApp::Browser: return TEXT("Browser");
	case EGTCApp::Fitness: return TEXT("Fitness");
	case EGTCApp::Stocks: return TEXT("Stocks");
	case EGTCApp::Contacts: return TEXT("Contacts");
	default: return TEXT("App");
	}
}

#undef LOCTEXT_NAMESPACE
