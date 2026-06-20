// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPoliceHelicopter.h"

#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "NavigationSystem.h"

#include "GTCPoliceOfficer.h"
#include "../Pursuit/HelicopterPursuit.h"
#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Weapons/Core/WeaponStats.h"
#include "../../Systems/Wanted/WantedSubsystem.h"

namespace
{
    // FHelicopterPursuit is the Godot frame (ground = XZ, up = +Y); Unreal is Z-up
    // (ground = XY, up = +Z). The orbit position swaps Y<->Z at the boundary.
    FORCEINLINE FVector ToHeliFrame(const FVector& U) { return FVector(U.X, U.Z, U.Y); }
    FORCEINLINE FVector FromHeliFrame(const FVector& G) { return FVector(G.X, G.Z, G.Y); }
}

AGTCPoliceHelicopter::AGTCPoliceHelicopter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Hull mesh is the root + the shootable surface (the player can bring it down).
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Body->SetRelativeScale3D(FVector(3.0f, 1.4f, 1.0f));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Body->SetStaticMesh(Cube);
    }
    SetRootComponent(Body);

    // Downward searchlight: sweeps the ground beneath the orbit.
    Searchlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Searchlight"));
    Searchlight->SetupAttachment(Body);
    Searchlight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // straight down
    Searchlight->SetOuterConeAngle(static_cast<float>(FHelicopterPursuit::DefaultConeDegrees));
    Searchlight->SetInnerConeAngle(static_cast<float>(FHelicopterPursuit::DefaultConeDegrees) * 0.6f);
    Searchlight->SetIntensity(50000.0f);
    Searchlight->SetAttenuationRadius(8000.0f);

    // Door gunner — the same weapon component the player uses, fired via aim override.
    Weapon = CreateDefaultSubobject<UGTCWeaponComponent>(TEXT("Weapon"));
}

void AGTCPoliceHelicopter::BeginPlay()
{
    Super::BeginPlay();
    Vitals = FNpcVitals(MaxHealth);
    DropsRemaining = MaxDrops;
    DropTimer = DropIntervalSec; // first insertion after the interval, not on arrival
    if (Weapon != nullptr)
    {
        Weapon->SetSingleWeapon(FWeaponStats::Rifle());
        Weapon->bDrawDebugShots = false;
    }
}

float AGTCPoliceHelicopter::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

int32 AGTCPoliceHelicopter::ReadStars() const
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

APawn* AGTCPoliceHelicopter::ResolveTarget() const
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

void AGTCPoliceHelicopter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || DeltaSeconds <= 0.0f)
    {
        return;
    }

    FireTimer = FMath::Max(0.0, FireTimer - DeltaSeconds);
    CachedStars = ReadStars();

    APawn* Target = ResolveTarget();
    if (Target == nullptr)
    {
        if (Weapon != nullptr)
        {
            Weapon->StopFire();
        }
        return;
    }
    const FVector PlayerLoc = Target->GetActorLocation();

    // --- Orbit above the player (FHelicopterPursuit, remapped to Unreal) ----------
    OrbitTime += DeltaSeconds;
    const FVector ChopperHeli = FHelicopterPursuit::OrbitPoint(
        ToHeliFrame(PlayerLoc), OrbitTime, OrbitRadiusCm, AltitudeCm, AngularSpeed);
    const FVector Desired = FromHeliFrame(ChopperHeli);
    SetActorLocation(FMath::VInterpTo(
        GetActorLocation(), Desired, DeltaSeconds, static_cast<float>(MoveInterpSpeed)));

    // --- Searchlight + "is the player lit" ---------------------------------------
    const FVector PlayerGround = PlayerLoc; // player capsule base is close enough to ground
    const bool bLit = AimAndLight(PlayerGround, DeltaSeconds);

    // --- Door gunner: fire down at a lit suspect, paced by the fire timer ---------
    if (Weapon != nullptr)
    {
        const FVector ChestAim = (PlayerLoc + FVector(0, 0, 50.0) - GetActorLocation()).GetSafeNormal();
        Weapon->SetAimOverride(ChestAim);
        if (bLit && FireTimer <= 0.0)
        {
            Weapon->StartFire();
            Weapon->StopFire();
            FireTimer = 0.45; // brisk suppressing fire from the air
        }
        else
        {
            Weapon->StopFire();
        }
    }

    // --- Fast-rope SWAT insertion at high heat -----------------------------------
    DropTimer = FMath::Max(0.0, DropTimer - DeltaSeconds);
    if (DropsRemaining > 0 && CachedStars >= DropMinStars && DropTimer <= 0.0)
    {
        DropTrooper(PlayerGround);
        --DropsRemaining;
        DropTimer = DropIntervalSec;
    }
}

void AGTCPoliceHelicopter::DropTrooper(const FVector& TargetGround)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // Insert near the player, NavMesh-projected so the trooper lands on walkable ground.
    FVector Point = TargetGround;
    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(Point, Projected, FVector(600.0, 600.0, 600.0)))
        {
            Point = Projected.Location;
        }
    }
    Point.Z += 90.0; // capsule half-height above the floor

    FRotator Rot = FRotator::ZeroRotator;
    FVector ToPlayer = TargetGround - Point;
    ToPlayer.Z = 0.0;
    if (!ToPlayer.IsNearlyZero())
    {
        Rot = ToPlayer.Rotation();
    }
    const FTransform Xform(Rot, Point);

    TSubclassOf<AGTCPoliceOfficer> SpawnClass = DropOfficerClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCPoliceOfficer::StaticClass();
    }
    AGTCPoliceOfficer* Trooper = World->SpawnActorDeferred<AGTCPoliceOfficer>(
        SpawnClass, Xform, this, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (Trooper == nullptr)
    {
        return;
    }
    // SWAT-grade; the officer's own stand-down timer retires it when the heat clears
    // (it isn't in the director's pool). Seed varies per drop for a distinct strafe.
    Trooper->InitializeUnit(EPoliceUnitType::Swat, static_cast<int32>(GetUniqueID()) + DropsRemaining);
    Trooper->FinishSpawning(Xform);
}

bool AGTCPoliceHelicopter::AimAndLight(const FVector& TargetGround, float DeltaSeconds)
{
    const FVector BodyLoc = GetActorLocation();

    // The searchlight stays pinned straight down (relative -90 pitch); the ground
    // patch it lights is the chopper's nadir. The player is "lit" when they stand
    // within that footprint — a real sweep condition as the chopper orbits.
    const double Altitude = FMath::Max(BodyLoc.Z - TargetGround.Z, 1.0);
    const double ConeHalf = FHelicopterPursuit::ConeHalfRadians(FHelicopterPursuit::DefaultConeDegrees);
    const double LitRadius = FHelicopterPursuit::SpotlightGroundRadius(Altitude, ConeHalf);

    const FVector Nadir(BodyLoc.X, BodyLoc.Y, TargetGround.Z);
    return FHelicopterPursuit::TargetLit(Nadir, TargetGround, LitRadius);
}

float AGTCPoliceHelicopter::TakeDamage(
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

void AGTCPoliceHelicopter::EnterDeath(bool bByPlayer)
{
    if (bDead)
    {
        return;
    }
    bDead = true;

    // Bringing down a police chopper is a serious crime.
    if (bByPlayer)
    {
        ReportCrimeToWanted(/*bKilled=*/true);
    }
    if (Weapon != nullptr)
    {
        Weapon->StopFire();
    }
    if (Searchlight != nullptr)
    {
        Searchlight->SetVisibility(false);
    }

    OnShotDown();
    SetLifeSpan(static_cast<float>(WreckLingerSeconds));
}

void AGTCPoliceHelicopter::ReportCrimeToWanted(bool bKilled) const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->ReportCrime(bKilled);
        }
    }
}
