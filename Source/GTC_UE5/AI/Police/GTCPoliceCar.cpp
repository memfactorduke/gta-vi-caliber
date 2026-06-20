// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPoliceCar.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "../Pursuit/PursuitTactics.h"
#include "../Pursuit/PursuitMemory.h"
#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Weapons/Core/WeaponStats.h"
#include "../../Systems/Wanted/WantedSubsystem.h"

namespace
{
    // FPursuitTactics is the Godot XZ frame, METRES; Unreal is Z-up, cm.
    FORCEINLINE FVector CoreM(const FVector& U) { return FVector(U.X * 0.01, 0.0, U.Y * 0.01); }
    FORCEINLINE FVector FromCoreM(const FVector& C) { return FVector(C.X * 100.0, C.Z * 100.0, 0.0); }
    FORCEINLINE FVector DirToCore(const FVector& U) { return FVector(U.X, 0.0, U.Y); }
}

AGTCPoliceCar::AGTCPoliceCar()
{
    PrimaryActorTick.bCanEverTick = true;

    // Car-sized box is the root + the shootable hull.
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
    Body->SetBoxExtent(FVector(230.0f, 95.0f, 75.0f));
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SetRootComponent(Body);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Body);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetRelativeScale3D(FVector(4.6f, 1.9f, 1.5f));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Mesh->SetStaticMesh(Cube);
    }

    Weapon = CreateDefaultSubobject<UGTCWeaponComponent>(TEXT("Weapon"));
}

void AGTCPoliceCar::BeginPlay()
{
    Super::BeginPlay();
    Vitals = FNpcVitals(MaxHealth);
    if (Weapon != nullptr)
    {
        Weapon->SetSingleWeapon(FWeaponStats::Smg());
        Weapon->bDrawDebugShots = false;
    }
}

float AGTCPoliceCar::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

int32 AGTCPoliceCar::ReadStars() const
{
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            return Wanted->Stars();
        }
    }
    return 0;
}

APawn* AGTCPoliceCar::ResolveTarget() const
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

bool AGTCPoliceCar::HasLineOfSight(const APawn* Target) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr || Target == nullptr)
    {
        return false;
    }
    const FVector From = GetActorLocation() + FVector(0.0, 0.0, 60.0);
    const FVector To = Target->GetActorLocation() + FVector(0.0, 0.0, 50.0);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCCarLOS), /*bTraceComplex*/ false, this);
    FHitResult Hit;
    const bool bBlocked = World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
    return !bBlocked || Hit.GetActor() == Target;
}

void AGTCPoliceCar::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || DeltaSeconds <= 0.0f)
    {
        return;
    }

    FireTimer = FMath::Max(0.0, FireTimer - DeltaSeconds);
    RamCooldown = FMath::Max(0.0, RamCooldown - DeltaSeconds);
    CachedStars = ReadStars();

    APawn* Target = ResolveTarget();
    if (Target == nullptr)
    {
        if (Weapon != nullptr) { Weapon->StopFire(); }
        return;
    }

    const FVector CarLoc = GetActorLocation();
    const FVector TruePlayerLoc = Target->GetActorLocation();
    const FVector Heading = GetActorForwardVector();

    // Pursuit memory: lose track when the sightline breaks — chase the last-known
    // spot (no velocity lead while blind), and don't ram/shoot what you can't see.
    const bool bLos = HasLineOfSight(Target);
    if (bLos)
    {
        LastKnownPos = TruePlayerLoc;
        TimeUnseen = 0.0;
        bHasLastKnown = true;
    }
    else if (bHasLastKnown)
    {
        TimeUnseen += DeltaSeconds;
    }
    const FVector PlayerLoc =
        bHasLastKnown ? FPursuitMemory::Target(bLos, TruePlayerLoc, LastKnownPos) : TruePlayerLoc;
    const FVector PlayerVel = bLos ? Target->GetVelocity() : FVector::ZeroVector;

    FVector ToPlayer = PlayerLoc - CarLoc;
    ToPlayer.Z = 0.0;
    const double DistCm = ToPlayer.Size();

    // --- Pursuit decision (FPursuitTactics, core XZ metres) ----------------------
    const double RamRangeM = RamRangeCm * 0.01;
    const EPursuitTactic Tactic = FPursuitTactics::ChooseTactic(
        CoreM(CarLoc), DirToCore(Heading.GetSafeNormal2D()), CoreM(PlayerLoc), CoreM(PlayerVel),
        CachedStars, RamRangeM);

    if (Tactic == EPursuitTactic::BackOff)
    {
        if (Weapon != nullptr) { Weapon->StopFire(); }
        return; // disengaged; the director recalls cars once the heat is gone
    }

    // Aim point: lead-intercept for the chase/ram/PIT, an offset wall for a block.
    FVector AimCore;
    if (Tactic == EPursuitTactic::Block)
    {
        const double Side = FPursuitTactics::PitSide(CoreM(CarLoc), CoreM(PlayerLoc), CoreM(PlayerVel));
        AimCore = FPursuitTactics::BlockOffset(CoreM(PlayerLoc), CoreM(PlayerVel), Side, /*DistanceM*/ 12.0);
    }
    else
    {
        AimCore = FPursuitTactics::InterceptPoint(
            CoreM(PlayerLoc), CoreM(PlayerVel), CoreM(CarLoc), MaxSpeed * 0.01);
    }
    FVector Aim = FromCoreM(AimCore);
    Aim.Z = CarLoc.Z; // stay on the car's ground height (full physics is the editor pass)

    // --- Steer + drive (kinematic) -----------------------------------------------
    FVector ToAim = Aim - CarLoc;
    ToAim.Z = 0.0;
    if (!ToAim.IsNearlyZero())
    {
        const double DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToAim.Y, ToAim.X));
        const float NewYaw = FMath::FixedTurn(
            static_cast<float>(GetActorRotation().Yaw), static_cast<float>(DesiredYaw),
            static_cast<float>(TurnRateDeg * DeltaSeconds));
        SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
    }
    const double DesiredSpeed =
        FPursuitTactics::DesiredSpeed(DistCm * 0.01, BaseSpeed * 0.01, MaxSpeed * 0.01) * 100.0;
    SetActorLocation(CarLoc + GetActorForwardVector() * (DesiredSpeed * DeltaSeconds), /*bSweep*/ true);

    // --- Ram: slam the player on contact when authorised -------------------------
    if (bLos && DistCm <= RamRangeCm && RamCooldown <= 0.0)
    {
        const FVector ShotDir = ToPlayer.GetSafeNormal();
        UGameplayStatics::ApplyPointDamage(
            Target, static_cast<float>(RamDamage), ShotDir, FHitResult(),
            GetInstigatorController(), this, nullptr);
        if (ACharacter* PlayerChar = Cast<ACharacter>(Target))
        {
            FVector Launch = ShotDir * 60000.0;
            Launch.Z = 30000.0;
            PlayerChar->LaunchCharacter(Launch, true, true);
        }
        RamCooldown = 1.2;
    }

    // --- Gunner fire when reasonably close --------------------------------------
    if (Weapon != nullptr)
    {
        const FVector Aim3D = (PlayerLoc + FVector(0, 0, 50.0) - CarLoc).GetSafeNormal();
        Weapon->SetAimOverride(Aim3D);
        if (bLos && DistCm <= 4000.0 && FireTimer <= 0.0)
        {
            Weapon->StartFire();
            Weapon->StopFire();
            FireTimer = 0.6;
        }
        else
        {
            Weapon->StopFire();
        }
    }
}

float AGTCPoliceCar::TakeDamage(
    float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bDead || DamageAmount <= 0.0f)
    {
        return Applied;
    }

    bool bByPlayer = false;
    if (const UWorld* World = GetWorld())
    {
        bByPlayer = EventInstigator != nullptr && EventInstigator == World->GetFirstPlayerController();
    }

    if (Vitals.Apply(static_cast<double>(DamageAmount)) == ENpcHitResult::Killed)
    {
        EnterDeath(bByPlayer);
    }
    return Applied;
}

void AGTCPoliceCar::EnterDeath(bool bByPlayer)
{
    if (bDead)
    {
        return;
    }
    bDead = true;
    if (bByPlayer)
    {
        ReportCrimeToWanted(/*bKilled=*/true);
    }
    if (Weapon != nullptr)
    {
        Weapon->StopFire();
    }
    OnWrecked();
    SetLifeSpan(static_cast<float>(WreckLingerSeconds));
}

void AGTCPoliceCar::ReportCrimeToWanted(bool bKilled) const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->ReportCrime(bKilled);
        }
    }
}
