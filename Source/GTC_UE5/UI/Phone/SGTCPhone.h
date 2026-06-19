#pragma once

#include "CoreMinimal.h"
#include "GTCPhoneIcons.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"

class SBox;
class STextBlock;
class SEditableTextBox;
class UTexture2D;
struct FButtonStyle;

/**
 * The bridge between the phone UI (pure Slate) and the game. The owning player controller
 * fills these in so each app can take real actions — move the in-game clock, spend cash,
 * toggle the camera, fire a screenshot, etc. — without the widget ever including a gameplay
 * header. Any callback left unset is simply treated as a no-op.
 */
struct FGTCPhoneServices
{
	TFunction<int64()>            GetCash;
	TFunction<void(int64)>        AddCash;        // signed delta
	TFunction<bool(int64)>        TrySpend;       // returns whether it was affordable
	TFunction<int32()>            GetWantedLevel;
	TFunction<float()>            GetStaminaPercent;
	TFunction<float()>            GetTimeHour;     // 0..24 in-game
	TFunction<void(float)>        SetTimeHour;
	TFunction<bool()>             IsFirstPerson;
	TFunction<void(bool)>         SetFirstPerson;
	TFunction<FString()>          GetLocationText;
	TFunction<float()>            GetSpeedKmh;
	TFunction<void()>             TakePhoto;       // world screenshot to Saved/Screenshots
	TFunction<TArray<FString>()>  ListPhotos;      // newest-first file paths
	TFunction<void()>             OnAppAction;     // optional: ping the world (haptic/sfx hook)
	TFunction<class UTexture*()>  GetMapTexture;   // live top-down world capture for the Maps app (null = use stylised fallback)
	TFunction<float()>            GetHeadingDeg;   // pawn yaw in degrees, for orienting the map marker
};

/**
 * GTC's in-world smartphone: a self-contained iPhone-class handset rendered entirely in
 * Slate. It draws its own device chrome (titanium frame, Dynamic Island, status bar, home
 * indicator), a home screen of original procedurally-generated app icons, a dock, and a
 * working screen for every app. It slides up from the lower-right on open and back down on
 * close. All artwork is generated at runtime (see GTCPhoneIcons) so nothing ships as a
 * binary asset.
 */
class GTC_UE5_API SGTCPhone : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGTCPhone) {}
		SLATE_ARGUMENT(FGTCPhoneServices, Services)
		/** Fired once the close animation finishes, so the owner can pull it from the viewport. */
		SLATE_EVENT(FSimpleDelegate, OnFullyClosed)
		/** Fired when the user dismisses the phone from inside the widget (back/toggle key), so the
		 *  owner stays the single source of truth for open/closed state and input focus. */
		SLATE_EVENT(FSimpleDelegate, OnCloseRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Begin the slide-up / slide-down. Close fires OnFullyClosed when it settles. */
	void AnimateOpen();
	void AnimateClose();
	bool IsOpening() const { return bTargetOpen; }

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	/** Open straight to an app by index (used by the headless console command). -1 / 0 = home. */
	void NavigateToAppIndex(int32 Index);

	/** Snap the app<->home transition to fully-settled (used by the headless tour so captures
	 *  don't catch a screen mid-fade). No-op for normal interactive use. */
	void SettleTransition();

private:
	// ---- brush pool: keeps generated/rounded brushes alive with stable addresses --------
	const FSlateBrush* Rounded(const FLinearColor& Fill, float Radius);
	const FSlateBrush* RoundedOutline(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineW);
	const FSlateBrush* Image(UTexture2D* Tex, FVector2f Size);
	const FSlateBrush* IconBrush(EGTCApp App, float Size);
	/** A persistent rounded-pill button style (SButton keeps the pointer, so it must outlive the widget). */
	const FButtonStyle* RoundedButtonStyle(const FLinearColor& Fill, float Radius);

	// ---- screen building ---------------------------------------------------------------
	void RefreshContent();
	TSharedRef<SWidget> BuildStatusBar(bool bDark);
	TSharedRef<SWidget> BuildHome();
	TSharedRef<SWidget> BuildDock();
	TSharedRef<SWidget> BuildAppScreen(EGTCApp App);
	TSharedRef<SWidget> BuildAppHeader(const FText& Title, const FLinearColor& Tint);
	TSharedRef<SWidget> BuildAppBody(EGTCApp App);

	// Per-app bodies.
	TSharedRef<SWidget> BodyMaps();
	TSharedRef<SWidget> BodyWeather();
	TSharedRef<SWidget> BodyClock();
	TSharedRef<SWidget> BodyWallet();
	TSharedRef<SWidget> BodyStocks();
	TSharedRef<SWidget> BodySettings();
	TSharedRef<SWidget> BodyCalculator();
	TSharedRef<SWidget> BodyNotes();
	TSharedRef<SWidget> BodyBrowser();
	TSharedRef<SWidget> BodyMusic();
	TSharedRef<SWidget> BodyPhone();
	TSharedRef<SWidget> BodyMessages();
	TSharedRef<SWidget> BodyCamera();
	TSharedRef<SWidget> BodyPhotos();
	TSharedRef<SWidget> BodyFitness();
	TSharedRef<SWidget> BodyContacts();

	// ---- shared widget helpers ---------------------------------------------------------
	TSharedRef<SWidget> AppIconButton(EGTCApp App, float IconSize, bool bShowLabel);
	TSharedRef<SWidget> PillButton(const FText& Label, const FLinearColor& Tint, TFunction<void()> OnClick, float Width = 0.f);
	TSharedRef<SWidget> ListRow(const FText& Left, const FText& Right, const FLinearColor& RightTint);
	TSharedRef<SWidget> Card(TSharedRef<SWidget> Content, const FLinearColor& Fill = FLinearColor(1,1,1,0.06f));
	TSharedRef<STextBlock> Label(const FText& Text, int32 Size, const FLinearColor& Color, const TCHAR* Style = TEXT("Regular"));

	void OpenApp(EGTCApp App);
	void GoHome();
	bool HandleBack(); // returns true if it consumed (was in an app)

	// Status / clock text.
	FText GetStatusClock() const;
	FText GetGameClock() const;
	static FString AppDisplayName(EGTCApp App);

	// ---- state -------------------------------------------------------------------------
	FGTCPhoneServices Services;
	FSimpleDelegate OnFullyClosed;
	FSimpleDelegate OnCloseRequested;
	/** Dismiss request: route through the owner if it's listening, else fall back to a local close. */
	void RequestDismiss();

	TArray<TStrongObjectPtr<UTexture2D>> OwnedTextures; // icons + wallpaper, GC-safe
	TArray<UTexture2D*> AppIconTex;                      // by (int)EGTCApp
	UTexture2D* WallpaperTex = nullptr;
	UTexture2D* MapTex = nullptr;        // stylised Maps-app backdrop

	TArray<TSharedPtr<FSlateBrush>> BrushPool;
	TArray<TSharedPtr<FButtonStyle>> StylePool;
	TArray<TStrongObjectPtr<UTexture2D>> PhotoTextures;  // lazily loaded screenshots

	TSharedPtr<SBox> ContentHost;
	TSharedPtr<SWidget> Device;            // the handset; animated independently of the scrim
	TSharedPtr<SEditableTextBox> NoteInput;
	TSharedPtr<SEditableTextBox> MsgInput;

	bool bInApp = false;
	EGTCApp CurrentApp = EGTCApp::Maps;

	// Responsive layout scale (viewport-height driven); applied via an SDPIScaler around the handset.
	float UIScale = 1.f;

	// App<->home transition: 0 just after a navigation, eases to 1 (scale+fade on the screen content).
	float ContentAnim = 1.f;

	// Animation: 0 = fully tucked away (off-screen, scaled down), 1 = fully out.
	float Anim = 0.f;
	bool bTargetOpen = false;
	bool bClosedNotified = false;

	// App working state.
	FString CalcDisplay = TEXT("0");
	double CalcAccum = 0.0;
	FString CalcOp;
	bool CalcFresh = true;

	int32 BrowserPage = 0;
	int32 MusicStation = 0;
	bool bMusicPlaying = true;
	bool bWaypointSet = false;

	TArray<FString> Notes;
	TArray<FString> MessageLog;     // simple "You: ..." appended lines for the open thread
	int32 OpenThread = -1;
	TArray<FString> Ledger;         // wallet transaction lines

	struct FHolding { FString Ticker; int32 Shares = 0; };
	TArray<FHolding> Portfolio;

	FString CallStatus;             // non-empty while a call screen is up
	int32 ActiveContact = -1;
};
