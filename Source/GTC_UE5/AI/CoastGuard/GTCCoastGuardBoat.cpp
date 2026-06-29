// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCCoastGuardBoat.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

#include "../Pursuit/NavalPursuit.h"
#include "../../Vehicles/Watercraft/GTCBoatComponent.h"
#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Weapons/Core/WeaponStats.h"
#include "../../Systems/Wanted/WantedSubsystem.h"

AGTCCoastGuardBoat::AGTCCoastGuardBoat()
{
    PrimaryActorTick.bCanEverTick = true;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Body->SetRelativeScale3D(FVector(4.0f, 1.6f, 1.0f));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Body->SetStaticMesh(Cube);
    }
    SetRootComponent(Body);

    // Reuse the player's boat locomotion: motion + buoyancy on the same tested cores.
    Boat = CreateDefaultSubobject<UGTCBoatComponent>(TEXT("Boat"));

    // Mounted gun — the same weapon component the player uses, fired via aim override.
    Weapon = CreateDefaultSubobject<UGTCWeaponComponent>(TEXT("Weapon"));
}

void AGTCCoastGuardBoat::BeginPlay()
{
    Super::BeginPlay();
    Vitals = FNpcVitals(MaxHealth);
    if (Weapon != nullptr)
    {
        Weapon->SetSingleWeapon(FWeaponStats::Rifle());
        Weapon->bDrawDebugShots = false;
    }
}

float AGTCCoastGuardBoat::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

int32 AGTCCoastGuardBoat::ReadStars() const
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

APawn* AGTCCoastGuardBoat::ResolveTarget() const
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

void AGTCCoastGuardBoat::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || DeltaSeconds <= 0.0f || Boat == nullptr)
    {
        return;
    }

    FireTimer = FMath::Max(0.0, FireTimer - DeltaSeconds);
    CachedStars = ReadStars();

    APawn* Target = ResolveTarget();
    if (Target == nullptr)
    {
        if (Weapon != nullptr) { Weapon->StopFire(); }
        return;
    }
    const FVector PlayerLoc = Target->GetActorLocation();
    const FVector PlayerVel = Target->GetVelocity();
    const FVector MyLoc = GetActorLocation();

    // --- Steer the boat toward the lead-intercept (FNavalPursuit) -----------------
    const double MySpeed = FMath::Abs(Boat->ForwardSpeed());
    const FVector Aim = FNavalPursuit::InterceptPoint(PlayerLoc, PlayerVel, MyLoc, FMath::Max(MySpeed, 1.0));
    const double DesiredHeading = FMath::Atan2(Aim.Y - MyLoc.Y, Aim.X - MyLoc.X);
    const double HeadingErr = FMath::FindDeltaAngleRadians(Boat->HeadingRad(), DesiredHeading);

    FVehicleControlState C;
    C.YawAxis = static_cast<float>(FMath::Clamp(HeadingErr * SteerGain, -1.0, 1.0));
    C.Throttle = 1.0f; // run it down
    Boat->SetControls(C);
    Boat->Step(DeltaSeconds);

    FVector Loc = MyLoc + Boat->WorldVelocity() * DeltaSeconds;
    Boat->IntegrateVertical(Loc, DeltaSeconds);
    SetActorLocation(Loc, /*bSweep*/ true);
    SetActorRotation(Boat->DesiredAttitude());

    // --- Mounted gun: fire with a clear shot in range ----------------------------
    if (Weapon != nullptr)
    {
        const FVector PlayerChest = PlayerLoc + FVector(0.0, 0.0, 50.0);
        const FVector GunLoc = GetActorLocation();
        Weapon->SetAimOverride((PlayerChest - GunLoc).GetSafeNormal());

        bool bCanFire = FVector::DistSquared(GunLoc, PlayerChest) <= (FireRangeCm * FireRangeCm);
        if (bCanFire)
        {
            if (const UWorld* World = GetWorld())
            {
                FHitResult Hit;
                FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCCoastGuardLOS), /*bTraceComplex*/ false, this);
                const bool bBlocked = World->LineTraceSingleByChannel(Hit, GunLoc, PlayerChest, ECC_Visibility, Params);
                bCanFire = !bBlocked || Hit.GetActor() == Target;
            }
        }
        if (bCanFire && FireTimer <= 0.0)
        {
            Weapon->StartFire();
            Weapon->StopFire();
            FireTimer = 0.5;
        }
        else
        {
            Weapon->StopFire();
        }
    }
}

float AGTCCoastGuardBoat::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
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

void AGTCCoastGuardBoat::EnterDeath(bool bByPlayer)
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
    OnSunk();
    SetLifeSpan(static_cast<float>(WreckLingerSeconds));
}

void AGTCCoastGuardBoat::ReportCrimeToWanted(bool bKilled) const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->ReportCrime(bKilled);
        }
    }
}
