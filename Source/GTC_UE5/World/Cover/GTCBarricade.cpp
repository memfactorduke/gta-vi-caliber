// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCBarricade.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AGTCBarricade::AGTCBarricade()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    // Block pawns AND the visibility channel so it counts as real cover (the AI's
    // cover-search and LOS checks both trace on ECC_Visibility).
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetRelativeScale3D(FVector(1.6f, 0.35f, 1.2f)); // a chest-high barrier
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Mesh->SetStaticMesh(Cube);
    }
    SetRootComponent(Mesh);
}

void AGTCBarricade::BeginPlay()
{
    Super::BeginPlay();
    Vitals = FNpcVitals(MaxHealth);
}

float AGTCBarricade::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

float AGTCBarricade::TakeDamage(
    float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bBroken || DamageAmount <= 0.0f)
    {
        return Applied;
    }
    if (Vitals.Apply(static_cast<double>(DamageAmount)) == ENpcHitResult::Killed)
    {
        EnterBroken();
    }
    return Applied;
}

void AGTCBarricade::EnterBroken()
{
    if (bBroken)
    {
        return;
    }
    bBroken = true;

    // Drop collision so it stops blocking shots, pawns, and LOS — it's no longer cover.
    if (Mesh != nullptr)
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    OnBroken();

    if (RubbleLingerSeconds > 0.0)
    {
        SetLifeSpan(static_cast<float>(RubbleLingerSeconds));
    }
}
