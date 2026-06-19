// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPlayerController.h"

#include "GTCPlayerState.h"
#include "GTCPlayerCharacter.h"
#include "../Stats/PlayerStats.h"
#include "../../Systems/Wanted/WantedSubsystem.h"
#include "../../UI/Phone/SGTCPhone.h"
#include "../../UI/Menus/SGTCPauseMenu.h"
#include "../../UI/Menus/SGTCCharacterCreator.h"
#include "../../UI/Menus/SGTCEmoteWheel.h"

#include "TimerManager.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "UnrealClient.h"

AGTCPlayerController::AGTCPlayerController()
{
    // Soft path so the headless (editor-closed) build never hard-links the
    // editor-authored IMC asset. Resolved at BeginPlay if present.
    DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Default.IMC_Default")));
}

void AGTCPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        // LoadSynchronous tolerates a missing asset (returns null) so the
        // headless build never fails when the editor-authored IMC isn't present.
        if (UInputMappingContext* Imc = DefaultMappingContext.LoadSynchronous())
        {
            Subsystem->AddMappingContext(Imc, DefaultMappingPriority);
        }
    }

    if (IsLocalPlayerController())
    {
        // Lock + capture the mouse to the viewport for mouse-look. Under PIE this
        // is what stops the cursor escaping (camera freezing at the screen edge,
        // then whipping when the cursor re-enters). The creator auto-opens shortly
        // after and temporarily hands the cursor back; closing it re-locks here.
        EnterGameInputMode();

        // Console hooks for the phone (also let a headless run drive it).
        IConsoleManager& CM = IConsoleManager::Get();
        PhoneConsoleCmds.Add(CM.RegisterConsoleCommand(TEXT("gtc.Phone.Toggle"), TEXT("Raise/lower the in-game phone."),
            FConsoleCommandDelegate::CreateUObject(this, &AGTCPlayerController::TogglePhone), ECVF_Default));
        PhoneConsoleCmds.Add(CM.RegisterConsoleCommand(TEXT("gtc.Phone.Open"), TEXT("Raise the in-game phone."),
            FConsoleCommandDelegate::CreateUObject(this, &AGTCPlayerController::OpenPhone), ECVF_Default));
        PhoneConsoleCmds.Add(CM.RegisterConsoleCommand(TEXT("gtc.Phone.Close"), TEXT("Lower the in-game phone."),
            FConsoleCommandDelegate::CreateUObject(this, &AGTCPlayerController::ClosePhone), ECVF_Default));
        PhoneConsoleCmds.Add(CM.RegisterConsoleCommand(TEXT("gtc.Phone.App"), TEXT("Open the phone to an app index (0 home, 1..16)."),
            FConsoleCommandWithArgsDelegate::CreateUObject(this, &AGTCPlayerController::PhoneAppCmd), ECVF_Default));

        // Character creator console hooks (also let a headless run drive it).
        PhoneConsoleCmds.Add(CM.RegisterConsoleCommand(TEXT("gtc.Creator.Open"), TEXT("Open the character creator."),
            FConsoleCommandDelegate::CreateUObject(this, &AGTCPlayerController::OpenCharacterCreator), ECVF_Default));
        PhoneConsoleCmds.Add(CM.RegisterConsoleCommand(TEXT("gtc.Creator.Close"), TEXT("Close the character creator."),
            FConsoleCommandDelegate::CreateUObject(this, &AGTCPlayerController::CloseCharacterCreator), ECVF_Default));
        PhoneConsoleCmds.Add(CM.RegisterConsoleCommand(TEXT("gtc.Creator.Toggle"), TEXT("Toggle the character creator."),
            FConsoleCommandDelegate::CreateUObject(this, &AGTCPlayerController::ToggleCharacterCreator), ECVF_Default));

        // Auto-show the creator on every gameplay map. Poll until the pawn is
        // possessed (it usually isn't yet at controller BeginPlay), then open once.
        if (UWorld* W = GetWorld())
        {
            W->GetTimerManager().SetTimer(
                CreatorAutoOpenTimer, this, &AGTCPlayerController::TryAutoOpenCreator, 0.25f, true, 0.5f);
        }
    }
}

void AGTCPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    IConsoleManager& CM = IConsoleManager::Get();
    for (IConsoleObject* Cmd : PhoneConsoleCmds)
    {
        if (Cmd) CM.UnregisterConsoleObject(Cmd);
    }
    PhoneConsoleCmds.Reset();
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(CreatorAutoOpenTimer);
    }

    // Tear down any open panel before this controller dies. The game viewport is
    // owned by the GameInstance and SURVIVES non-seamless travel (OpenLevel), so a
    // panel left in it would keep painting over the next map and the raw TSharedPtr
    // would keep the widget alive past its now-destroyed owner. Remove directly
    // (don't route through the close handlers — ClosePhone defers removal to an
    // animation callback that won't run on a destroyed controller).
    if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
    {
        if (Phone.IsValid()) VPC->RemoveViewportWidgetContent(Phone.ToSharedRef());
        if (PauseMenu.IsValid()) VPC->RemoveViewportWidgetContent(PauseMenu.ToSharedRef());
        if (CharacterCreator.IsValid()) VPC->RemoveViewportWidgetContent(CharacterCreator.ToSharedRef());
        if (EmoteWheel.IsValid()) VPC->RemoveViewportWidgetContent(EmoteWheel.ToSharedRef());
    }
    Phone.Reset();
    PauseMenu.Reset();
    CharacterCreator.Reset();
    EmoteWheel.Reset();
    bPhoneOpen = bPhoneInViewport = bPauseMenuOpen = bCreatorOpen = bEmoteWheelOpen = false;

    Super::EndPlay(EndPlayReason);
}

void AGTCPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Bound at the controller level (not Enhanced Input) so they work regardless of the
    // possessed pawn's mapping contexts, and Esc still works while the menu is up.
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AGTCPlayerController::TogglePauseMenu);

    // Phone: Up arrow (GTA muscle memory) or P. Closing is handled by the widget itself.
    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AGTCPlayerController::TogglePhone);
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AGTCPlayerController::TogglePhone);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &AGTCPlayerController::TogglePhone);

    // Character creator: F4 to reopen it any time (it also auto-shows on map start
    // and has an "Edit Character" entry in the Esc menu).
    InputComponent->BindKey(EKeys::F4, IE_Pressed, this, &AGTCPlayerController::ToggleCharacterCreator);

    // Emote panel: ONE key (B, or gamepad face-left) opens the picker for all emotes —
    // replaces the old per-emote hard keys (H/G/J).
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AGTCPlayerController::ToggleEmoteWheel);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AGTCPlayerController::ToggleEmoteWheel);
}

void AGTCPlayerController::EnterGameInputMode()
{
    FInputModeGameOnly Mode;
    Mode.SetConsumeCaptureMouseDown(true);
    SetInputMode(Mode);
    SetShowMouseCursor(false);

    // FInputModeGameOnly alone leaves PIE free to let the OS cursor wander out of
    // the viewport. Force the viewport itself to capture permanently and lock the
    // cursor inside it so mouse-look gets continuous, bounded deltas.
    if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
    {
        VPC->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
        VPC->SetMouseLockMode(EMouseLockMode::LockAlways);
        VPC->SetHideCursorDuringCapture(true);
    }
}

// ============================================================================
//  Phone
// ============================================================================

void AGTCPlayerController::TogglePhone()
{
    if (bPhoneOpen) ClosePhone();
    else OpenPhone();
}

void AGTCPlayerController::OpenPhone()
{
    // Don't raise the phone over a modal panel (pause menu or creator) — the phone
    // would steal focus/cursor and, on close, strip their input mode. Mirrors the
    // creator's own guard.
    if (!IsLocalPlayerController() || bPauseMenuOpen || bCreatorOpen)
    {
        return;
    }

    if (!Phone.IsValid())
    {
        // Bridge the apps to live game systems. Each callback re-resolves the pawn/state so
        // it survives respawns. Services left unset are treated as no-ops by the widget, so
        // apps without a backing system (stamina, view mode, time-of-day, live map) simply
        // fall back to their stylised defaults.
        FGTCPhoneServices Svc;
        Svc.GetCash = [this]() -> int64
        {
            const AGTCPlayerState* PS = GetPlayerState<AGTCPlayerState>();
            return (PS && PS->StatsComponent) ? PS->StatsComponent->GetMoney() : 0;
        };
        Svc.AddCash = [this](int64 D)
        {
            if (AGTCPlayerState* PS = GetPlayerState<AGTCPlayerState>())
                if (PS->StatsComponent) PS->StatsComponent->AddMoney((int32)D);
        };
        Svc.TrySpend = [this](int64 A) -> bool
        {
            AGTCPlayerState* PS = GetPlayerState<AGTCPlayerState>();
            return (PS && PS->StatsComponent) ? PS->StatsComponent->SpendMoney((int32)A) : false;
        };
        Svc.GetWantedLevel = [this]() -> int32
        {
            UWantedSubsystem* W = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWantedSubsystem>() : nullptr;
            return W ? W->Stars() : 0;
        };
        Svc.GetSpeedKmh = [this]() -> float { const APawn* P = GetPawn(); return P ? P->GetVelocity().Size() * 0.036f : 0.f; };
        Svc.GetHeadingDeg = [this]() -> float { const APawn* P = GetPawn(); return P ? P->GetActorRotation().Yaw : 0.f; };
        Svc.GetLocationText = [this]() -> FString
        {
            if (const APawn* P = GetPawn())
            {
                const FVector L = P->GetActorLocation();
                return FString::Printf(TEXT("Caliber Isles  ·  %.0f, %.0f"), L.X / 100.f, L.Y / 100.f);
            }
            return TEXT("Caliber Isles");
        };
        Svc.TakePhoto = [this]()
        {
            const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
            IFileManager::Get().MakeDirectory(*Dir, true);
            const FString File = FPaths::Combine(Dir, FString::Printf(TEXT("gtcphoto_%s.png"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))));
            FScreenshotRequest::RequestScreenshot(File, /*bShowUI*/ false, /*bAddUniqueSuffix*/ false);
        };
        Svc.ListPhotos = [this]() -> TArray<FString>
        {
            const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
            TArray<FString> Names;
            IFileManager::Get().FindFiles(Names, *(Dir / TEXT("gtcphoto_*.png")), true, false);
            Names.Sort([](const FString& A, const FString& B) { return A > B; }); // newest first
            TArray<FString> Out;
            for (const FString& N : Names) Out.Add(Dir / N);
            return Out;
        };

        Phone = SNew(SGTCPhone)
            .Services(Svc)
            .OnCloseRequested(FSimpleDelegate::CreateUObject(this, &AGTCPlayerController::ClosePhone))
            .OnFullyClosed(FSimpleDelegate::CreateUObject(this, &AGTCPlayerController::OnPhoneClosed));
    }

    if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
    {
        if (!bPhoneInViewport)
        {
            VPC->AddViewportWidgetContent(Phone.ToSharedRef(), 2000);
            bPhoneInViewport = true;
        }
    }

    bPhoneOpen = true;

    // Free the cursor for tapping; the world keeps running (no pause) so it feels live.
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(Phone);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    SetInputMode(Mode);
    SetShowMouseCursor(true);

    Phone->AnimateOpen();
}

void AGTCPlayerController::ClosePhone()
{
    if (!bPhoneOpen) return;
    bPhoneOpen = false;

    // Hand control straight back; the widget plays its lower animation, then OnPhoneClosed
    // pulls it from the viewport.
    EnterGameInputMode();
    if (Phone.IsValid()) Phone->AnimateClose();
}

void AGTCPlayerController::OnPhoneClosed()
{
    if (bPhoneOpen) return; // reopened mid-animation
    if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
    {
        if (Phone.IsValid() && bPhoneInViewport)
        {
            VPC->RemoveViewportWidgetContent(Phone.ToSharedRef());
            bPhoneInViewport = false;
        }
    }
}

void AGTCPlayerController::PhoneAppCmd(const TArray<FString>& Args)
{
    OpenPhone();
    const int32 Idx = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
    if (Phone.IsValid()) Phone->NavigateToAppIndex(Idx);
}

// ============================================================================
//  Esc pause menu
// ============================================================================

void AGTCPlayerController::TogglePauseMenu()
{
    if (bPauseMenuOpen) ClosePauseMenu();
    else OpenPauseMenu();
}

void AGTCPlayerController::OpenPauseMenu()
{
    if (bPauseMenuOpen)
    {
        return;
    }

    if (!PauseMenu.IsValid())
    {
        // Resume is wired; the view-mode / mission-editor hooks are left unbound (no-op via
        // ExecuteIfBound) until this project grows a first/third-person view system and an
        // in-game mission-editor widget.
        PauseMenu = SNew(SGTCPauseMenu)
            .OnResume(FSimpleDelegate::CreateUObject(this, &AGTCPlayerController::ClosePauseMenu))
            .OnEditCharacter(FSimpleDelegate::CreateLambda([this]()
            {
                ClosePauseMenu();
                OpenCharacterCreator();
            }));
    }

    if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
    {
        VPC->AddViewportWidgetContent(PauseMenu.ToSharedRef(), 5000);
    }

    bPauseMenuOpen = true;
    UGameplayStatics::SetGamePaused(this, true);

    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(PauseMenu);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    SetShowMouseCursor(true);
}

void AGTCPlayerController::ClosePauseMenu()
{
    if (PauseMenu.IsValid())
    {
        if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
        {
            VPC->RemoveViewportWidgetContent(PauseMenu.ToSharedRef());
        }
    }

    bPauseMenuOpen = false;
    UGameplayStatics::SetGamePaused(this, false);

    EnterGameInputMode();
}

// ============================================================================
//  Character creator
// ============================================================================

void AGTCPlayerController::TryAutoOpenCreator()
{
    if (bCreatorAutoShown)
    {
        if (UWorld* W = GetWorld())
        {
            W->GetTimerManager().ClearTimer(CreatorAutoOpenTimer);
        }
        return;
    }
    // Wait until a player pawn is actually possessed before showing the creator.
    if (Cast<AGTCPlayerCharacter>(GetPawn()) == nullptr)
    {
        return;
    }
    bCreatorAutoShown = true;
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(CreatorAutoOpenTimer);
    }
    OpenCharacterCreator();
}

void AGTCPlayerController::ToggleCharacterCreator()
{
    if (bCreatorOpen) CloseCharacterCreator();
    else OpenCharacterCreator();
}

void AGTCPlayerController::OpenCharacterCreator()
{
    if (!IsLocalPlayerController() || bCreatorOpen || bPauseMenuOpen)
    {
        return;
    }
    if (Cast<AGTCPlayerCharacter>(GetPawn()) == nullptr)
    {
        return; // nothing to dress yet
    }

    if (!CharacterCreator.IsValid())
    {
        // Each callback re-resolves the pawn so the creator survives a respawn.
        FGTCCreatorServices Svc;
        Svc.Cycle = [this](int32 Slot, int32 Delta)
        {
            if (AGTCPlayerCharacter* P = Cast<AGTCPlayerCharacter>(GetPawn()))
                P->CycleAppearanceSlot(Slot, Delta);
        };
        Svc.Label = [this](int32 Slot) -> FText
        {
            AGTCPlayerCharacter* P = Cast<AGTCPlayerCharacter>(GetPawn());
            return P ? P->GetAppearanceSlotText(Slot) : FText::GetEmpty();
        };
        Svc.Randomize = [this]()
        {
            if (AGTCPlayerCharacter* P = Cast<AGTCPlayerCharacter>(GetPawn()))
                P->RandomizeAppearance();
        };
        Svc.Confirm = [this]() { CloseCharacterCreator(); };

        CharacterCreator = SNew(SGTCCharacterCreator).Services(Svc);
    }

    if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
    {
        VPC->AddViewportWidgetContent(CharacterCreator.ToSharedRef(), 6000);
    }

    bCreatorOpen = true;
    UGameplayStatics::SetGamePaused(this, true);

    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(CharacterCreator);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    SetShowMouseCursor(true);
}

void AGTCPlayerController::CloseCharacterCreator()
{
    if (!bCreatorOpen)
    {
        return;
    }

    // Persist the chosen look so it carries across maps.
    if (AGTCPlayerCharacter* P = Cast<AGTCPlayerCharacter>(GetPawn()))
    {
        P->CommitLook();
    }

    if (CharacterCreator.IsValid())
    {
        if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
        {
            VPC->RemoveViewportWidgetContent(CharacterCreator.ToSharedRef());
        }
    }

    bCreatorOpen = false;
    UGameplayStatics::SetGamePaused(this, false);

    EnterGameInputMode();
}

// ============================================================================
//  Emote panel — one key (B) opens a picker for ALL emotes
// ============================================================================

void AGTCPlayerController::ToggleEmoteWheel()
{
    if (bEmoteWheelOpen) CloseEmoteWheel();
    else OpenEmoteWheel();
}

void AGTCPlayerController::OpenEmoteWheel()
{
    if (bEmoteWheelOpen)
    {
        return;
    }

    // Rebuild the service bridge from the live pawn each open (re-resolves the pawn so
    // it survives a respawn, mirroring the creator/phone services).
    FGTCEmoteServices Services;
    Services.Names = AGTCPlayerCharacter::GetEmoteNames();
    Services.Play = [this](int32 Index)
    {
        if (AGTCPlayerCharacter* P = Cast<AGTCPlayerCharacter>(GetPawn()))
        {
            P->PlayEmote(Index);
        }
    };
    Services.Close = [this]() { CloseEmoteWheel(); };

    EmoteWheel = SNew(SGTCEmoteWheel).Services(Services);

    if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
    {
        VPC->AddViewportWidgetContent(EmoteWheel.ToSharedRef(), 5500);
    }

    bEmoteWheelOpen = true;

    // Do NOT pause — the chosen emote plays on the live pawn in real time. Show the
    // cursor + GameAndUI so a click or a number key lands on the panel.
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(EmoteWheel);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    SetShowMouseCursor(true);
}

void AGTCPlayerController::CloseEmoteWheel()
{
    if (!bEmoteWheelOpen)
    {
        return;
    }

    if (EmoteWheel.IsValid())
    {
        if (UGameViewportClient* VPC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
        {
            VPC->RemoveViewportWidgetContent(EmoteWheel.ToSharedRef());
        }
    }

    bEmoteWheelOpen = false;
    EnterGameInputMode();
}
