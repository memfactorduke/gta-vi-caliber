// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCSwatVan.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"

#include "GTCPoliceOfficer.h"
#include "../../World/Cover/GTCBarricade.h"
#include "../../Systems/Wanted/WantedSubsystem.h"
#include "../../World/Surfaces/SurfaceImpact.h"
#include "Engine/GameInstance.h"

AGTCSwatVan::AGTCSwatVan()
{
    PrimaryActorTick.bCanEverTick = true;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Body->SetRelativeScale3D(FVector(4.2f, 2.0f, 2.0f)); // boxy van
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Body->SetStaticMesh(Cube);
    }
    // Rounds into the van throw sparks (metal panel). See World/Surfaces/SurfaceImpact.h.
    Body->ComponentTags.Add(GTCSurfaceTags::SurfaceTag(EGTCSurface::Metal));
    SetRootComponent(Body);
}

void AGTCSwatVan::BeginPlay()
{
    Super::BeginPlay();
    Vitals = FNpcVitals(MaxHealth);
}

float AGTCSwatVan::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

int32 AGTCSwatVan::ReadStars() const
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

APawn* AGTCSwatVan::ResolveTarget() const
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

void AGTCSwatVan::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || DeltaSeconds <= 0.0f)
    {
        return;
    }

    CachedStars = ReadStars();

    APawn* Target = ResolveTarget();
    if (Target == nullptr || bDeployed)
    {
        return; // unloaded: hold position as a (shootable) prop until recalled
    }

    const FVector VanLoc = GetActorLocation();
    const FVector PlayerLoc = Target->GetActorLocation();
    FVector ToPlayer = PlayerLoc - VanLoc;
    ToPlayer.Z = 0.0;
    const double Dist = ToPlayer.Size();

    if (Dist <= StandoffCm)
    {
        DeploySquad(PlayerLoc);
        bDeployed = true;
        return;
    }

    // Kinematic steer + advance toward the player.
    if (!ToPlayer.IsNearlyZero())
    {
        const double DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToPlayer.Y, ToPlayer.X));
        const float NewYaw = FMath::FixedTurn(
            static_cast<float>(GetActorRotation().Yaw), static_cast<float>(DesiredYaw),
            static_cast<float>(TurnRateDeg * DeltaSeconds));
        SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
    }
    SetActorLocation(VanLoc + GetActorForwardVector() * (ApproachSpeed * DeltaSeconds), /*bSweep*/ true);
}

void AGTCSwatVan::DeploySquad(const FVector& PlayerGround)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    TSubclassOf<AGTCPoliceOfficer> SpawnClass = SquadOfficerClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCPoliceOfficer::StaticClass();
    }

    UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

    // Fan the squad out behind the rear doors.
    const FVector Rear = GetActorLocation() - GetActorForwardVector() * 300.0;
    const int32 Count = FMath::Max(1, SquadSize);
    for (int32 i = 0; i < Count; ++i)
    {
        const double Spread = (static_cast<double>(i) - 0.5 * (Count - 1)) * 180.0;
        FVector Point = Rear + GetActorRightVector() * Spread;
        if (Nav != nullptr)
        {
            FNavLocation Projected;
            if (Nav->ProjectPointToNavigation(Point, Projected, FVector(400.0, 400.0, 600.0)))
            {
                Point = Projected.Location;
            }
        }
        Point.Z += 90.0;

        FRotator Rot = FRotator::ZeroRotator;
        FVector ToPlayer = PlayerGround - Point;
        ToPlayer.Z = 0.0;
        if (!ToPlayer.IsNearlyZero())
        {
            Rot = ToPlayer.Rotation();
        }
        const FTransform Xform(Rot, Point);

        AGTCPoliceOfficer* Trooper = World->SpawnActorDeferred<AGTCPoliceOfficer>(
            SpawnClass, Xform, this, nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if (Trooper != nullptr)
        {
            // SWAT-grade; the officer's own stand-down timer retires it when heat clears.
            Trooper->InitializeUnit(EPoliceUnitType::Swat, static_cast<int32>(GetUniqueID()) + i);
            Trooper->FinishSpawning(Xform);
        }
    }

    // Drop destructible barricades just forward of the squad as cover toward the player.
    TSubclassOf<AGTCBarricade> BarCls = BarricadeClass;
    if (!BarCls)
    {
        BarCls = AGTCBarricade::StaticClass();
    }
    FVector ToPlayer = PlayerGround - Rear;
    ToPlayer.Z = 0.0;
    const FVector DirToPlayer = ToPlayer.GetSafeNormal();
    const FVector Right(-DirToPlayer.Y, DirToPlayer.X, 0.0);
    const float BarYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Right.Y, Right.X)));
    const int32 BarCount = FMath::Max(0, BarricadeCount);
    for (int32 b = 0; b < BarCount; ++b)
    {
        const double Lateral = (static_cast<double>(b) - 0.5 * (BarCount - 1)) * 260.0;
        FVector BarPoint = Rear + DirToPlayer * 250.0 + Right * Lateral;
        if (Nav != nullptr)
        {
            FNavLocation Projected;
            if (Nav->ProjectPointToNavigation(BarPoint, Projected, FVector(400.0, 400.0, 600.0)))
            {
                BarPoint = Projected.Location;
            }
        }
        BarPoint.Z += 60.0;
        if (AGTCBarricade* Bar =
                World->SpawnActor<AGTCBarricade>(BarCls, FTransform(FRotator(0.0f, BarYaw, 0.0f), BarPoint)))
        {
            DeployedBarricades.Add(Bar);
        }
    }
}

void AGTCSwatVan::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Take the deployed barricades with the van (recall or wreck) — they're unowned
    // and never self-despawn otherwise.
    for (const TWeakObjectPtr<AGTCBarricade>& Weak : DeployedBarricades)
    {
        if (AGTCBarricade* Bar = Weak.Get())
        {
            Bar->Destroy();
        }
    }
    DeployedBarricades.Reset();
    Super::EndPlay(EndPlayReason);
}

float AGTCSwatVan::TakeDamage(
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

void AGTCSwatVan::EnterDeath(bool bByPlayer)
{
    if (bDead)
    {
        return;
    }
    bDead = true;

    // Wrecking a police vehicle is a crime.
    if (bByPlayer)
    {
        ReportCrimeToWanted(/*bKilled=*/true);
    }
    OnWrecked();
    SetLifeSpan(static_cast<float>(WreckLingerSeconds));
}

void AGTCSwatVan::ReportCrimeToWanted(bool bKilled) const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->ReportCrime(bKilled);
        }
    }
}
