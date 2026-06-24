// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCExplosive.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

#include "../../Weapons/Ballistics/ExplosionModel.h"
#include "GTCFire.h"

AGTCExplosive::AGTCExplosive()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetCollisionProfileName(TEXT("BlockAll")); // solid + shootable (ECC_Visibility)
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.1f));
    if (UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
    {
        Mesh->SetStaticMesh(Cylinder);
    }
    SetRootComponent(Mesh);
}

float AGTCExplosive::TakeDamage(
    float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bDetonated || DamageAmount <= 0.0f)
    {
        return Applied;
    }
    Health -= static_cast<double>(DamageAmount);
    if (Health <= 0.0)
    {
        Detonate(EventInstigator);
    }
    return Applied;
}

void AGTCExplosive::Detonate(AController* ByInstigator)
{
    if (bDetonated)
    {
        return;
    }
    bDetonated = true;

    UWorld* World = GetWorld();
    const FVector Center = GetActorLocation();

    TArray<AGTCExplosive*> ToChain;
    if (World != nullptr)
    {
        // Damage targets are ALL pawns — query the Pawn channel, because the player's
        // capsule responds only to ECC_Pawn (AI capsules opt into Visibility so the
        // player can shoot them, but the player's does not). Chain neighbours are other
        // explosives, which block Visibility. Two queries so neither set is missed.
        const double QueryR = FMath::Max(BlastRadiusCm, ChainTriggerRadiusCm);
        const FCollisionShape QuerySphere = FCollisionShape::MakeSphere(static_cast<float>(QueryR));

        // Chain pass: neighbouring explosives within range (the bDetonated guard
        // terminates the cascade — no A<->B ping-pong).
        TArray<FOverlapResult> ChainOverlaps;
        World->OverlapMultiByChannel(ChainOverlaps, Center, FQuat::Identity, ECC_Visibility, QuerySphere);
        for (const FOverlapResult& O : ChainOverlaps)
        {
            AGTCExplosive* Other = Cast<AGTCExplosive>(O.GetActor());
            if (Other != nullptr && Other != this && !Other->bDetonated
                && FExplosionModel::ShouldChain(Center, Other->GetActorLocation(), ChainTriggerRadiusCm))
            {
                ToChain.Add(Other);
            }
        }

        // Damage pass: every pawn caught on the Pawn channel.
        TArray<FOverlapResult> Overlaps;
        World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn, QuerySphere);

        TSet<AActor*> Damaged;
        for (const FOverlapResult& O : Overlaps)
        {
            AActor* Actor = O.GetActor();
            if (Actor == nullptr || Actor == this || Damaged.Contains(Actor))
            {
                continue;
            }
            Damaged.Add(Actor);

            const FVector Loc = Actor->GetActorLocation();
            const double Damage = FExplosionModel::DamageAt(Center, Loc, MaxDamage, BlastRadiusCm);
            if (Damage > 0.0)
            {
                const FVector ShotDir = (Loc - Center).GetSafeNormal();
                UGameplayStatics::ApplyPointDamage(
                    Actor, static_cast<float>(Damage), ShotDir, FHitResult(), ByInstigator, this, nullptr);
            }

            // Knockback in the engine frame (outward in the ground plane, lifted +Z).
            FVector Outward = Loc - Center;
            Outward.Z = 0.0;
            Outward = Outward.GetSafeNormal();
            const double Mag = MaxImpulse * FExplosionModel::Falloff(FVector::Dist(Center, Loc), BlastRadiusCm);
            if (Mag > 0.0)
            {
                FVector Impulse = Outward * Mag;
                Impulse.Z += Mag * FExplosionModel::UpwardBias;
                if (ACharacter* Char = Cast<ACharacter>(Actor))
                {
                    Char->LaunchCharacter(Impulse, true, true);
                }
                else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
                {
                    if (Prim->IsSimulatingPhysics())
                    {
                        Prim->AddRadialImpulse(
                            Center, static_cast<float>(BlastRadiusCm), static_cast<float>(MaxImpulse),
                            ERadialImpulseFalloff::RIF_Linear, /*bVelChange=*/true);
                    }
                }
            }
        }

#if ENABLE_DRAW_DEBUG
        if (bDrawDebugBlast)
        {
            DrawDebugSphere(World, Center, static_cast<float>(BlastRadiusCm), 16, FColor::Orange, false, 1.5f);
        }
#endif
    }

    OnExplode(Center, static_cast<float>(BlastRadiusCm));

    // Set off chained explosives after our own blast resolves; the guard above makes
    // a barrel field cascade once and then stop (no A<->B ping-pong).
    for (AGTCExplosive* Other : ToChain)
    {
        if (Other != nullptr && !Other->bDetonated)
        {
            Other->Detonate(ByInstigator);
        }
    }

    // Leave a spreading fire where it went off — the slow burning aftermath.
    if (bLeavesFire && World != nullptr)
    {
        AGTCFire::SpawnFire(World, Center, /*StartIntensity*/ 0.9, /*FuelLoad*/ 5.0, ByInstigator, /*Generation*/ 0, FireClass);
    }

    Destroy();
}
