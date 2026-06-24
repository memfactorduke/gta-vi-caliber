// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTurret.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Weapons/Core/WeaponStats.h"
#include "../../World/Surfaces/SurfaceImpact.h"

AGTCTurret::AGTCTurret()
{
    PrimaryActorTick.bCanEverTick = true;

    Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
    Base->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Base->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.5f));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Base->SetStaticMesh(Cube);
    }
    // Metal emplacement: rounds throw sparks off the base and head.
    Base->ComponentTags.Add(GTCSurfaceTags::SurfaceTag(EGTCSurface::Metal));
    SetRootComponent(Base);

    // Swivel head: holds the barrel, rotates to track the target.
    Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
    Head->SetupAttachment(Base);
    Head->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
    Head->SetRelativeScale3D(FVector(1.2f, 0.4f, 0.4f));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Head->SetStaticMesh(Cube);
    }
    Head->ComponentTags.Add(GTCSurfaceTags::SurfaceTag(EGTCSurface::Metal));

    Weapon = CreateDefaultSubobject<UGTCWeaponComponent>(TEXT("Weapon"));
}

void AGTCTurret::BeginPlay()
{
    Super::BeginPlay();
    Vitals = FNpcVitals(MaxHealth);
    if (Weapon != nullptr)
    {
        Weapon->SetSingleWeapon(FWeaponStats::Rifle());
        Weapon->bDrawDebugShots = false;
    }
}

float AGTCTurret::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

APawn* AGTCTurret::ResolveTarget() const
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

bool AGTCTurret::HasLineOfSight(const FVector& From, const APawn* Target) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr || Target == nullptr)
    {
        return false;
    }
    const FVector To = Target->GetActorLocation() + FVector(0.0, 0.0, 50.0);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCTurretLOS), /*bTraceComplex*/ false, this);
    FHitResult Hit;
    const bool bBlocked = World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
    return !bBlocked || Hit.GetActor() == Target;
}

void AGTCTurret::Stun(float Seconds)
{
    StunTimer = FMath::Max(StunTimer, static_cast<double>(Seconds));
}

void AGTCTurret::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || DeltaSeconds <= 0.0f)
    {
        return;
    }

    FireTimer = FMath::Max(0.0, FireTimer - DeltaSeconds);

    // Flashbanged: the turret's optics are blinded — hold fire until it recovers.
    if (StunTimer > 0.0)
    {
        StunTimer -= static_cast<double>(DeltaSeconds);
        if (Weapon != nullptr) { Weapon->StopFire(); }
        return;
    }

    APawn* Target = ResolveTarget();
    if (Target == nullptr)
    {
        if (Weapon != nullptr) { Weapon->StopFire(); }
        return;
    }

    const FVector HeadLoc = Head != nullptr ? Head->GetComponentLocation() : GetActorLocation();
    const FVector AimAt = Target->GetActorLocation() + FVector(0.0, 0.0, 50.0);
    FVector ToTarget = AimAt - HeadLoc;
    const double Dist = ToTarget.Size();
    const FVector AimDir = ToTarget.GetSafeNormal();

    // Swivel the head toward the target (full 3D aim).
    if (Head != nullptr && !AimDir.IsNearlyZero())
    {
        const FRotator Desired = AimDir.Rotation();
        const FRotator Smoothed = FMath::RInterpConstantTo(
            Head->GetComponentRotation(), Desired, DeltaSeconds, static_cast<float>(TurnRateDeg));
        Head->SetWorldRotation(Smoothed);
    }

    const bool bLos = HasLineOfSight(HeadLoc, Target);
    if (Weapon != nullptr)
    {
        Weapon->SetAimOverride(AimDir);
        if (Dist <= RangeM * 100.0 && bLos && FireTimer <= 0.0)
        {
            Weapon->StartFire();
            Weapon->StopFire();
            FireTimer = FireCooldownSec;
        }
        else
        {
            Weapon->StopFire();
        }
    }
}

float AGTCTurret::TakeDamage(
    float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bDead || DamageAmount <= 0.0f)
    {
        return Applied;
    }
    if (Vitals.Apply(static_cast<double>(DamageAmount)) == ENpcHitResult::Killed)
    {
        EnterDeath();
    }
    return Applied;
}

void AGTCTurret::EnterDeath()
{
    if (bDead)
    {
        return;
    }
    bDead = true;
    if (Weapon != nullptr)
    {
        Weapon->StopFire();
    }
    OnDestroyed();
    SetLifeSpan(static_cast<float>(WreckLingerSeconds));
}
