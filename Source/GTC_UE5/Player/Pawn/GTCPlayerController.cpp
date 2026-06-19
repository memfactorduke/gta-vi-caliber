// Copyright (c) 2026 GTC contributors

#include "GTCPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

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
}
