// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GTCPlayerCharacter.generated.h"

class UPlayerHealthComponent;
class UGTCInteractionComponent;
class UInputAction;
class UInputComponent;
class USpringArmComponent;
class UCameraComponent;
struct FInputActionValue;

/**
 * AGTCPlayerCharacter — the player pawn (ACharacter with CharacterMovementComponent).
 *
 * Owns a UPlayerHealthComponent (HEALTH ONLY — its model is constructed with
 * ArmorMax=0 so the armor pool is neutralized per the W3 armor-ownership
 * resolution). Armor + money live on the StatsComponent owned by
 * AGTCPlayerState.
 *
 * Input: binds IA_Move / IA_Look / IA_Jump via UEnhancedInputComponent in
 * SetupPlayerInputComponent. The input ASSETS are soft-referenced by path
 * (/Game/Input/IA_*) so the editor-closed headless build has no hard editor
 * dependency. The IA_Move / IA_Look Axis2D *composite mapping* on IMC_Default is
 * editor-authored and is finalized when the editor reopens — the C++ handlers
 * are bound now; the composite wiring is DEFERRED to the editor-reopen phase.
 *
 * Damage routing: TakeDamageRouted feeds the single GtcDamage::ApplyToPlayer
 * entry point (stats armor soaks first, overflow hits health-only model).
 */
UCLASS()
class GTC_UE5_API AGTCPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGTCPlayerCharacter();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    /** Health store (health-only; armor neutralized via ArmorMax=0). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player")
    TObjectPtr<UPlayerHealthComponent> HealthComponent;

    /** Context-sensitive interaction (gathers/scores IGTCInteractable targets). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player")
    TObjectPtr<UGTCInteractionComponent> InteractionComponent;

    /** Collision-aware third-person camera boom (controller drives its rotation). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    /** Follow camera at the boom end — gives PIE a framed view (non-black capture). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    /**
     * Route incoming damage through the W3 resolution: stats armor (on the
     * PlayerState) soaks first, the overflow hits this pawn's health-only model.
     * Returns the overflow that reached health.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player")
    float TakeDamageRouted(float Amount);

protected:
    // --- Enhanced Input actions (soft-referenced by path) ----------------------

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> InteractAction;

    // --- Input handlers --------------------------------------------------------

    void HandleMove(const FInputActionValue& Value);
    void HandleLook(const FInputActionValue& Value);
    void HandleJumpStarted(const FInputActionValue& Value);
    void HandleJumpCompleted(const FInputActionValue& Value);
    void HandleInteract(const FInputActionValue& Value);
};
