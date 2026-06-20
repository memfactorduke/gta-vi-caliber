// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"

#include "../../Player/Health/PlayerHealthComponent.h"

AGTCPickup::AGTCPickup()
{
    PrimaryActorTick.bCanEverTick = true;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    Trigger->InitSphereRadius(90.0f);
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    SetRootComponent(Trigger);
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGTCPickup::OnTriggerOverlap);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Trigger);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // the trigger does the work
    Mesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Mesh->SetStaticMesh(Cube);
    }
}

void AGTCPickup::OnTriggerOverlap(
    UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/,
    int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
    TryCollect(OtherActor);
}

bool AGTCPickup::TryCollect(AActor* OtherActor)
{
    if (bCollected)
    {
        return false;
    }
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (Pawn == nullptr || !Pawn->IsPlayerControlled())
    {
        return false;
    }
    UPlayerHealthComponent* Health = Pawn->FindComponentByClass<UPlayerHealthComponent>();
    if (Health == nullptr)
    {
        return false;
    }

    // Leave it on the ground when the relevant bar is already full, so it's there
    // when the player actually needs it.
    if (Kind == EGTCPickupKind::Health)
    {
        if (Health->GetHealthFraction() >= 1.0f)
        {
            return false;
        }
        Health->Heal(Amount);
    }
    else
    {
        if (Health->GetArmorFraction() >= 1.0f)
        {
            return false;
        }
        Health->AddArmor(Amount);
    }

    OnCollected(Pawn);
    bCollected = true;
    if (bRespawns)
    {
        SetActive(false);
        RespawnTimer = RespawnSeconds;
    }
    else
    {
        Destroy();
    }
    return true;
}

void AGTCPickup::SetActive(bool bActive)
{
    SetActorHiddenInGame(!bActive);
    if (Trigger != nullptr)
    {
        Trigger->SetGenerateOverlapEvents(bActive);
    }
}

void AGTCPickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    if (bCollected && bRespawns)
    {
        RespawnTimer -= DeltaSeconds;
        if (RespawnTimer <= 0.0f)
        {
            bCollected = false;
            SetActive(true);
        }
        return;
    }

    // Idle spin so the pickup reads as collectable.
    if (Mesh != nullptr)
    {
        SpinAccum += DeltaSeconds * 90.0f;
        Mesh->SetRelativeRotation(FRotator(0.0f, SpinAccum, 0.0f));
    }
}
