#include "GTCFrontEndController.h"
#include "SGTCFrontEnd.h"
#include "../Core/GTCGameInstance.h"

#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "MediaSoundComponent.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "Widgets/SWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGTCFrontEnd, Log, All);

static TAutoConsoleVariable<float> CVarFrontEndAutoStart(
	TEXT("gtc.FrontEndAutoStart"), 0.0f,
	TEXT("Dev: auto-press Start this many seconds after the menu appears (0 = off)."));

AGTCFrontEndController::AGTCFrontEndController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
}

void AGTCFrontEndController::BeginPlay()
{
	Super::BeginPlay();

	// Runtime media objects — no editor-authored MediaPlayer/MediaTexture assets.
	MediaPlayer = NewObject<UMediaPlayer>(this, TEXT("FrontEndMediaPlayer"));
	MediaPlayer->PlayOnOpen = true;
	MediaPlayer->OnEndReached.AddDynamic(this, &AGTCFrontEndController::HandleVideoEnded);

	MediaTexture = NewObject<UMediaTexture>(this, TEXT("FrontEndMediaTexture"));
	MediaTexture->AutoClear = true;
	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();

	MediaSound = NewObject<UMediaSoundComponent>(this, TEXT("FrontEndMediaSound"));
	MediaSound->SetMediaPlayer(MediaPlayer);
	MediaSound->RegisterComponentWithWorld(GetWorld());

	FrontEndWidget = SNew(SGTCFrontEnd)
		.VideoTexture(MediaTexture)
		.OnSkip(FSimpleDelegate::CreateUObject(this, &AGTCFrontEndController::RequestSkipIntro))
		.OnStart(FSimpleDelegate::CreateUObject(this, &AGTCFrontEndController::RequestStartGame))
		.OnSettings(FSimpleDelegate::CreateUObject(this, &AGTCFrontEndController::RequestSettings))
		.OnExit(FSimpleDelegate::CreateUObject(this, &AGTCFrontEndController::RequestExit));

	if (UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		ViewportClient->AddViewportWidgetContent(FrontEndWidget.ToSharedRef(), 1000);
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(FrontEndWidget);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// Begin streaming the world in the background while the intro plays.
	BeginPreloadWorld();

	// Start on black and fade up while the first intro video plays.
	FadeAlpha = 1.0f;
	FadeDir = -1.0f;
	IntroIndex = 0;
	if (IntroMovies.Num() > 0)
	{
		EnterPhase(EGTCFrontEndPhase::Intro);
		PlayMovie(IntroMovies[0], /*bLoop*/ false);
	}
	else
	{
		// No intro configured — go straight to the menu.
		BeginHandoffToMenu();
	}
}

void AGTCFrontEndController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FrontEndWidget.IsValid())
	{
		if (UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			ViewportClient->RemoveViewportWidgetContent(FrontEndWidget.ToSharedRef());
		}
		FrontEndWidget.Reset();
	}
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	Super::EndPlay(EndPlayReason);
}

void AGTCFrontEndController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FrontEndWidget.IsValid())
	{
		return;
	}

	// Poll background world-load progress and reflect it on the intro overlay.
	if (!bWorldPreloaded)
	{
		const float Pct = GetAsyncLoadPercentage(FName(*WorldMapPath));
		if (Pct >= 0.0f)
		{
			PreloadProgress = FMath::Clamp(Pct / 100.0f, 0.0f, 1.0f);
		}
	}
	FrontEndWidget->SetIntroProgress(PreloadProgress);
	FrontEndWidget->SetSkipReady(bWorldPreloaded);

	// Advance the fade.
	if (FadeDir != 0.0f && FadeTime > KINDA_SMALL_NUMBER)
	{
		FadeAlpha = FMath::Clamp(FadeAlpha + FadeDir * (DeltaSeconds / FadeTime), 0.0f, 1.0f);

		if (FadeDir > 0.0f && FadeAlpha >= 1.0f)
		{
			// Reached full black — run whatever was waiting on the fade.
			FadeDir = 0.0f;
			const EPendingAction Action = PendingAction;
			PendingAction = EPendingAction::None;
			if (Action == EPendingAction::ShowMenu)
			{
				PlayMovie(MenuMovie, /*bLoop*/ true);
				EnterPhase(EGTCFrontEndPhase::Menu);
				FrontEndWidget->SetVisual(EGTCFrontEndVisual::Menu);
				FadeDir = -1.0f; // fade back up into the menu
			}
			else if (Action == EPendingAction::OpenWorld)
			{
				OpenWorld();
				return;
			}
		}
		else if (FadeDir < 0.0f && FadeAlpha <= 0.0f)
		{
			FadeDir = 0.0f;
		}
	}

	// Dev capture hook: auto-Start after dwelling on the menu.
	if (Phase == EGTCFrontEndPhase::Menu && !bStarting)
	{
		const float AutoStart = CVarFrontEndAutoStart.GetValueOnGameThread();
		if (AutoStart > 0.0f && bWorldPreloaded)
		{
			MenuDwell += DeltaSeconds;
			if (MenuDwell >= AutoStart)
			{
				RequestStartGame();
			}
		}
	}

	// Cosmetic loading progress while starting the world.
	if (bStarting)
	{
		LoadProgress = FMath::Clamp(LoadProgress + DeltaSeconds / FMath::Max(FadeTime, 0.1f), 0.0f, 1.0f);
		FrontEndWidget->SetLoadingProgress(LoadProgress);
	}

	FrontEndWidget->SetFadeAlpha(FadeAlpha);
}

void AGTCFrontEndController::EnterPhase(EGTCFrontEndPhase NewPhase)
{
	Phase = NewPhase;
}

FString AGTCFrontEndController::ResolveMoviePath(const FString& ProjectRelativePath)
{
	// The boot videos may sit directly under Content/ or inside the licensed
	// GTCaliberAssets content submodule. Try each root in order and return the
	// first that actually exists, so the config paths stay short and root-agnostic.
	const FString ContentDir = FPaths::ProjectContentDir();
	const TCHAR* Roots[] =
	{
		TEXT(""),                              // Content/<rel>
		TEXT("GTCaliberAssets/Content/"),      // Content/GTCaliberAssets/Content/<rel>
	};

	FString FirstCandidate;
	for (const TCHAR* Root : Roots)
	{
		const FString Candidate = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(ContentDir, Root, ProjectRelativePath));
		if (FirstCandidate.IsEmpty())
		{
			FirstCandidate = Candidate;
		}
		if (FPaths::FileExists(Candidate))
		{
			return Candidate;
		}
	}
	// None found — return the canonical path so the warning log is meaningful.
	return FirstCandidate;
}

void AGTCFrontEndController::PlayMovie(const FString& ProjectRelativePath, bool bLoop)
{
	if (!MediaPlayer)
	{
		return;
	}
	const FString FullPath = ResolveMoviePath(ProjectRelativePath);
	MediaPlayer->Close();
	MediaPlayer->SetLooping(bLoop);
	if (!MediaPlayer->OpenFile(FullPath))
	{
		UE_LOG(LogGTCFrontEnd, Warning, TEXT("Front-end: failed to open movie '%s'."), *FullPath);
		// Don't hang the boot if a video is missing.
		if (Phase == EGTCFrontEndPhase::Intro)
		{
			HandleVideoEnded();
		}
	}
}

void AGTCFrontEndController::HandleVideoEnded()
{
	if (Phase != EGTCFrontEndPhase::Intro)
	{
		return; // menu video loops; ignore.
	}

	++IntroIndex;
	if (IntroMovies.IsValidIndex(IntroIndex))
	{
		PlayMovie(IntroMovies[IntroIndex], /*bLoop*/ false);
	}
	else
	{
		BeginHandoffToMenu();
	}
}

void AGTCFrontEndController::BeginHandoffToMenu()
{
	if (PendingAction == EPendingAction::ShowMenu)
	{
		return;
	}
	PendingAction = EPendingAction::ShowMenu;
	FadeDir = 1.0f; // fade to black, then Tick swaps to the menu video and fades up.
}

void AGTCFrontEndController::RequestSkipIntro()
{
	// Only allow skipping once the world has finished loading in the background,
	// so a skip never drops the player onto an unprepared world.
	if (Phase == EGTCFrontEndPhase::Intro && bWorldPreloaded)
	{
		BeginHandoffToMenu();
	}
}

void AGTCFrontEndController::BeginPreloadWorld()
{
	if (WorldMapPath.IsEmpty())
	{
		bWorldPreloaded = true;
		return;
	}
	LoadPackageAsync(
		WorldMapPath,
		FLoadPackageAsyncDelegate::CreateUObject(this, &AGTCFrontEndController::OnWorldPackageLoaded));
}

void AGTCFrontEndController::OnWorldPackageLoaded(const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
{
	bWorldPreloaded = true;
	PreloadProgress = 1.0f;
}

void AGTCFrontEndController::RequestStartGame()
{
	if (Phase != EGTCFrontEndPhase::Menu || bStarting)
	{
		return;
	}
	bStarting = true;
	LoadProgress = 0.0f;
	FrontEndWidget->SetVisual(EGTCFrontEndVisual::Loading);
	PendingAction = EPendingAction::OpenWorld;
	FadeDir = 1.0f;
}

void AGTCFrontEndController::OpenWorld()
{
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}

	// Hand off to the GameInstance, which raises the loading cover and keeps it up
	// across the travel until the world is loaded and rendering. The front-end
	// widget tears down with this controller on travel.
	if (UGTCGameInstance* GI = GetGameInstance<UGTCGameInstance>())
	{
		GI->BeginTravelToWorld(WorldMapPath, PreloadProgress);
		return;
	}

	// Fallback if the custom GameInstance isn't configured.
	UE_LOG(LogGTCFrontEnd, Warning, TEXT("Front-end: UGTCGameInstance missing; opening world directly."));
	UGameplayStatics::OpenLevel(this, FName(*WorldMapPath));
}

void AGTCFrontEndController::RequestSettings()
{
	// Minimal but real: toggle fullscreen/windowed and persist it. This is a
	// placeholder for a fuller settings panel (audio/quality/resolution) — the
	// button is wired to a working action rather than left dead.
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const EWindowMode::Type Current = Settings->GetFullscreenMode();
		const EWindowMode::Type Next = (Current == EWindowMode::Fullscreen || Current == EWindowMode::WindowedFullscreen)
			? EWindowMode::Windowed
			: EWindowMode::WindowedFullscreen;
		Settings->SetFullscreenMode(Next);
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}

void AGTCFrontEndController::RequestExit()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}
