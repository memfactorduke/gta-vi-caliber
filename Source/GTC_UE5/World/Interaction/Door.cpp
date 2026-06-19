// Copyright Epic Games, Inc. All Rights Reserved.

#include "Door.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// --- FDoorSwing (pure, headless-tested) -------------------------------------

float FDoorSwing::Advance(float Frac, float DeltaSeconds, bool bOpening, float DurationSeconds)
{
    const float Target = bOpening ? 1.0f : 0.0f;

    // A non-positive duration has no travel time — snap to the target. Likewise a
    // non-positive delta makes no progress, so hold (already-clamped) state.
    if (DurationSeconds <= 0.0f)
    {
        return Target;
    }
    if (DeltaSeconds <= 0.0f)
    {
        return FMath::Clamp(Frac, 0.0f, 1.0f);
    }

    const float Step = DeltaSeconds / DurationSeconds; // full 0..1 sweep over DurationSeconds
    const float Next = bOpening ? Frac + Step : Frac - Step;
    return FMath::Clamp(Next, 0.0f, 1.0f);
}

float FDoorSwing::YawDegrees(float Frac, float OpenAngleDeg)
{
    const float F = FMath::Clamp(Frac, 0.0f, 1.0f);
    const float Eased = F * F * (3.0f - 2.0f * F); // smoothstep
    return Eased * OpenAngleDeg;
}

// --- AGTCDoor (the UE adapter) ----------------------------------------------

AGTCDoor::AGTCDoor()
{
    // Tick only while the leaf is sweeping — StartSweeping() flips it on, Tick
    // turns it back off once the door has settled at its target.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
    Hinge->SetupAttachment(Root);

    DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
    DoorPanel->SetupAttachment(Hinge);
    // Collision on so the interaction component's object-query overlap finds us.
    DoorPanel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Default to the prototyping door leaf so a freshly placed AGTCDoor is visible
    // and door-shaped out of the box; designers can swap the mesh per instance.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DoorMesh(
        TEXT("/Game/GTCaliberAssets/Content/LevelPrototyping/Interactable/Door/Meshes/SM_Door.SM_Door"));
    if (DoorMesh.Succeeded())
    {
        DoorPanel->SetStaticMesh(DoorMesh.Object);
    }
}

void AGTCDoor::BeginPlay()
{
    Super::BeginPlay();

    bLocked = bStartsLocked;
    if (DoorPanel != nullptr)
    {
        DoorPanel->SetRelativeLocation(PanelOffset);
    }
    // Seed the hinge to the current (shut) state so editor-time tweaks to
    // OpenAngleDeg don't leave a stale pose.
    if (Hinge != nullptr)
    {
        Hinge->SetRelativeRotation(FRotator(0.0f, FDoorSwing::YawDegrees(CurrentFrac, OpenAngleDeg), 0.0f));
    }
}

void AGTCDoor::Interact_Implementation(AActor* /*Instigator*/)
{
    bIsOpen = !bIsOpen;
    StartSweeping();
}

bool AGTCDoor::CanInteract_Implementation(const AActor* /*Instigator*/) const
{
    return !bLocked;
}

FText AGTCDoor::GetInteractPrompt_Implementation() const
{
    return bIsOpen
        ? NSLOCTEXT("GTCDoor", "Close", "Press E to close")
        : NSLOCTEXT("GTCDoor", "Open", "Press E to open");
}

void AGTCDoor::StartSweeping()
{
    SetActorTickEnabled(true);
}

void AGTCDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    CurrentFrac = FDoorSwing::Advance(CurrentFrac, DeltaSeconds, bIsOpen, OpenDurationSeconds);

    if (Hinge != nullptr)
    {
        Hinge->SetRelativeRotation(FRotator(0.0f, FDoorSwing::YawDegrees(CurrentFrac, OpenAngleDeg), 0.0f));
    }

    // Settled at the target (fully open or fully shut) — stop ticking until the
    // next Interact() to keep idle doors free.
    const float Target = bIsOpen ? 1.0f : 0.0f;
    if (FMath::IsNearlyEqual(CurrentFrac, Target))
    {
        SetActorTickEnabled(false);
    }
}
