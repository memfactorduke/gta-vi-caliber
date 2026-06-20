// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCThrowable.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Throwable.h"
#include "../Ballistics/ExplosionModel.h"
#include "../../World/Hazards/GTCFire.h"

AGTCThrowable::AGTCThrowable()
{
    PrimaryActorTick.bCanEverTick = true;

    // Sphere collision is the root + the bounce surface (blocks the world so the
    // projectile movement can ricochet off it).
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(8.0f);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    SetRootComponent(Collision);

    // Cosmetic body (engine sphere scaled to a grenade); never blocks anything.
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetRelativeScale3D(FVector(0.16f));
    if (UStaticMesh* SphereMesh =
            LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
    {
        Mesh->SetStaticMesh(SphereMesh);
    }

    // Arc + bounce physics. Velocity is set in Throw(); gravity pulls it down.
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->bShouldBounce = true;
    Movement->Bounciness = 0.4f;
    Movement->Friction = 0.4f;
    Movement->ProjectileGravityScale = 1.0f;
    Movement->bAutoActivate = false; // activates on Throw
}

void AGTCThrowable::Throw(const FVector& AimDir, double ThrowSpeed, double InFuseRemaining)
{
    const FVector LaunchVel = FThrowable::LaunchVelocity(AimDir, ThrowSpeed);
    if (Movement != nullptr)
    {
        Movement->Velocity = LaunchVel;
        Movement->Activate(true);
    }
    FuseRemaining = InFuseRemaining;
    bArmed = true;
}

void AGTCThrowable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bArmed || bDetonated || DeltaSeconds <= 0.0f)
    {
        return;
    }
    FuseRemaining -= DeltaSeconds;
    if (FuseRemaining <= 0.0)
    {
        Detonate();
    }
}

void AGTCThrowable::Detonate()
{
    if (bDetonated)
    {
        return;
    }
    bDetonated = true;

    UWorld* World = GetWorld();
    const FVector Center = GetActorLocation();
    if (World != nullptr)
    {
        // Gather everything that can take a hit within the blast sphere.
        TArray<FOverlapResult> Overlaps;
        const FCollisionShape Sphere = FCollisionShape::MakeSphere(static_cast<float>(BlastRadiusCm));
        World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn, Sphere);

        AController* InstigatorController = GetInstigatorController();
        TSet<AActor*> Hit; // one blow per actor, even with multiple overlapping shapes
        for (const FOverlapResult& O : Overlaps)
        {
            AActor* Actor = O.GetActor();
            if (Actor == nullptr || Actor == this || Hit.Contains(Actor))
            {
                continue;
            }
            Hit.Add(Actor);

            const FVector ActorLoc = Actor->GetActorLocation();
            const double Dist = FVector::Dist(Center, ActorLoc);

            // Damage: frame-agnostic radial falloff from the tested core.
            const double Damage = FExplosionModel::DamageAt(Center, ActorLoc, MaxDamage, BlastRadiusCm);
            if (Damage > 0.0)
            {
                const FVector ShotDir = (ActorLoc - Center).GetSafeNormal();
                UGameplayStatics::ApplyPointDamage(
                    Actor, static_cast<float>(Damage), ShotDir, FHitResult(),
                    InstigatorController, this, nullptr);
            }

            // Knockback applied in the ENGINE frame: outward in the ground plane,
            // lifted on +Z (FExplosionModel biases up along Godot +Y, so we use its
            // tested Falloff + UpwardBias here rather than its Y-up vector).
            FVector Outward = ActorLoc - Center;
            Outward.Z = 0.0;
            Outward = Outward.GetSafeNormal();
            const double Mag = MaxImpulse * FExplosionModel::Falloff(Dist, BlastRadiusCm);
            if (Mag > 0.0)
            {
                FVector Impulse = Outward * Mag;
                Impulse.Z += Mag * FExplosionModel::UpwardBias;
                if (ACharacter* Char = Cast<ACharacter>(Actor))
                {
                    Char->LaunchCharacter(Impulse, true, true);
                }
                else if (UPrimitiveComponent* Prim =
                             Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
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

    // Molotov: leave a spreading fire at the impact point (the burning aftermath).
    if (bIncendiary && World != nullptr)
    {
        AGTCFire::SpawnFire(
            World, Center, /*StartIntensity*/ 0.85, /*FuelLoad*/ 4.0, GetInstigatorController(),
            /*Generation*/ 0, FireClass);
    }

    OnDetonate(Center, static_cast<float>(BlastRadiusCm));
    Destroy();
}

AGTCThrowable* AGTCThrowable::SpawnAndThrow(
    UWorld* World, const FVector& Origin, const FVector& AimDir, AActor* InInstigator,
    double ThrowSpeed, double FuseSeconds, double CookSeconds, TSubclassOf<AGTCThrowable> Class,
    bool bIncendiary)
{
    if (World == nullptr || !Class)
    {
        return nullptr;
    }

    const FTransform Xform(AimDir.Rotation(), Origin);
    AGTCThrowable* Grenade = World->SpawnActorDeferred<AGTCThrowable>(
        Class, Xform, InInstigator, Cast<APawn>(InInstigator),
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (Grenade == nullptr)
    {
        return nullptr;
    }
    Grenade->bIncendiary = bIncendiary;
    Grenade->FinishSpawning(Xform);

    // Cooking shortens the fuse; an over-cooked grenade goes off effectively now.
    const double Fuse = FThrowable::FuseAfterCook(FuseSeconds, CookSeconds);
    Grenade->Throw(AimDir, ThrowSpeed, Fuse);
    return Grenade;
}
