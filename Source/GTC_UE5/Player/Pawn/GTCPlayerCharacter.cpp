// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "../Health/PlayerHealthComponent.h"
#include "../Health/PlayerHealthModel.h"
#include "../../World/Interaction/InteractionComponent.h"
#include "../Stats/PlayerStats.h"
#include "GTCPlayerState.h"
#include "GtcDamageRouting.h"

AGTCPlayerCharacter::AGTCPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    // Health-only store (its FPlayerHealthModel is built with ArmorMax=0 inside
    // the component; armor lives on the PlayerState's stats component).
    HealthComponent = CreateDefaultSubobject<UPlayerHealthComponent>(TEXT("HealthComponent"));
    InteractionComponent = CreateDefaultSubobject<UGTCInteractionComponent>(TEXT("InteractionComponent"));

    // Third-person camera so PIE renders a framed (non-black) view — the W3
    // per-merge recording standard captures the viewport, so the pawn must own a
    // camera. The boom collision-springs toward the pawn; the controller drives
    // its rotation (bUsePawnControlRotation), and the body orients to movement.
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 80.0f);

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    bUseControllerRotationYaw = false;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bOrientRotationToMovement = true;
    }

    // Soft paths so the editor-closed headless build never hard-links the
    // editor-authored input assets. Resolved when input is set up.
    MoveAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Move.IA_Move")));
    LookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Look.IA_Look")));
    JumpAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Jump.IA_Jump")));
    InteractAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Interact.IA_Interact")));
}

void AGTCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // LoadSynchronous tolerates a missing asset (returns null) in the
        // headless build. The IA_Move/IA_Look Axis2D composite on IMC_Default is
        // editor-authored and finalized at editor reopen; the handlers bind now.
        if (UInputAction* Move = MoveAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Move, ETriggerEvent::Triggered, this, &AGTCPlayerCharacter::HandleMove);
        }
        if (UInputAction* Look = LookAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Look, ETriggerEvent::Triggered, this, &AGTCPlayerCharacter::HandleLook);
        }
        if (UInputAction* Jump = JumpAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Jump, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleJumpStarted);
            EnhancedInput->BindAction(Jump, ETriggerEvent::Completed, this, &AGTCPlayerCharacter::HandleJumpCompleted);
        }
        if (UInputAction* Interact = InteractAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Interact, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleInteract);
        }
    }
}

void AGTCPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    if (Controller == nullptr)
    {
        return;
    }
    // Move relative to the controller yaw (standard third/first-person move).
    const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    AddMovementInput(Forward, Axis.Y);
    AddMovementInput(Right, Axis.X);
}

void AGTCPlayerCharacter::HandleLook(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AGTCPlayerCharacter::HandleJumpStarted(const FInputActionValue& /*Value*/)
{
    Jump();
}

void AGTCPlayerCharacter::HandleJumpCompleted(const FInputActionValue& /*Value*/)
{
    StopJumping();
}

void AGTCPlayerCharacter::HandleInteract(const FInputActionValue& /*Value*/)
{
    // Act on the interaction component's currently selected target (the component
    // refreshes selection each tick via its live overlap/score). No-op when
    // nothing is in reach.
    if (InteractionComponent != nullptr)
    {
        InteractionComponent->TriggerInteract();
    }
}

float AGTCPlayerCharacter::TakeDamageRouted(float Amount)
{
    // The single damage-routing entry point, mirroring GtcDamage::ApplyToPlayer
    // (the tested pure logic) through the component mutators so the
    // server-authoritative broadcasts + replicated mirrors fire:
    //   1. stats armor (the SOLE pool, on the PlayerState) soaks first,
    //   2. the overflow hits this pawn's health-only model (ArmorMax=0 -> no
    //      second soak). Exactly one soak path, no double-soak, no second pool.
    AGTCPlayerState* PS = GetPlayerState<AGTCPlayerState>();
    if (PS == nullptr || PS->StatsComponent == nullptr || HealthComponent == nullptr)
    {
        return Amount;
    }
    const float Overflow = PS->StatsComponent->SoakDamage(Amount);
    HealthComponent->ApplyDamage(Overflow);
    return Overflow;
}
