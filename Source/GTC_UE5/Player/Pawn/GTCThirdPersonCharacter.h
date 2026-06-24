// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GTCThirdPersonCharacter.generated.h"

class UInputAction;
struct FInputActionValue;

/**
 * AGTCThirdPersonCharacter — a thin C++ base for the Blueprint ThirdPerson player
 * (BP_ThirdPersonCharacter reparents to this). It adds the GTC interact behaviours
 * the template pawn lacks, WITHOUT disturbing the existing Blueprint:
 *
 *   - press-E (IA_Interact) to get IN/OUT of a nearby parked car (GET-IN ONLY —
 *     no driving yet; the car is not possessed, the player just rides along), and
 *   - press-E to use a nearby C++ IGTCInteractable (the shop / pay-spray / mod-shop
 *     storefronts), which the Blueprint's own sphere-trace interact can't reach
 *     because those use the C++ interface, not the BP BPI_Interction one.
 *
 * The IA_Interact binding here COEXISTS with the Blueprint's own IA_Interact event
 * (both fire on press), so the BP weapon-pickup flow (BPI_Interction) keeps working
 * untouched. Interact priority on press: exit car > nearby shop/door > enter car.
 *
 * IA_Interact is soft-referenced by path (the canonical submodule Input asset) so the
 * headless build has no hard editor-asset dependency. GTC-prefixed; extends the plain
 * engine ACharacter so reparenting the Epic template Blueprint is loss-free (the BP's
 * mesh, camera boom, movement, weapon vars and event graph all carry over).
 */
UCLASS()
class GTC_UE5_API AGTCThirdPersonCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGTCThirdPersonCharacter();

    /** Reach (cm) to enter a parked car when pressing interact. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle", meta = (ClampMin = "50.0"))
    float VehicleEnterReach = 350.0f;

    /** Reach (cm) to trigger a nearby C++ IGTCInteractable (shop / door). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Interaction", meta = (ClampMin = "50.0"))
    float InteractReach = 250.0f;

    /** Lateral offset (cm) the player is dropped at on exit, beside the car. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Vehicle", meta = (ClampMin = "0.0"))
    float ExitSideOffset = 200.0f;

    /** True while seated in a car (get-in only; no driving). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Vehicle")
    bool bInVehicle = false;

    /** Full interact chain: exit car > nearby shop/door > enter car. Also Blueprint-callable. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Interaction")
    void DoContextInteract();

    /** Enter the nearest "Vehicle"-tagged actor within reach. Returns true if entered. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle")
    bool TryEnterNearbyVehicle();

    /** Leave the current car and drop the player beside it. Returns true if we were in one. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Vehicle")
    bool ExitVehicle();

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    /** Teardown/streaming safety net: never leave the player dangling-attached to a car. */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** IA_Interact, soft-loaded from the canonical submodule Input path. */
    UPROPERTY(EditAnywhere, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> InteractAction;

private:
    void HandleInteract(const FInputActionValue& Value);

    /** Interact with the nearest C++ IGTCInteractable in reach. Returns true if one fired. */
    bool TryInteractNearby();

    /** Auto-exit (unfreeze + unhide) the instant the car we ride is destroyed/streamed out. */
    UFUNCTION()
    void OnSeatedVehicleDestroyed(AActor* DestroyedActor);

    /** The car we are currently seated in (get-in only). */
    UPROPERTY()
    TWeakObjectPtr<AActor> CurrentVehicle;
};
