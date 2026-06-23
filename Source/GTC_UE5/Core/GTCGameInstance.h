#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "../Player/Pawn/GTCPlayerLook.h"
#include "../Systems/Save/SaveJson.h"
#include "GTCGameInstance.generated.h"

class UTexture2D;
class SGTCLoadingScreen;
class USaveSubsystem;

/**
 * Root game instance — persists across level loads. Besides long-lived systems,
 * it owns the front-end loading cover: because the GameInstance (and its game
 * viewport) survive `OpenLevel`, the loading screen added here stays on screen
 * straight through the travel and is only torn down once the destination world
 * has actually loaded and rendered — so the player never sees a half-built world.
 */
UCLASS()
class GTC_UE5_API UGTCGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** Key-art for the loading slideshow, loaded from disk at startup. */
	const TArray<TObjectPtr<UTexture2D>>& GetLoadingArt() const { return LoadingArt; }

	/** Show the cover (if not already up) and feed it a starting progress. */
	void ShowLoadingScreen(float InitialProgress);
	void SetLoadingProgress(float Progress);

	/** Show the cover, then travel to the given map; the cover persists across
	 *  the load and clears only once the new world is up and rendering. */
	void BeginTravelToWorld(const FString& MapPath, float InitialProgress);

	// --- Persistent player look (survives level travel) ------------------------

	/** True once the player has confirmed a character in the creator. */
	bool HasSavedLook() const { return SavedLook.bValid; }

	/** The look to restore onto the player pawn when it spawns on any map. */
	const FGTCPlayerLook& GetSavedLook() const { return SavedLook; }

	/** Stash the look the player built; marked valid so it restores on every spawn. */
	void SetSavedLook(const FGTCPlayerLook& In)
	{
		SavedLook = In;
		SavedLook.bValid = true;
	}

	// --- Persistent throwable ammo (survives travel + save/load) ----------------
	// The pawn PUSHES its ammo here whenever it changes (pawn -> GameInstance, no reverse
	// dependency) and RESTORES from here on spawn, exactly like the look.

	bool HasSavedThrowableAmmo() const { return bHasSavedAmmo; }

	void GetSavedThrowableAmmo(int32& OutFlashbang, int32& OutGrenade, int32& OutMolotov) const
	{
		OutFlashbang = SavedFlashbang;
		OutGrenade = SavedGrenade;
		OutMolotov = SavedMolotov;
	}

	void SetSavedThrowableAmmo(int32 Flashbang, int32 Grenade, int32 Molotov)
	{
		SavedFlashbang = Flashbang;
		SavedGrenade = Grenade;
		SavedMolotov = Molotov;
		bHasSavedAmmo = true;
	}

	// --- Persistent player transform ("reload where you left off") --------------
	// Pulled from the live pawn at save time (continuous state, not event-driven), held
	// here, and applied on the next pawn spawn — overriding the spawn-relocation fallback.

	bool HasSavedTransform() const { return bHasSavedTransform; }

	void GetSavedTransform(FVector& OutLocation, float& OutYaw) const
	{
		OutLocation = SavedPlayerLocation;
		OutYaw = SavedPlayerYaw;
	}

	void SetSavedTransform(const FVector& Location, float Yaw)
	{
		SavedPlayerLocation = Location;
		SavedPlayerYaw = Yaw;
		bHasSavedTransform = true;
	}

	// --- Save persistence (player owner) ---------------------------------------
	// The GameInstance is the persistent owner of player save state (it already carries
	// the look across travel), so it registers the "player" save section with
	// USaveSubsystem. This first slice persists the created look to disk; future player
	// state (transform, throwable ammo, money) extends the SAME hook/section. Split out
	// RegisterSaveHooks so a test can wire a directly-constructed USaveSubsystem.

	/** Bind + register the "player" save/load hooks with `Save`. Idempotent; false if null. */
	bool RegisterSaveHooks(USaveSubsystem* Save);

	/** Drop the "player" hooks from the USaveSubsystem they were registered with. */
	void UnregisterSaveHooks();

private:
	/** Save hook: write the persistent player state (currently the look) into "player". */
	void OnSavePlayer(const TSharedRef<FGtcJsonObject>& SectionOut);

	/** Load hook: restore player state from "player" (absent/invalid -> unchanged). */
	void OnLoadPlayer(const TSharedRef<FGtcJsonObject>& SectionIn);

	/** The USaveSubsystem the "player" hook is registered with (non-owning). */
	UPROPERTY(Transient)
	TObjectPtr<USaveSubsystem> RegisteredSave = nullptr;

	void LoadLoadingArt();
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);
	void OnReadyToReveal();
	void OnLoadingDismissed();
	TSharedRef<class SWidget> BuildLoadingWidget();

	/** Force the gameplay controller into game-only input. Re-asserted for a few seconds
	 *  after travel (see InputAssertSecondsRemaining) because the movie-player loading
	 *  screen releases the viewport late and can leave it unfocused / input-dead. */
	void RestoreGameInput(bool bFlushKeys);

	/** Counts down after reveal; while > 0, CaptureTick re-applies game input each frame. */
	float InputAssertSecondsRemaining = 0.0f;

	/** Dev-only: when CVar gtc.CaptureInterval > 0, write an engine screenshot
	 *  every N seconds (bypasses OS screen-recording permissions). */
	bool CaptureTick(float DeltaTime);
	FTSTicker::FDelegateHandle CaptureTickerHandle;
	float CaptureAccum = 0.0f;
	int32 CaptureIndex = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> LoadingArt;

	TSharedPtr<SGTCLoadingScreen> LoadingScreen;

	/** Player's confirmed appearance, persisted here so it carries across maps. */
	UPROPERTY()
	FGTCPlayerLook SavedLook;

	/** Latest throwable ammo the pawn pushed up, persisted in the "player" save section. */
	int32 SavedFlashbang = 0;
	int32 SavedGrenade = 0;
	int32 SavedMolotov = 0;
	bool bHasSavedAmmo = false;

	/** Player transform pulled at save time, applied on the next pawn spawn. */
	FVector SavedPlayerLocation = FVector::ZeroVector;
	float SavedPlayerYaw = 0.0f;
	bool bHasSavedTransform = false;

	/** True between BeginTravelToWorld and the destination's PostLoadMapWithWorld. */
	bool bTravelingToWorld = false;

	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
	FTimerHandle RevealTimer;

	/** Seconds the world renders behind the cover before we reveal it. */
	static constexpr float RenderWarmupSeconds = 2.0f;
};
