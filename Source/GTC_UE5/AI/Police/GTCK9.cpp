// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCK9.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AGTCK9::AGTCK9()
{
    PrimaryActorTick.bCanEverTick = true;

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->InitCapsuleSize(34.0f, 40.0f); // low, dog-sized
    }
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = true;
        Move->MaxWalkSpeed = static_cast<float>(RunSpeed);
        Move->MaxAcceleration = 2200.0f; // snappy, darting
    }
    bUseControllerRotationYaw = false;
}

void AGTCK9::InitializeK9(int32 /*Seed*/)
{
    Vitals = FNpcVitals(MaxHealth);
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = static_cast<float>(RunSpeed);
    }
}

APawn* AGTCK9::ResolveTarget() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }
    return nullptr;
}

void AGTCK9::FaceTarget(const FVector& TargetPos, float DeltaSeconds)
{
    FVector ToTarget = TargetPos - GetActorLocation();
    ToTarget.Z = 0.0;
    if (ToTarget.IsNearlyZero())
    {
        return;
    }
    const FRotator Desired(0.0, ToTarget.Rotation().Yaw, 0.0);
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Desired, DeltaSeconds, 10.0f));
}

void AGTCK9::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || DeltaSeconds <= 0.0f)
    {
        return;
    }

    BiteTimer = FMath::Max(0.0, BiteTimer - DeltaSeconds);

    APawn* Target = ResolveTarget();
    if (Target == nullptr)
    {
        return;
    }

    const FVector SelfPos = GetActorLocation();
    const FVector TargetPos = Target->GetActorLocation();
    FVector ToTarget = TargetPos - SelfPos;
    ToTarget.Z = 0.0;
    const double DistM = ToTarget.Size() / 100.0;
    const FVector Dir = ToTarget.GetSafeNormal();

    // Charge the player at full tilt.
    if (!Dir.IsNearlyZero())
    {
        AddMovementInput(Dir, 1.0f);
    }
    FaceTarget(TargetPos, DeltaSeconds);

    // Bite on contact.
    if (DistM <= BiteRangeM && BiteTimer <= 0.0)
    {
        UGameplayStatics::ApplyPointDamage(
            Target, static_cast<float>(BiteDamage), Dir, FHitResult(), GetController(), this, nullptr);
        OnBite(Target);
        BiteTimer = BiteCooldownSec;
    }
}

float AGTCK9::TakeDamage(
    float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bDead || DamageAmount <= 0.0f)
    {
        return Applied;
    }
    FVector Travel = GetActorForwardVector();
    if (DamageCauser != nullptr)
    {
        Travel = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
    }
    if (Vitals.Apply(static_cast<double>(DamageAmount)) == ENpcHitResult::Killed)
    {
        EnterDeath(Travel);
    }
    return Applied;
}

void AGTCK9::EnterDeath(const FVector& Travel)
{
    if (bDead)
    {
        return;
    }
    bDead = true;

    if (AController* C = GetController())
    {
        C->StopMovement();
    }
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->StopMovementImmediately();
        Move->DisableMovement();
    }
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (USkeletalMeshComponent* Body = GetMesh())
    {
        Body->SetCollisionProfileName(TEXT("Ragdoll"));
        Body->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        Body->SetAllBodiesSimulatePhysics(true);
        Body->SetSimulatePhysics(true);
        Body->WakeAllRigidBodies();
        const FVector Shove = Travel.GetSafeNormal();
        if (!Shove.IsNearlyZero())
        {
            Body->AddImpulse(Shove * 400.0f, NAME_None, /*bVelChange*/ true);
        }
    }

    OnKilled(-Travel.GetSafeNormal());
    SetLifeSpan(static_cast<float>(CorpseLingerSeconds));
}
