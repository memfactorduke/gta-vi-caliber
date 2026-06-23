// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCHostile.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

#include "EngineUtils.h"

#include "../Combat/CombatAi.h"
#include "../Combat/CombatCover.h"
#include "../Combat/Suppression.h"
#include "../Police/GTCPoliceOfficer.h"
#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Weapons/Core/WeaponStats.h"
#include "Kismet/GameplayStatics.h"

#include "../../World/Pickups/GTCPickup.h"
#include "../../Weapons/Throwables/GTCThrowable.h"
#include "../../World/Surfaces/SurfaceImpact.h"

namespace
{
    // FCombatAi is the Godot planar frame (ground = XZ, metres); Unreal is XY/Z-up cm.
    constexpr double MetresToCm = 100.0;
    FORCEINLINE FVector ToCorePlanar(const FVector& U) { return FVector(U.X, 0.0, U.Y); }
    FORCEINLINE FVector FromCorePlanar(const FVector& C) { return FVector(C.X, C.Z, 0.0); }
}

AGTCHostile::AGTCHostile()
{
    PrimaryActorTick.bCanEverTick = true;

    Weapon = CreateDefaultSubobject<UGTCWeaponComponent>(TEXT("Weapon"));

    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = true;
        Move->MaxWalkSpeed = static_cast<float>(RunSpeed);
    }
    bUseControllerRotationYaw = false;

    // Living pawn -> "creature" surface: rounds into the body spray blood. See
    // Source/GTC_UE5/World/Surfaces/SurfaceImpact.h.
    Tags.Add(GTCSurfaceTags::CreatureTag());
}

void AGTCHostile::InitializeHostile(int32 Seed, bool bMeleeVariant)
{
    bMelee = bMeleeVariant;
    Rng.Initialize(Seed);
    StrafeSign = Rng.FRand() < 0.5 ? -1.0 : 1.0;
    // Ranged thugs roll a role: plain, molotov-thrower, or long-range marksman.
    if (!bMelee)
    {
        const double Roll = Rng.FRand();
        if (Roll < MolotovChance)
        {
            bThrowsMolotov = true;
        }
        else if (Roll < MolotovChance + SniperChance)
        {
            bSniper = true;
            PreferredRangeM = SniperRangeM;
            FireCooldownSec = SniperFireCooldownSec;
        }
    }
    Vitals = FNpcVitals(MaxHealth);
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = static_cast<float>(RunSpeed);
    }
}

void AGTCHostile::Stun(float Seconds)
{
    StunTimer = FMath::Max(StunTimer, static_cast<double>(Seconds));
}

void AGTCHostile::BeginPlay()
{
    Super::BeginPlay();
    if (Vitals.MaxHealth <= 0.0)
    {
        Vitals = FNpcVitals(MaxHealth);
    }
    if (Weapon != nullptr && !bMelee)
    {
        if (bSniper)
        {
            // A long-range marksman rifle: hits hard, reaches far, tight spread.
            FWeaponStats Marksman = FWeaponStats::Rifle();
            Marksman.Damage = 55.0;
            Marksman.Range = 260.0;
            Marksman.DamageFalloffStart = 140.0;
            Marksman.DamageFalloffEnd = 250.0;
            Marksman.BaseSpread = 0.003;
            Marksman.MagSize = 5;
            Weapon->SetSingleWeapon(Marksman);
        }
        else
        {
            Weapon->SetSingleWeapon(FWeaponStats::Pistol());
        }
        Weapon->bDrawDebugShots = false;
    }
}

APawn* AGTCHostile::PickTarget() const
{
    const UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }
    const FVector Self = GetActorLocation();

    APawn* Best = nullptr;
    double BestDistSq = TNumericLimits<double>::Max();

    // The player is always a candidate, at any range.
    if (const APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Player = PC->GetPawn())
        {
            Best = Player;
            BestDistSq = FVector::DistSquared(Self, Player->GetActorLocation());
        }
    }

    // A nearby cop is fair game too — gangs and the law shoot it out.
    const double RangeCm = TargetSearchRangeM * 100.0;
    const double RangeSq = RangeCm * RangeCm;
    for (TActorIterator<AGTCPoliceOfficer> It(World); It; ++It)
    {
        AGTCPoliceOfficer* Cop = *It;
        if (Cop == nullptr || Cop->IsDead())
        {
            continue;
        }
        const double DistSq = FVector::DistSquared(Self, Cop->GetActorLocation());
        if (DistSq <= RangeSq && DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Cop;
        }
    }
    return Best;
}

int32 AGTCHostile::CountNearbyAllies(double RangeCm) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return 0;
    }
    const FVector Self = GetActorLocation();
    const double RangeSq = RangeCm * RangeCm;
    int32 Count = 0;
    for (TActorIterator<AGTCHostile> It(World); It; ++It)
    {
        const AGTCHostile* Other = *It;
        if (Other == nullptr || Other == this || Other->IsDead())
        {
            continue;
        }
        if (FVector::DistSquared(Self, Other->GetActorLocation()) <= RangeSq)
        {
            ++Count;
        }
    }
    return Count;
}

bool AGTCHostile::HasLineOfSight(const APawn* Target) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr || Target == nullptr)
    {
        return false;
    }
    FVector EyeLoc;
    FRotator EyeRot;
    GetActorEyesViewPoint(EyeLoc, EyeRot);
    const FVector To = Target->GetActorLocation() + FVector(0.0, 0.0, 50.0);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCHostileLOS), /*bTraceComplex*/ false, this);
    FHitResult Hit;
    const bool bBlocked = World->LineTraceSingleByChannel(Hit, EyeLoc, To, ECC_Visibility, Params);
    return !bBlocked || Hit.GetActor() == Target;
}

void AGTCHostile::FaceTarget(const FVector& TargetPos, float DeltaSeconds)
{
    FVector ToTarget = TargetPos - GetActorLocation();
    ToTarget.Z = 0.0;
    if (ToTarget.IsNearlyZero())
    {
        return;
    }
    const FRotator Desired = ToTarget.Rotation();
    const FRotator Smoothed =
        FMath::RInterpTo(GetActorRotation(), FRotator(0.0, Desired.Yaw, 0.0), DeltaSeconds, 8.0f);
    SetActorRotation(Smoothed);
}

bool AGTCHostile::FindCover(const APawn* Target, FVector& OutCover) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr || Target == nullptr)
    {
        return false;
    }
    const FVector SelfPos = GetActorLocation();
    const FVector ThreatChest = Target->GetActorLocation() + FVector(0.0, 0.0, 50.0);

    // Sample a ring; keep spots whose sightline to the threat is geometry-blocked.
    TArray<FCombatCover::FCoverPoint> Candidates;
    static const double Radii[] = {350.0, 700.0};
    constexpr int32 Spokes = 8;
    for (double Radius : Radii)
    {
        for (int32 Spoke = 0; Spoke < Spokes; ++Spoke)
        {
            const double Angle = (UE_DOUBLE_TWO_PI * Spoke) / Spokes;
            const FVector Sample =
                SelfPos + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
            const FVector SampleChest = Sample + FVector(0.0, 0.0, 50.0);

            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCHostileCover), /*bTraceComplex*/ false, this);
            const bool bBlocked =
                World->LineTraceSingleByChannel(Hit, SampleChest, ThreatChest, ECC_Visibility, Params);
            if (bBlocked && Hit.GetActor() != Target)
            {
                FCombatCover::FCoverPoint Candidate;
                Candidate.Pos = ToCorePlanar(Sample);
                Candidate.Normal = ToCorePlanar((ThreatChest - Sample).GetSafeNormal2D());
                Candidates.Add(Candidate);
            }
        }
    }
    if (Candidates.Num() == 0)
    {
        return false;
    }

    const FCombatCover::FCoverPoint Best =
        FCombatCover::BestCover(Candidates, ToCorePlanar(SelfPos), ToCorePlanar(ThreatChest));
    if (!Best.bValid)
    {
        return false;
    }
    OutCover = FromCorePlanar(Best.Pos);
    OutCover.Z = SelfPos.Z;
    return true;
}

void AGTCHostile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || DeltaSeconds <= 0.0f)
    {
        return;
    }

    FireTimer = FMath::Max(0.0, FireTimer - DeltaSeconds);
    Suppression = FSuppression::Decay(Suppression, static_cast<double>(DeltaSeconds));

    // Retarget on a throttle: the nearest threat (player or a nearby cop).
    RetargetTimer -= static_cast<double>(DeltaSeconds);
    if (RetargetTimer <= 0.0 || !CurrentTarget.IsValid())
    {
        CurrentTarget = PickTarget();
        NearbyAllyCount = CountNearbyAllies(2500.0); // ~25 m
        RetargetTimer = 0.5;
    }
    APawn* Target = CurrentTarget.Get();
    if (Target == nullptr)
    {
        if (Weapon != nullptr) { Weapon->StopFire(); }
        return;
    }

    // Flashbanged: hold position and hold fire until the stun wears off.
    if (StunTimer > 0.0)
    {
        StunTimer -= static_cast<double>(DeltaSeconds);
        if (Weapon != nullptr) { Weapon->StopFire(); }
        return;
    }

    const FVector SelfPos = GetActorLocation();
    const FVector TargetPos = Target->GetActorLocation();
    FVector ToTarget = TargetPos - SelfPos;
    ToTarget.Z = 0.0;
    const double DistanceM = ToTarget.Size() / MetresToCm;

    // Melee thug: charge to contact and strike (FMeleeCombat), no gun.
    if (bMelee)
    {
        Fighter.Tick(static_cast<double>(DeltaSeconds));
        MeleeStrikeCooldown = FMath::Max(0.0, MeleeStrikeCooldown - DeltaSeconds);
        const FVector Dir = ToTarget.GetSafeNormal();
        if (!Dir.IsNearlyZero())
        {
            AddMovementInput(Dir, 1.0f); // full-speed charge
        }
        FaceTarget(TargetPos, DeltaSeconds);
        if (DistanceM <= MeleeRangeM && MeleeStrikeCooldown <= 0.0
            && Fighter.CanStrike(EMeleeStrike::Light))
        {
            const double Dmg = Fighter.Strike(EMeleeStrike::Light);
            UGameplayStatics::ApplyPointDamage(
                Target, static_cast<float>(Dmg), Dir, FHitResult(), GetController(), this, nullptr);
            MeleeStrikeCooldown = 0.8;
        }
        return;
    }

    const FVector ToTargetCore = ToCorePlanar(ToTarget.GetSafeNormal());
    const FVector FacingCore = ToCorePlanar(GetActorForwardVector().GetSafeNormal2D());
    const bool bInArc = FCombatAi::InFiringArc(FacingCore, ToTargetCore, FMath::DegreesToRadians(25.0));
    const bool bLos = HasLineOfSight(Target);
    const int32 Ammo = (Weapon != nullptr) ? Weapon->CurrentAmmoInMag() : 0;

    const FVector2D Band = FCombatAi::EngagementBand(PreferredRangeM, /*Hysteresis*/ 2.0);
    // Morale: a thug whose gang has been decimated AND who is taking damage loses its
    // nerve and routs. The reduced aggression nudges the tactical AI, but DecideAction's
    // own Retreat only fires near death, so force the rout explicitly when alone + hurt
    // (real cover still wins if it found some).
    const double EffectiveAggression = (NearbyAllyCount <= 1) ? Aggression * 0.35 : Aggression;
    ECombatAction Action = FCombatAi::DecideAction(
        DistanceM, Band, bLos, bInArc,
        FSuppression::EffectiveHealthFrac(Vitals.Fraction(), Suppression), EffectiveAggression, Ammo);
    if (NearbyAllyCount <= 1 && Vitals.Fraction() < 0.6 && Action != ECombatAction::TakeCover)
    {
        Action = ECombatAction::Retreat;
    }

    // Movement — TakeCover heads to a real LOS-blocked spot, not just a backpedal.
    FVector MoveDir;
    CoverSearchTimer = FMath::Max(0.0, CoverSearchTimer - DeltaSeconds);
    if (Action == ECombatAction::TakeCover)
    {
        if (!bHasCover || bLos || CoverSearchTimer <= 0.0)
        {
            FVector Found;
            bHasCover = FindCover(Target, Found);
            if (bHasCover)
            {
                CoverPos = Found;
            }
            CoverSearchTimer = 0.6;
        }
        if (bHasCover)
        {
            FVector ToCover = CoverPos - SelfPos;
            ToCover.Z = 0.0;
            MoveDir = ToCover.GetSafeNormal();
        }
        else
        {
            MoveDir = FromCorePlanar(
                FCombatAi::DesiredMove(Action, ToCorePlanar(SelfPos), ToCorePlanar(TargetPos), StrafeSign));
        }
    }
    else
    {
        bHasCover = false;
        MoveDir = FromCorePlanar(
            FCombatAi::DesiredMove(Action, ToCorePlanar(SelfPos), ToCorePlanar(TargetPos), StrafeSign));
    }
    const double Speed = FCombatAi::MoveSpeed(Action, RunSpeed);
    const double Scale = RunSpeed > 0.0 ? Speed / RunSpeed : 0.0;
    if (Scale > 0.0 && !MoveDir.IsNearlyZero())
    {
        AddMovementInput(MoveDir, static_cast<float>(Scale));
    }

    FaceTarget(TargetPos, DeltaSeconds);

    // Fire.
    if (Weapon != nullptr)
    {
        FVector EyeLoc;
        FRotator EyeRot;
        GetActorEyesViewPoint(EyeLoc, EyeRot);
        const FVector AimDir = (TargetPos + FVector(0.0, 0.0, 50.0) - EyeLoc).GetSafeNormal();
        Weapon->SetAimOverride(AimDir);
        if (FCombatAi::ShouldFire(Action, FireTimer <= 0.0) && bLos)
        {
            Weapon->StartFire();
            Weapon->StopFire();
            FireTimer = FireCooldownSec * FSuppression::FireRateMult(Suppression);
        }
        else
        {
            Weapon->StopFire();
        }
    }

    // A thrower lobs a molotov at mid-range to set the player's cover alight.
    if (bThrowsMolotov)
    {
        MolotovCooldown = FMath::Max(0.0, MolotovCooldown - DeltaSeconds);
        if (bLos && MolotovCooldown <= 0.0 && DistanceM >= 8.0 && DistanceM <= 24.0)
        {
            if (UWorld* World = GetWorld())
            {
                const FVector Origin = GetActorLocation() + GetActorForwardVector() * 50.0 + FVector(0.0, 0.0, 60.0);
                FVector Lob = TargetPos - Origin;
                Lob.Z = 0.0;
                Lob = Lob.GetSafeNormal();
                Lob.Z = 0.5; // arc it up so it lobs
                Lob = Lob.GetSafeNormal();

                TSubclassOf<AGTCThrowable> Cls = MolotovClass;
                if (!Cls)
                {
                    Cls = AGTCThrowable::StaticClass();
                }
                AGTCThrowable::SpawnAndThrow(
                    World, Origin, Lob, this, /*ThrowSpeed*/ 1400.0, /*FuseSeconds*/ 2.0,
                    /*CookSeconds*/ 0.0, Cls, /*bIncendiary*/ true);
                MolotovCooldown = MolotovIntervalSec;
            }
        }
    }
}

float AGTCHostile::TakeDamage(
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

    const double Sev = FNpcVitals::WoundSeverity(static_cast<double>(DamageAmount), Vitals.MaxHealth);
    Suppression = FSuppression::Add(Suppression, FSuppression::FromHit(Sev));
    if (Vitals.Apply(static_cast<double>(DamageAmount)) == ENpcHitResult::Killed)
    {
        EnterDeath(Travel);
    }
    else
    {
        OnGunshotWound(static_cast<float>(Sev), -Travel.GetSafeNormal());
    }
    return Applied;
}

void AGTCHostile::EnterDeath(const FVector& BulletTravel)
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
        const FVector Shove = BulletTravel.GetSafeNormal();
        if (!Shove.IsNearlyZero())
        {
            Body->AddImpulse(Shove * 600.0f, NAME_None, /*bVelChange*/ true);
        }
    }

    // Loot: a downed thug may leave a health pickup behind.
    if (UWorld* World = GetWorld())
    {
        if (Rng.FRand() < LootDropChance)
        {
            TSubclassOf<AGTCPickup> LootClass = LootPickupClass;
            if (!LootClass)
            {
                LootClass = AGTCPickup::StaticClass();
            }
            FVector DropAt = GetActorLocation();
            DropAt.Z += 40.0;
            const FTransform Xform(FRotator::ZeroRotator, DropAt);
            if (AGTCPickup* Loot = World->SpawnActorDeferred<AGTCPickup>(
                    LootClass, Xform, nullptr, nullptr,
                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
            {
                Loot->Kind = EGTCPickupKind::Health;
                Loot->Amount = 25.0f;
                Loot->bRespawns = false;
                Loot->FinishSpawning(Xform);
            }
        }
    }

    OnKilled(-BulletTravel.GetSafeNormal());
    SetLifeSpan(static_cast<float>(CorpseLingerSeconds));
}
