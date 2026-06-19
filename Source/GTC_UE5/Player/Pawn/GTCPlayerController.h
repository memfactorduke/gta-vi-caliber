// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GTCPlayerController.generated.h"

class UInputMappingContext;
class SGTCPhone;
class SGTCPauseMenu;
class SGTCCharacterCreator;
class SGTCEmoteWheel;
class IConsoleObject;

/**
 * AGTCPlayerController — installs the default Enhanced Input mapping context and
 * hosts the in-world Slate UI ported from the legacy GTC project:
 *
 *   - the in-game smartphone (UI/Phone/SGTCPhone), toggled with Up / P or the
 *     gtc.Phone.* console commands; its apps bridge to live game systems
 *     (wallet -> PlayerState stats, wanted stars, pawn speed/heading, camera
 *     photos) through FGTCPhoneServices lambdas, and
 *   - the Esc pause menu (UI/Menus/SGTCPauseMenu).
 *
 * The IMC is soft-referenced by path (/Game/Input/IMC_Default) so the headless,
 * editor-closed build has no hard dependency on the editor-authored asset.
 */
UCLASS()
class GTC_UE5_API AGTCPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AGTCPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupInputComponent() override;

    /** Default mapping context, soft-referenced so headless builds don't hard-link the asset. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;

    /** Priority for the default mapping context. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    int32 DefaultMappingPriority = 0;

private:
    // ---- Input mode -----------------------------------------------------------
    /** Restore the captured, cursor-hidden gameplay input mode and force the game
     *  viewport to lock + permanently capture the mouse. Plain FInputModeGameOnly
     *  is not enough under PIE, where the OS cursor otherwise drifts out of the
     *  viewport and mouse-look freezes at the edge / whips on re-entry. Called at
     *  BeginPlay and whenever a UI panel closes back to gameplay. */
    void EnterGameInputMode();

    // ---- Phone ----------------------------------------------------------------
    void TogglePhone();
    void OpenPhone();
    void ClosePhone();
    void OnPhoneClosed();
    void PhoneAppCmd(const TArray<FString>& Args);

    TSharedPtr<SGTCPhone> Phone;
    bool bPhoneOpen = false;
    bool bPhoneInViewport = false;

    // ---- Esc pause menu -------------------------------------------------------
    void TogglePauseMenu();
    void OpenPauseMenu();
    void ClosePauseMenu();

    TSharedPtr<SGTCPauseMenu> PauseMenu;
    bool bPauseMenuOpen = false;

    // ---- Character creator ----------------------------------------------------
    void ToggleCharacterCreator();
    void OpenCharacterCreator();
    void CloseCharacterCreator();

    /** Polled shortly after BeginPlay until the pawn is possessed, then opens the
     *  creator once. Runs on every gameplay map, so the creator always appears. */
    void TryAutoOpenCreator();

    TSharedPtr<SGTCCharacterCreator> CharacterCreator;
    bool bCreatorOpen = false;
    bool bCreatorAutoShown = false;
    FTimerHandle CreatorAutoOpenTimer;

    // ---- Emote panel (one key opens a picker for all emotes) ------------------
    void ToggleEmoteWheel();
    void OpenEmoteWheel();
    void CloseEmoteWheel();

    TSharedPtr<SGTCEmoteWheel> EmoteWheel;
    bool bEmoteWheelOpen = false;

    /** Registered phone/dev console commands, unregistered on EndPlay. */
    TArray<IConsoleObject*> PhoneConsoleCmds;
};
