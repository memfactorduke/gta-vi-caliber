#include "GTCGameInstance.h"
#include "../FrontEnd/SGTCLoadingScreen.h"
#include "../Systems/Save/SaveSubsystem.h"

#include "Engine/Texture2D.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "HAL/IConsoleManager.h"
#include "MoviePlayer.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGTC, Log, All);

static TAutoConsoleVariable<float> CVarGTCCaptureInterval(
	TEXT("gtc.CaptureInterval"), 0.0f,
	TEXT("Dev: if > 0, write an engine screenshot every N seconds to Saved/Screenshots."));

namespace
{
	// Curated, like the Godot prototype's explicit list — missing files are skipped.
	static const TCHAR* GLoadingArtFiles[] =
	{
		TEXT("pawn_crew.png"),
		TEXT("crypto_atm.png"),
		TEXT("news_anchor.png"),
		TEXT("sheriff_gator.png"),
		TEXT("suncoast_flood.png"),
		TEXT("hurricane_supplies.png"),
		TEXT("airboat_swamp.png"),
		TEXT("retirement_scooter.png"),
	};
}

void UGTCGameInstance::Init()
{
	Super::Init();
	UE_LOG(LogGTC, Log, TEXT("GTC GameInstance initialized."));

	LoadLoadingArt();
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGTCGameInstance::OnPreLoadMap);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGTCGameInstance::OnPostLoadMapWithWorld);
	CaptureTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UGTCGameInstance::CaptureTick), 0.0f);

	// Subsystems exist after Super::Init(); register the "player" save section.
	RegisterSaveHooks(GetSubsystem<USaveSubsystem>());
}

void UGTCGameInstance::Shutdown()
{
	UE_LOG(LogGTC, Log, TEXT("GTC GameInstance shutting down."));
	UnregisterSaveHooks();
	if (PreLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
		PreLoadMapHandle.Reset();
	}
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	if (CaptureTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(CaptureTickerHandle);
		CaptureTickerHandle.Reset();
	}
	if (LoadingScreen.IsValid())
	{
		if (UGameViewportClient* VPC = GetGameViewportClient())
		{
			VPC->RemoveViewportWidgetContent(LoadingScreen.ToSharedRef());
		}
		LoadingScreen.Reset();
	}
	Super::Shutdown();
}

bool UGTCGameInstance::RegisterSaveHooks(USaveSubsystem* Save)
{
	if (Save == nullptr)
	{
		return false;
	}
	FSaveHook OnSave;
	OnSave.BindUObject(this, &UGTCGameInstance::OnSavePlayer);
	FLoadHook OnLoad;
	OnLoad.BindUObject(this, &UGTCGameInstance::OnLoadPlayer);
	if (!Save->RegisterHook(TEXT("player"), OnSave, OnLoad))
	{
		return false;
	}
	RegisteredSave = Save;
	return true;
}

void UGTCGameInstance::UnregisterSaveHooks()
{
	if (RegisteredSave)
	{
		RegisteredSave->UnregisterHook(TEXT("player"));
	}
	RegisteredSave = nullptr;
}

void UGTCGameInstance::OnSavePlayer(const TSharedRef<FGtcJsonObject>& SectionOut)
{
	// Persist the created look as plain numbers (bool stored as 0/1 — the JSON model's
	// read side is number/string; SetBool has no symmetric GetBool).
	SectionOut->SetNumber(TEXT("look_valid"), SavedLook.bValid ? 1.0 : 0.0);
	SectionOut->SetNumber(TEXT("look_body"), SavedLook.Body);
	SectionOut->SetNumber(TEXT("look_face"), SavedLook.Face);
	SectionOut->SetNumber(TEXT("look_hair"), SavedLook.Hair);
	SectionOut->SetNumber(TEXT("look_outfit"), SavedLook.Outfit);
	SectionOut->SetNumber(TEXT("look_skin"), SavedLook.Skin);

	// Throwable ammo (the pawn pushes it up on every change).
	SectionOut->SetNumber(TEXT("ammo_valid"), bHasSavedAmmo ? 1.0 : 0.0);
	SectionOut->SetNumber(TEXT("ammo_flashbang"), SavedFlashbang);
	SectionOut->SetNumber(TEXT("ammo_grenade"), SavedGrenade);
	SectionOut->SetNumber(TEXT("ammo_molotov"), SavedMolotov);

	// Player transform: pull the live pawn's position/yaw at save time (continuous state,
	// so it's pulled here rather than pushed). Generic APawn API — no pawn-type dependency.
	if (APlayerController* PC = GetFirstLocalPlayerController())
	{
		if (APawn* P = PC->GetPawn())
		{
			SavedPlayerLocation = P->GetActorLocation();
			SavedPlayerYaw = static_cast<float>(P->GetActorRotation().Yaw);
			bHasSavedTransform = true;
		}
	}
	SectionOut->SetNumber(TEXT("transform_valid"), bHasSavedTransform ? 1.0 : 0.0);
	SectionOut->SetNumber(TEXT("pos_x"), SavedPlayerLocation.X);
	SectionOut->SetNumber(TEXT("pos_y"), SavedPlayerLocation.Y);
	SectionOut->SetNumber(TEXT("pos_z"), SavedPlayerLocation.Z);
	SectionOut->SetNumber(TEXT("yaw"), SavedPlayerYaw);
}

void UGTCGameInstance::OnLoadPlayer(const TSharedRef<FGtcJsonObject>& SectionIn)
{
	// An absent/never-confirmed look leaves the current look untouched (no clobber).
	if (SectionIn->GetNumber(TEXT("look_valid"), 0.0) <= 0.5)
	{
		return;
	}
	FGTCPlayerLook Look;
	Look.Body = static_cast<int32>(SectionIn->GetNumber(TEXT("look_body"), 0.0));
	Look.Face = static_cast<int32>(SectionIn->GetNumber(TEXT("look_face"), 0.0));
	Look.Hair = static_cast<int32>(SectionIn->GetNumber(TEXT("look_hair"), 0.0));
	Look.Outfit = static_cast<int32>(SectionIn->GetNumber(TEXT("look_outfit"), 0.0));
	Look.Skin = static_cast<int32>(SectionIn->GetNumber(TEXT("look_skin"), 0.0));
	SetSavedLook(Look); // marks bValid + stores; pawns restore it on spawn

	// Throwable ammo (absent → leave the loadout's starting allotment).
	if (SectionIn->GetNumber(TEXT("ammo_valid"), 0.0) > 0.5)
	{
		SetSavedThrowableAmmo(
			static_cast<int32>(SectionIn->GetNumber(TEXT("ammo_flashbang"), 0.0)),
			static_cast<int32>(SectionIn->GetNumber(TEXT("ammo_grenade"), 0.0)),
			static_cast<int32>(SectionIn->GetNumber(TEXT("ammo_molotov"), 0.0)));
	}

	// Player transform (absent → spawn at the level's PlayerStart as normal).
	if (SectionIn->GetNumber(TEXT("transform_valid"), 0.0) > 0.5)
	{
		const FVector Loc(
			SectionIn->GetNumber(TEXT("pos_x"), 0.0),
			SectionIn->GetNumber(TEXT("pos_y"), 0.0),
			SectionIn->GetNumber(TEXT("pos_z"), 0.0));
		SetSavedTransform(Loc, static_cast<float>(SectionIn->GetNumber(TEXT("yaw"), 0.0)));
	}
}

void UGTCGameInstance::LoadLoadingArt()
{
	// Key art may sit under Content/ or inside the licensed GTCaliberAssets content
	// submodule; check each root so the curated set loads wherever it ships.
	const FString ContentDir = FPaths::ProjectContentDir();
	const TCHAR* Roots[] =
	{
		TEXT("FrontEnd/LoadingArt"),
		TEXT("GTCaliberAssets/Content/FrontEnd/LoadingArt"),
	};

	for (const TCHAR* File : GLoadingArtFiles)
	{
		for (const TCHAR* Root : Roots)
		{
			const FString Path = FPaths::Combine(ContentDir, Root, File);
			if (!FPaths::FileExists(Path))
			{
				continue;
			}
			if (UTexture2D* Tex = FImageUtils::ImportFileAsTexture2D(Path))
			{
				LoadingArt.Add(Tex);
			}
			break; // first root that has this file wins
		}
	}
	UE_LOG(LogGTC, Log, TEXT("GTC: loaded %d loading-screen art textures."), LoadingArt.Num());
}

void UGTCGameInstance::ShowLoadingScreen(float InitialProgress)
{
	if (!LoadingScreen.IsValid())
	{
		TArray<UTexture2D*> Art;
		Art.Reserve(LoadingArt.Num());
		for (const TObjectPtr<UTexture2D>& Tex : LoadingArt)
		{
			Art.Add(Tex.Get());
		}

		LoadingScreen = SNew(SGTCLoadingScreen)
			.ArtTextures(Art)
			.OnDismissed(FSimpleDelegate::CreateUObject(this, &UGTCGameInstance::OnLoadingDismissed));

		if (UGameViewportClient* VPC = GetGameViewportClient())
		{
			VPC->AddViewportWidgetContent(LoadingScreen.ToSharedRef(), 10000);
		}
	}
	LoadingScreen->SetTargetProgress(FMath::Max(InitialProgress, 0.92f));
}

void UGTCGameInstance::SetLoadingProgress(float Progress)
{
	if (LoadingScreen.IsValid())
	{
		LoadingScreen->SetTargetProgress(Progress);
	}
}

TSharedRef<SWidget> UGTCGameInstance::BuildLoadingWidget()
{
	TArray<UTexture2D*> Art;
	Art.Reserve(LoadingArt.Num());
	for (const TObjectPtr<UTexture2D>& Tex : LoadingArt)
	{
		Art.Add(Tex.Get());
	}
	TSharedRef<SGTCLoadingScreen> Screen = SNew(SGTCLoadingScreen).ArtTextures(Art);
	Screen->SetDone(); // load happens behind it; the bar eases to full
	return Screen;
}

void UGTCGameInstance::BeginTravelToWorld(const FString& MapPath, float InitialProgress)
{
	bTravelingToWorld = true;
	UE_LOG(LogGTC, Log, TEXT("GTC: traveling to world '%s'."), *MapPath);
	// The loading screen is raised by OnPreLoadMap (movie player) so it keeps
	// animating on its own thread through the synchronous world load.
	UGameplayStatics::OpenLevel(this, FName(*MapPath));
}

void UGTCGameInstance::OnPreLoadMap(const FString& MapName)
{
	if (!bTravelingToWorld)
	{
		return;
	}
	// Movie-player loading screen: rendered on a dedicated thread during the
	// blocking travel, so the slideshow + bar stay smooth through the hitch.
	// Holds until the map finishes loading AND a minimum display time (covers the
	// first render warmup), so the world is never revealed half-built.
	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.bMoviesAreSkippable = false;
	Attributes.bWaitForManualStop = false;
	Attributes.MinimumLoadingScreenDisplayTime = 3.5f;
	Attributes.WidgetLoadingScreen = BuildLoadingWidget();
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

void UGTCGameInstance::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!bTravelingToWorld || LoadedWorld == nullptr)
	{
		return;
	}
	bTravelingToWorld = false;

	if (LoadingScreen.IsValid())
	{
		// Legacy viewport-cover path: fill the bar before revealing.
		LoadingScreen->SetDone();
	}
	// Deferred so the gameplay controller/pawn exist: dismiss any legacy cover and
	// restore game input (works for both the movie-player and viewport paths).
	LoadedWorld->GetTimerManager().SetTimer(
		RevealTimer, this, &UGTCGameInstance::OnReadyToReveal, RenderWarmupSeconds, /*bLoop*/ false);
}

void UGTCGameInstance::OnReadyToReveal()
{
	if (LoadingScreen.IsValid())
	{
		LoadingScreen->BeginDismiss();
	}

	// The movie-player loading screen auto-hides on its own (after its minimum display
	// time), so OnLoadingDismissed won't fire on that path — restore gameplay input here
	// too, and keep re-asserting it for a few seconds so the late movie-player teardown
	// can't steal focus back and leave the character input-dead.
	RestoreGameInput(/*bFlushKeys=*/true);
	InputAssertSecondsRemaining = 6.0f;
}

void UGTCGameInstance::RestoreGameInput(bool bFlushKeys)
{
	APlayerController* PC = GetFirstLocalPlayerController();
	if (!PC)
	{
		return;
	}
	PC->SetInputMode(FInputModeGameOnly());
	PC->SetShowMouseCursor(false);
	// Flush only on the initial restore — flushing every frame would eat the keys the
	// player is actively pressing during the assert window.
	if (bFlushKeys)
	{
		PC->FlushPressedKeys();
	}
}

bool UGTCGameInstance::CaptureTick(float DeltaTime)
{
	// Re-assert game input every frame for a short window after travel, so the
	// late-tearing-down movie-player loading screen can't leave the viewport unfocused.
	if (InputAssertSecondsRemaining > 0.0f)
	{
		InputAssertSecondsRemaining -= DeltaTime;
		RestoreGameInput(/*bFlushKeys=*/false);
	}

	const float Interval = CVarGTCCaptureInterval.GetValueOnGameThread();
	if (Interval <= 0.0f)
	{
		CaptureAccum = 0.0f;
		return true;
	}
	CaptureAccum += DeltaTime;
	if (CaptureAccum >= Interval)
	{
		CaptureAccum = 0.0f;
		const FString FileName = FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Screenshots"),
			FString::Printf(TEXT("gtc_%04d.png"), CaptureIndex++));
		FScreenshotRequest::RequestScreenshot(FileName, /*bShowUI*/ true, /*bAddUniqueSuffix*/ false);
	}
	return true; // keep ticking
}

void UGTCGameInstance::OnLoadingDismissed()
{
	if (LoadingScreen.IsValid())
	{
		if (UGameViewportClient* VPC = GetGameViewportClient())
		{
			VPC->RemoveViewportWidgetContent(LoadingScreen.ToSharedRef());
		}
		LoadingScreen.Reset();
	}

	// Hand input back to gameplay. Unreal does NOT reset the viewport's input mode on
	// level travel, so the FInputModeUIOnly + menu focus the front-end set would
	// otherwise linger into the city and leave the character unable to move.
	RestoreGameInput(/*bFlushKeys=*/true);
	InputAssertSecondsRemaining = 6.0f;
}
