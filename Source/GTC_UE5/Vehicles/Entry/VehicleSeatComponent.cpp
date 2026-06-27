// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleSeatComponent.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"

UVehicleSeatComponent::UVehicleSeatComponent()
{
    // Tick so the active transition advances; we early-out when not in one, so the
    // per-frame cost while driving / on foot is a single bool check.
    PrimaryComponentTick.bCanEverTick = true;
}

void UVehicleSeatComponent::BuildWorldSeats(const TArray<FVehicleSeatDef>& Defs, const FTransform& CarXform,
    TArray<FVehicleEntry::FSeat>& Out)
{
    Out.Reset();
    Out.Reserve(Defs.Num());
    for (const FVehicleSeatDef& Def : Defs)
    {
        FVehicleEntry::FSeat Seat;
        // Anchor is a position -> full transform (rotation + translation + scale).
        Seat.Anchor = CarXform.TransformPosition(Def.AnchorOffset);
        // Exit offset is a direction -> rotate only, so "step to the door's side"
        // stays on that side as the car yaws.
        Seat.ExitOffset = CarXform.GetRotation().RotateVector(Def.ExitOffset);
        Seat.bOccupied = false;
        Out.Add(Seat);
    }
}

FVehicleEntry::FParams UVehicleSeatComponent::Params() const
{
    FVehicleEntry::FParams P;
    P.Reach = ReachCm;
    P.EnterSeconds = EnterSeconds;
    P.ExitSeconds = ExitSeconds;
    return P;
}

int32 UVehicleSeatComponent::DriverSeatIndex() const
{
    for (int32 Index = 0; Index < Seats.Num(); ++Index)
    {
        if (Seats[Index].bIsDriver)
        {
            return Index;
        }
    }
    return Seats.Num() > 0 ? 0 : FVehicleEntry::None;
}

bool UVehicleSeatComponent::CanEnter(const AActor* Instigator) const
{
    if (Entry.State() != FVehicleEntry::EState::OnFoot || Instigator == nullptr || GetOwner() == nullptr)
    {
        return false;
    }

    TArray<FVehicleEntry::FSeat> WorldSeats;
    BuildWorldSeats(Seats, GetOwner()->GetActorTransform(), WorldSeats);
    return FVehicleEntry::NearestAvailableSeat(WorldSeats, Instigator->GetActorLocation(), ReachCm)
        != FVehicleEntry::None;
}

FText UVehicleSeatComponent::GetEnterPrompt() const
{
    return NSLOCTEXT("GTCVehicle", "EnterVehiclePrompt", "Enter Vehicle");
}

bool UVehicleSeatComponent::HandleEnter(AActor* Instigator)
{
    if (Entry.State() != FVehicleEntry::EState::OnFoot || GetOwner() == nullptr)
    {
        return false;
    }

    APawn* PlayerPawn = Cast<APawn>(Instigator);
    if (PlayerPawn == nullptr)
    {
        return false;
    }
    APlayerController* PC = Cast<APlayerController>(PlayerPawn->GetController());
    if (PC == nullptr)
    {
        return false;
    }

    TArray<FVehicleEntry::FSeat> WorldSeats;
    BuildWorldSeats(Seats, GetOwner()->GetActorTransform(), WorldSeats);
    const int32 SeatIndex =
        FVehicleEntry::NearestAvailableSeat(WorldSeats, PlayerPawn->GetActorLocation(), ReachCm);
    if (SeatIndex == FVehicleEntry::None)
    {
        return false;
    }

    if (!Entry.BeginEnter(SeatIndex))
    {
        return false;
    }

    // Remember the actors; the possession fires on the Seated edge in Tick.
    StoredPlayerPawn = PlayerPawn;
    StoredController = PC;
    EnteredSeatIndex = SeatIndex;
    return true;
}

bool UVehicleSeatComponent::BeginExit()
{
    // Guarded by the core (only valid while Seated); Tick handles the OnFoot edge.
    return Entry.BeginExit();
}

float UVehicleSeatComponent::GetDoorOpenAlpha() const
{
    return static_cast<float>(Entry.TransitionAlpha(Params()));
}

void UVehicleSeatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!Entry.InTransition())
    {
        return;
    }

    const FVehicleEntry::EState Before = Entry.State();
    const bool bCompleted = Entry.Update(DeltaTime, Params());
    if (!bCompleted)
    {
        return;
    }

    if (Before == FVehicleEntry::EState::Entering)
    {
        OnSeated();
    }
    else // was Exiting
    {
        OnExited();
    }
}

void UVehicleSeatComponent::OnSeated()
{
    APawn* Car = Cast<APawn>(GetOwner());
    if (StoredController == nullptr || StoredPlayerPawn == nullptr || Car == nullptr)
    {
        return;
    }

    // 1) The controller drives the car now. This also un-possesses the player pawn
    //    (which we keep, attached, as the visible driver).
    StoredController->Possess(Car);

    // 2) Put the (still-visible) player into the driver seat; kill its physics so it
    //    rides the car instead of fighting it.
    if (USkeletalMeshComponent* CarMesh = Car->FindComponentByClass<USkeletalMeshComponent>())
    {
        StoredPlayerPawn->AttachToComponent(
            CarMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, DriverSeatSocket);
    }
    StoredPlayerPawn->SetActorEnableCollision(false);

    if (ACharacter* PlayerChar = Cast<ACharacter>(StoredPlayerPawn))
    {
        if (UCharacterMovementComponent* Move = PlayerChar->GetCharacterMovement())
        {
            Move->DisableMovement();
        }
        // Hold a seated pose by overriding the locomotion ABP with a single looping
        // node. A missing SitPose is a silent no-op (player rides in its default pose).
        if (USkeletalMeshComponent* Body = PlayerChar->GetMesh())
        {
            if (UAnimSequence* Pose = SitPose.LoadSynchronous())
            {
                Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                Body->SetAnimation(Pose);
                Body->Play(true);
            }
            // Hide the legs while seated: the driver rides in the footwell, out of
            // view through the glass, so we show torso-up only and sidestep the
            // leg-pose / clipping issues entirely. Restored in OnExited(). Hiding
            // thigh_* collapses each whole leg (calf/foot/ball/toes are children).
            Body->HideBoneByName(TEXT("thigh_l"), EPhysBodyOp::PBO_None);
            Body->HideBoneByName(TEXT("thigh_r"), EPhysBodyOp::PBO_None);
        }
    }

    // 3) Swap input to the driving context (high priority; idempotent re-add is safe).
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(StoredController->GetLocalPlayer()))
    {
        if (UInputMappingContext* Imc = DrivingContext.LoadSynchronous())
        {
            Subsystem->AddMappingContext(Imc, DrivingContextPriority);
        }
    }

    // 4) Blend the view to the car (it carries its own orbit cam).
    StoredController->SetViewTargetWithBlend(Car, ViewBlendSeconds);
}

void UVehicleSeatComponent::OnExited()
{
    AActor* Car = GetOwner();

    // Drop the player beside the DRIVER door, using the car's CURRENT transform (it
    // may have driven far from where they got in).
    FVector ExitPos = (Car != nullptr) ? Car->GetActorLocation() : FVector::ZeroVector;
    if (Car != nullptr)
    {
        TArray<FVehicleEntry::FSeat> WorldSeats;
        BuildWorldSeats(Seats, Car->GetActorTransform(), WorldSeats);
        const int32 DriverIdx = DriverSeatIndex();
        if (WorldSeats.IsValidIndex(DriverIdx))
        {
            ExitPos = FVehicleEntry::ExitStandPosition(WorldSeats[DriverIdx]);
        }
    }

    if (StoredPlayerPawn != nullptr)
    {
        StoredPlayerPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        StoredPlayerPawn->SetActorLocation(ExitPos);
        StoredPlayerPawn->SetActorEnableCollision(true);

        if (ACharacter* PlayerChar = Cast<ACharacter>(StoredPlayerPawn))
        {
            if (UCharacterMovementComponent* Move = PlayerChar->GetCharacterMovement())
            {
                Move->SetMovementMode(MOVE_Walking);
            }
            if (USkeletalMeshComponent* Body = PlayerChar->GetMesh())
            {
                // Show the legs again for on-foot locomotion (mirror of OnSeated).
                Body->UnHideBoneByName(TEXT("thigh_l"));
                Body->UnHideBoneByName(TEXT("thigh_r"));
                // Hand the body back to its locomotion AnimBP.
                Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            }
        }
    }

    if (StoredController != nullptr)
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(StoredController->GetLocalPlayer()))
        {
            if (UInputMappingContext* Imc = DrivingContext.LoadSynchronous())
            {
                Subsystem->RemoveMappingContext(Imc);
            }
        }

        if (StoredPlayerPawn != nullptr)
        {
            // Re-possess re-runs the character's SetupPlayerInputComponent, which
            // rebuilds the player IMC — so we only manage the driving context here.
            StoredController->Possess(StoredPlayerPawn);
            StoredController->SetViewTargetWithBlend(StoredPlayerPawn, ViewBlendSeconds);
        }
    }

    StoredPlayerPawn = nullptr;
    StoredController = nullptr;
    EnteredSeatIndex = FVehicleEntry::None;
}
