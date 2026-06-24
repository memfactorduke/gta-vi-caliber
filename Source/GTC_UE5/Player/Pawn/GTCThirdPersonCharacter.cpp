// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCThirdPersonCharacter.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "../../World/Interaction/Interactable.h" // IGTCInteractable: the C++ shops/doors

AGTCThirdPersonCharacter::AGTCThirdPersonCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    // Canonical Input asset (the redirect in DefaultEngine.ini also maps /Game/Input/... here).
    InteractAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_Interact.IA_Interact")));
}

void AGTCThirdPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Bind IA_Interact in C++ alongside the Blueprint's own IA_Interact event. Enhanced
    // Input fires every delegate bound to the same action, so the BP's BPI_Interction
    // weapon-pickup flow and this handler both run on press — no Blueprint graph edit.
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (UInputAction* Interact = InteractAction.LoadSynchronous())
        {
            EIC->BindAction(Interact, ETriggerEvent::Started, this, &AGTCThirdPersonCharacter::HandleInteract);
        }
    }
}

void AGTCThirdPersonCharacter::HandleInteract(const FInputActionValue& /*Value*/)
{
    DoContextInteract();
}

void AGTCThirdPersonCharacter::DoContextInteract()
{
    // Priority: if we're in a car, get out; else use a nearby shop/door; else get in a car.
    if (bInVehicle)
    {
        ExitVehicle();
        return;
    }
    if (TryInteractNearby())
    {
        return;
    }
    TryEnterNearbyVehicle();
}

bool AGTCThirdPersonCharacter::TryInteractNearby()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return false;
    }

    const FVector Me = GetActorLocation();
    TArray<AActor*> Candidates;
    UGameplayStatics::GetAllActorsWithInterface(World, UGTCInteractable::StaticClass(), Candidates);

    AActor* Best = nullptr;
    float BestDistSq = InteractReach * InteractReach;
    for (AActor* A : Candidates)
    {
        if (A == nullptr)
        {
            continue;
        }
        const float DistSq = static_cast<float>(FVector::DistSquared(A->GetActorLocation(), Me));
        if (DistSq <= BestDistSq && IGTCInteractable::Execute_CanInteract(A, this))
        {
            BestDistSq = DistSq;
            Best = A;
        }
    }

    if (Best != nullptr)
    {
        IGTCInteractable::Execute_Interact(Best, this);
        return true;
    }
    return false;
}

bool AGTCThirdPersonCharacter::TryEnterNearbyVehicle()
{
    if (bInVehicle)
    {
        return false;
    }
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return false;
    }

    const FVector Me = GetActorLocation();
    TArray<AActor*> Cars;
    UGameplayStatics::GetAllActorsWithTag(World, FName(TEXT("Vehicle")), Cars);

    AActor* Best = nullptr;
    float BestDistSq = VehicleEnterReach * VehicleEnterReach;
    for (AActor* Car : Cars)
    {
        if (Car == nullptr)
        {
            continue;
        }
        const float DistSq = static_cast<float>(FVector::DistSquared(Car->GetActorLocation(), Me));
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Car;
        }
    }

    if (Best == nullptr)
    {
        return false;
    }

    CurrentVehicle = Best;
    bInVehicle = true;

    // Recover automatically if the car is destroyed / streamed out while we ride it, so the
    // player can never get stranded hidden + frozen (the only enterable cars are placed
    // actors, but level streaming/teardown can still remove one underneath us).
    Best->OnDestroyed.AddUniqueDynamic(this, &AGTCThirdPersonCharacter::OnSeatedVehicleDestroyed);

    // Get IN (no driving): stop + freeze + hide the body, then ride along with the car so
    // the follow-camera frames it. The car is NOT possessed — driving is a separate, future
    // task; this is the get-in/out milestone (mirrors AGTCCitizen::EnterVehicleAndHide).
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->StopMovementImmediately();
        Move->DisableMovement();
    }
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    AttachToActor(Best, FAttachmentTransformRules::KeepWorldTransform);
    return true;
}

bool AGTCThirdPersonCharacter::ExitVehicle()
{
    if (!bInVehicle)
    {
        return false;
    }

    // Stop watching the car's destruction — we're leaving it.
    if (AActor* Car = CurrentVehicle.Get())
    {
        Car->OnDestroyed.RemoveDynamic(this, &AGTCThirdPersonCharacter::OnSeatedVehicleDestroyed);
    }

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Step out beside the car at the CURRENT ride-along height (read after detach), never a
    // stale boarding altitude — mirrors AGTCCitizen::ArriveHomeFromDrive. Falls back to the
    // pawn's current spot if the car is already gone.
    FVector Out = GetActorLocation();
    if (const AActor* Car = CurrentVehicle.Get())
    {
        Out = Car->GetActorLocation() + Car->GetActorRightVector() * ExitSideOffset;
        Out.Z = GetActorLocation().Z;
    }

    // Re-enable collision BEFORE the swept move so the capsule resolves against an adjacent
    // car/wall instead of embedding (a swept move with collision off does nothing).
    SetActorEnableCollision(true);
    SetActorLocation(Out, /*bSweep*/ true);
    SetActorHiddenInGame(false);
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->SetMovementMode(MOVE_Walking);
    }

    bInVehicle = false;
    CurrentVehicle = nullptr;
    return true;
}

void AGTCThirdPersonCharacter::OnSeatedVehicleDestroyed(AActor* /*DestroyedActor*/)
{
    // The car vanished under us — get out so we don't stay hidden/frozen. ExitVehicle's
    // car-gone fallback drops us at our current (ride-along) location.
    if (bInVehicle)
    {
        ExitVehicle();
    }
}

void AGTCThirdPersonCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // World/PIE teardown or streaming: drop any attachment so nothing dangles into GC.
    if (bInVehicle)
    {
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        bInVehicle = false;
        CurrentVehicle = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
