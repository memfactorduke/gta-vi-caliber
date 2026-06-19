#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"
#include "GameFramework/PlayerController.h"
#include "GTCFrontEndController.generated.h"

class UMediaPlayer;
class UMediaTexture;
class UMediaSoundComponent;
class UFileMediaSource;
class SGTCFrontEnd;

/** Boot front-end phases, mirroring the original Godot flow:
 *  pre-intro video -> intro video -> looping main menu -> load world. */
UENUM()
enum class EGTCFrontEndPhase : uint8
{
	PreIntro,
	Intro,
	Menu,
	Starting
};

/**
 * Drives the game's front-end: plays the opening videos full-screen (skippable),
 * crossfades through black into the looping main menu, and on "Start Game" fades
 * out and opens the open-world map.
 *
 * The whole flow is C++/Slate + Media Framework, so it needs no editor-authored
 * UMG, MediaPlayer, or MediaTexture assets — the video objects are created at
 * runtime and pointed straight at the MP4s in Content/Movies.
 */
UCLASS()
class GTC_UE5_API AGTCFrontEndController : public APlayerController
{
	GENERATED_BODY()

public:
	AGTCFrontEndController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Called by the Slate widget. */
	void RequestSkipIntro();
	void RequestStartGame();
	void RequestSettings();
	void RequestExit();

protected:
	/** Map opened when the player presses Start Game (overridable in config). */
	UPROPERTY(EditDefaultsOnly, Config, Category = "GTC|FrontEnd")
	FString WorldMapPath = TEXT("/Game/GTCaliberAssets/Content/CityBeachStrip/Maps/CityBeachStrip");

	/** Ordered boot videos (project-relative to Content/), played before the menu. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "GTC|FrontEnd")
	TArray<FString> IntroMovies = { TEXT("Movies/PreIntro.mp4"), TEXT("Movies/Intro.mp4") };

	/** Looping background video shown behind the menu. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "GTC|FrontEnd")
	FString MenuMovie = TEXT("Movies/MenuLoop.mp4");

	UPROPERTY(EditDefaultsOnly, Config, Category = "GTC|FrontEnd")
	float FadeTime = 0.6f;

private:
	UFUNCTION()
	void HandleVideoEnded();

	void EnterPhase(EGTCFrontEndPhase NewPhase);
	void PlayMovie(const FString& ProjectRelativePath, bool bLoop);
	void BeginHandoffToMenu();
	void OpenWorld();
	static FString ResolveMoviePath(const FString& ProjectRelativePath);

	/** Kick off the background world load so it's warm before the player asks. */
	void BeginPreloadWorld();
	void OnWorldPackageLoaded(const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result);

	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> MediaTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMediaSoundComponent> MediaSound;

	TSharedPtr<SGTCFrontEnd> FrontEndWidget;

	EGTCFrontEndPhase Phase = EGTCFrontEndPhase::PreIntro;

	/** Index of the next intro movie to play. */
	int32 IntroIndex = 0;

	/** Signed fade direction: +1 fading to black, -1 fading up from black, 0 idle. */
	float FadeDir = 0.0f;
	float FadeAlpha = 0.0f;

	/** What to do once an in-progress fade-to-black completes. */
	enum class EPendingAction : uint8 { None, ShowMenu, OpenWorld };
	EPendingAction PendingAction = EPendingAction::None;

	/** Cosmetic loading-bar progress while the world streams in. */
	float LoadProgress = 0.0f;
	bool bStarting = false;

	/** Background preload of the world package (so Start is near-instant). */
	bool bWorldPreloaded = false;
	float PreloadProgress = 0.0f;

	/** Dev: seconds dwelt on the menu, for the gtc.FrontEndAutoStart capture hook. */
	float MenuDwell = 0.0f;
};
