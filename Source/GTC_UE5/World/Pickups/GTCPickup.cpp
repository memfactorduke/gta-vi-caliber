// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"

#include "../../Player/Health/PlayerHealthComponent.h"
#include "../../Player/Stats/PlayerStats.h"
#include "../../Player/Pawn/GTCPlayerCharacter.h"
#include "GameFramework/PlayerState.h"

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

void AGTCPickup::BeginPlay()
{
    Super::BeginPlay();
    // Dropped loot (one-shot, not hand-placed) self-reclaims if never picked up, so a
    // long firefight doesn't litter the world with abandoned pickups.
    if (!bRespawns && !bPersistent && IdleDespawnSeconds > 0.0f)
    {
        SetLifeSpan(IdleDespawnSeconds);
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

    // Leave it on the ground when the relevant bar is already full, so it's there when
    // the player actually needs it. Health lives on the pawn's UPlayerHealthComponent;
    // armor is the SOLE responsibility of the PlayerState's UPlayerStatsComponent (the
    // health component's armor pool is deliberately zeroed), so route each to its owner.
    if (Kind == EGTCPickupKind::Health)
    {
        UPlayerHealthComponent* Health = Pawn->FindComponentByClass<UPlayerHealthComponent>();
        if (Health == nullptr || Health->GetHealthFraction() >= 1.0f)
        {
            return false;
        }
        Health->Heal(Amount);
    }
    else if (Kind == EGTCPickupKind::Armor)
    {
        UPlayerStatsComponent* Stats = nullptr;
        if (APlayerState* PS = Pawn->GetPlayerState())
        {
            Stats = PS->FindComponentByClass<UPlayerStatsComponent>();
        }
        if (Stats == nullptr || Stats->GetArmor() >= Stats->GetMaxArmor())
        {
            return false;
        }
        Stats->AddArmor(Amount);
    }
    else
    {
        // A throwable-ammo pickup: FlashbangAmmo/GrenadeAmmo/MolotovAmmo map 1:1 onto the
        // loadout kind indices (0/1/2). Leave it on the ground when that kind is capped.
        AGTCPlayerCharacter* Player = Cast<AGTCPlayerCharacter>(Pawn);
        if (Player == nullptr)
        {
            return false;
        }
        const int32 KindIndex =
            static_cast<int32>(Kind) - static_cast<int32>(EGTCPickupKind::FlashbangAmmo);
        if (Player->IsThrowableAtCap(KindIndex))
        {
            return false;
        }
        Player->AddThrowableAmmo(KindIndex, FMath::Max(1, FMath::RoundToInt(Amount)));
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
