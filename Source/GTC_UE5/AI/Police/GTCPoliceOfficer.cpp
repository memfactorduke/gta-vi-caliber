// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPoliceOfficer.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

#include "../Combat/CombatAi.h"
#include "../Combat/CombatCover.h"
#include "../Combat/Suppression.h"
#include "../Combat/PoliceCombat.h"
#include "../Combat/Apprehend.h"
#include "../Pursuit/PursuitMemory.h"
#include "../../NPC/Agent/GTCCrowdSubsystem.h"
#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Weapons/Core/WeaponStats.h"
#include "../../Systems/Wanted/WantedSubsystem.h"
#include "../../World/Pickups/GTCPickup.h"
#include "../../Weapons/Throwables/GTCThrowable.h"
#include "../Hostiles/GTCHostile.h"
#include "../../World/Surfaces/SurfaceImpact.h"
#include "EngineUtils.h"

namespace
{
    // FWeaponStats / FPoliceCombat distances are METRES (the Godot frame); Unreal
    // world units are centimetres.
    constexpr double MetresToCm = 100.0;

    // Frame remap between the engine (Z-up: planar = XY) and the pure-core combat
    // helpers (Godot frame: planar = XZ, Y ignored). A planar Unreal vector's Y
    // rides in the core's Z slot and back.
    FORCEINLINE FVector ToCorePlanar(const FVector& U) { return FVector(U.X, 0.0, U.Y); }
    FORCEINLINE FVector FromCorePlanar(const FVector& C) { return FVector(C.X, C.Z, 0.0); }

    // Per-unit toughness: heavier responders soak more rounds.
    double MaxHealthForUnit(EPoliceUnitType Unit)
    {
        switch (Unit)
        {
            case EPoliceUnitType::Swat: return 180.0;
            case EPoliceUnitType::Military: return 240.0;
            case EPoliceUnitType::Cruiser: return 120.0;
            default: return 100.0; // BeatCop (Helicopter is not embodied on foot)
        }
    }

    // Per-unit sidearm: beat cops carry pistols, heavier units carry rifles.
    FWeaponStats LoadoutForUnit(EPoliceUnitType Unit)
    {
        switch (Unit)
        {
            case EPoliceUnitType::Swat:
            case EPoliceUnitType::Military: return FWeaponStats::Rifle();
            case EPoliceUnitType::Cruiser: return FWeaponStats::Smg();
            default: return FWeaponStats::Pistol();
        }
    }
}

AGTCPoliceOfficer::AGTCPoliceOfficer()
{
    PrimaryActorTick.bCanEverTick = true;

    // A default AI controller possesses the officer so CharacterMovement consumes
    // AddMovementInput (the same reason AGTCCitizen does it).
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    // The officer keeps the player covered: we set its yaw manually each tick to
    // face the target (so aim + firing arc line up), rather than turning to face
    // travel. Movement still follows AddMovementInput, letting it strafe sideways
    // while aiming at the player.
    bUseControllerRotationYaw = false;
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = false;
        Move->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
        Move->MaxWalkSpeed = static_cast<float>(RunSpeed);
        Move->MaxAcceleration = 1200.0f;
        Move->BrakingDecelerationWalking = 1400.0f;
    }

    // Make the officer shootable: the player's hitscan line-traces on ECC_Visibility
    // (UGTCWeaponComponent::TraceChannel), so the capsule + mesh must block it or
    // rounds pass straight through. Mirrors AGTCCitizen's setup.
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
    if (USkeletalMeshComponent* Body = GetMesh())
    {
        Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }

    // The officer's gun: the same component the player uses, fired via an aim
    // override (this owner has no follow camera).
    Weapon = CreateDefaultSubobject<UGTCWeaponComponent>(TEXT("Weapon"));

    // Living pawn -> "creature" surface: rounds into the officer spray blood, not a
    // generic puff. See Source/GTC_UE5/World/Surfaces/SurfaceImpact.h.
    Tags.Add(GTCSurfaceTags::CreatureTag());
}

void AGTCPoliceOfficer::BeginPlay()
{
    Super::BeginPlay();

    if (!bInitialized)
    {
        InitializeUnit(UnitType, GetUniqueID());
    }

    // Override the component's default arsenal with this unit's single sidearm.
    if (Weapon != nullptr)
    {
        Weapon->SetSingleWeapon(LoadoutForUnit(UnitType));
        // Officers fire constantly; the per-shot debug tracers (a single-player dev
        // aid) would flood the screen across a whole squad, so off for AI.
        Weapon->bDrawDebugShots = false;
    }
}

void AGTCPoliceOfficer::InitializeUnit(EPoliceUnitType InUnitType, int32 Seed)
{
    UnitType = InUnitType;
    Rng.Initialize(Seed);
    StrafeSign = Rng.FRand() < 0.5 ? -1.0 : 1.0;
    Vitals = FNpcVitals(MaxHealthForUnit(UnitType));
    bShielded = (UnitType == EPoliceUnitType::Swat) && (Rng.FRand() < ShieldChance);
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = static_cast<float>(RunSpeed);
    }
    bInitialized = true;
}

void AGTCPoliceOfficer::Stun(float Seconds)
{
    StunTimer = FMath::Max(StunTimer, static_cast<double>(Seconds));
}

float AGTCPoliceOfficer::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

void AGTCPoliceOfficer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDead || DeltaSeconds <= 0.0f)
    {
        return;
    }

    FireTimer = FMath::Max(0.0, FireTimer - DeltaSeconds);

    // Refresh the live wanted level (the escalation input) + whether the player is
    // actively committing crimes right now (the "armed/dangerous" signal for apprehend).
    CachedStars = 0;
    bCachedPlayerDangerous = false;
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            CachedStars = Wanted->Stars();
            bCachedPlayerDangerous = Wanted->IsCrimeActive();
        }
    }

    APawn* Target = ResolveTarget();
    if (Target == nullptr || CachedStars <= 0)
    {
        // No target, or the player is no longer wanted: stand down (the director
        // recalls pooled officers). Holster, and after a grace period go home — this
        // also retires chopper-dropped officers the director never pooled.
        if (Weapon != nullptr)
        {
            Weapon->StopFire();
        }
        // No actionable target (player gone OR no longer wanted): after a grace period
        // go home. Accumulate whenever we're standing down — not only at zero stars —
        // so an officer with a momentarily-null player pawn still eventually retires.
        StandDownTimer += DeltaSeconds;
        if (StandDownTimer > 8.0)
        {
            Destroy();
        }
        return;
    }

    // Retire if the player has left this officer far behind. The director distance-
    // recalls its pooled officers, but van/heli-dropped troopers aren't pooled, so this
    // is their only distance cull (without it they linger map-wide at sustained heat).
    constexpr double FarCullCm = 22000.0; // ~220 m
    if (FVector::DistSquaredXY(GetActorLocation(), Target->GetActorLocation()) > FarCullCm * FarCullCm)
    {
        if (Weapon != nullptr)
        {
            Weapon->StopFire();
        }
        StandDownTimer += DeltaSeconds;
        if (StandDownTimer > 8.0)
        {
            Destroy();
        }
        return;
    }
    StandDownTimer = 0.0;

    // Flashbanged: hold position and hold fire until the stun wears off.
    if (StunTimer > 0.0)
    {
        StunTimer -= DeltaSeconds;
        if (Weapon != nullptr)
        {
            Weapon->StopFire();
        }
        return;
    }

    // Pursuit memory: track the last spot we actually saw the player; when the
    // sightline is broken, steer to that last-known point rather than the true
    // position — so breaking line of sight is a real way to shake the chase.
    const bool bLos = HasLineOfSight(Target);
    const FVector TruePlayerPos = Target->GetActorLocation();
    if (bLos)
    {
        LastKnownPos = TruePlayerPos;
        TimeUnseen = 0.0;
        bHasLastKnown = true;
    }
    else if (bHasLastKnown)
    {
        TimeUnseen += DeltaSeconds;
    }
    // Default combat target: pursue the player (live when seen, last-known when not).
    APawn* CombatPawn = Target;
    FVector CombatPos =
        bHasLastKnown ? FPursuitMemory::Target(bLos, TruePlayerPos, LastKnownPos) : TruePlayerPos;
    bool bCombatLos = bLos;
    bool bFightingHostile = false;

    // Self-defense: ONLY when the player is out of sight, turn on a gang member that's
    // right on top of us (so a cop being shot at by gangs shoots back). When the player
    // is visible nothing changes — player pursuit always wins.
    if (!bLos)
    {
        if (AGTCHostile* Foe = FindNearbyHostile(1200.0))
        {
            CombatPawn = Foe;
            CombatPos = Foe->GetActorLocation();
            bCombatLos = HasLineOfSight(Foe);
            bFightingHostile = true;
        }
    }

    FaceTarget(CombatPos, DeltaSeconds);
    DriveCombat(DeltaSeconds, CombatPawn, CombatPos, bCombatLos, bFightingHostile);
}

APawn* AGTCPoliceOfficer::ResolveTarget() const
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

bool AGTCPoliceOfficer::HasLineOfSight(const APawn* Target) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr || Target == nullptr)
    {
        return false;
    }

    FVector EyeLoc;
    FRotator EyeRot;
    GetActorEyesViewPoint(EyeLoc, EyeRot);
    const FVector TargetChest = Target->GetActorLocation() + FVector(0.0, 0.0, 50.0);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCPoliceLOS), /*bTraceComplex=*/false, this);
    FHitResult Hit;
    const bool bBlocked = World->LineTraceSingleByChannel(
        Hit, EyeLoc, TargetChest, ECC_Visibility, Params);

    // Clear if nothing blocked, or the only thing the ray hit is the target itself.
    return !bBlocked || Hit.GetActor() == Target;
}

void AGTCPoliceOfficer::FaceTarget(const FVector& TargetPos, float DeltaSeconds)
{
    FVector ToTarget = TargetPos - GetActorLocation();
    ToTarget.Z = 0.0;
    if (ToTarget.IsNearlyZero())
    {
        return;
    }
    const double DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X));
    const float NewYaw = FMath::FixedTurn(
        static_cast<float>(GetActorRotation().Yaw), static_cast<float>(DesiredYaw),
        static_cast<float>(AimTurnRateDeg * DeltaSeconds));
    SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
}

bool AGTCPoliceOfficer::FindCover(const APawn* Target, const FVector& ThreatPos, FVector& OutCover) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr || Target == nullptr)
    {
        return false;
    }
    const FVector SelfPos = GetActorLocation();
    const FVector PlayerChest = ThreatPos + FVector(0.0, 0.0, 50.0);

    // Sample a ring of nearby spots; a spot whose sightline to the player is blocked
    // by world geometry is cover. Its normal faces the threat by construction, so
    // FCombatCover::BestCover just picks the nearest such protecting spot.
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
            FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCCoverTrace), /*bTraceComplex*/ false, this);
            const bool bBlocked =
                World->LineTraceSingleByChannel(Hit, SampleChest, PlayerChest, ECC_Visibility, Params);
            if (bBlocked && Hit.GetActor() != Target)
            {
                FCombatCover::FCoverPoint Candidate;
                Candidate.Pos = ToCorePlanar(Sample);
                Candidate.Normal = ToCorePlanar((PlayerChest - Sample).GetSafeNormal2D());
                Candidates.Add(Candidate);
            }
        }
    }
    if (Candidates.Num() == 0)
    {
        return false;
    }

    const FCombatCover::FCoverPoint Best =
        FCombatCover::BestCover(Candidates, ToCorePlanar(SelfPos), ToCorePlanar(PlayerChest));
    if (!Best.bValid)
    {
        return false;
    }
    OutCover = FromCorePlanar(Best.Pos);
    OutCover.Z = SelfPos.Z; // keep the officer's own ground height
    return true;
}

AGTCHostile* AGTCPoliceOfficer::FindNearbyHostile(double RangeCm) const
{
    const UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }
    const FVector Self = GetActorLocation();
    AGTCHostile* Best = nullptr;
    double BestDistSq = RangeCm * RangeCm;
    for (TActorIterator<AGTCHostile> It(World); It; ++It)
    {
        AGTCHostile* Foe = *It;
        if (Foe == nullptr || Foe->IsDead())
        {
            continue;
        }
        const double DistSq = FVector::DistSquared(Self, Foe->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Foe;
        }
    }
    return Best;
}

void AGTCPoliceOfficer::DriveCombat(
    float DeltaSeconds, APawn* Target, const FVector& TargetPos, bool bLos, bool bFightingHostile)
{
    const FVector SelfPos = GetActorLocation();

    // Distance is metres for the pure core; directions are planar units remapped
    // into the core's XZ frame.
    FVector ToTargetU = TargetPos - SelfPos;
    ToTargetU.Z = 0.0;
    const double DistanceM = ToTargetU.Size() / MetresToCm;
    const FVector ToTargetCore = ToCorePlanar(ToTargetU.GetSafeNormal());
    const FVector FacingCore = ToCorePlanar(GetActorForwardVector().GetSafeNormal2D());

    // --- Apprehend: at low heat, an unarmed compliant suspect is arrested rather
    // than gunned down. Closing to the catch range is what lets the (already wired)
    // bust loop land — the gunfight plan below holds at ~12-20 m, never within 2 m. ---
    // A player who's actively committing crimes (just shot someone) is treated as armed
    // and maximally threatening, so cops fight instead of walking up to cuff them — this
    // is the live fallback for the crowd snapshot's bArmed, which only flips if a weapon
    // path wires SetPlayerArmed.
    bool bPlayerArmed = bCachedPlayerDangerous;
    double PlayerThreat01 = bCachedPlayerDangerous ? 1.0 : 0.0;
    if (const UWorld* World = GetWorld())
    {
        if (const UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
        {
            const FGTCThreatSnapshot Threat = Crowd->GetPlayerThreat();
            bPlayerArmed = bPlayerArmed || Threat.bArmed;
            PlayerThreat01 = FMath::Max(
                PlayerThreat01, Threat.bArmed ? 1.0 : FMath::Clamp(Threat.Speed / 600.0, 0.0, 1.0));
        }
    }
    if (!bFightingHostile && FApprehend::ShouldApprehend(CachedStars, bPlayerArmed, PlayerThreat01))
    {
        // Walk straight in and plant inside the catch range (matches
        // UArrestSubsystem::CatchDistance) so the director's bust loop can grapple.
        constexpr double CatchRangeM = 2.0;
        const FVector ApproachDir =
            FromCorePlanar(FApprehend::ApproachHeading(ToCorePlanar(SelfPos), ToCorePlanar(TargetPos)));
        const double ApproachSpeed = FApprehend::ApproachSpeed(RunSpeed, DistanceM, CatchRangeM);
        if (UCharacterMovementComponent* Move = GetCharacterMovement())
        {
            Move->MaxWalkSpeed = static_cast<float>(RunSpeed);
        }
        if (ApproachSpeed > 0.0 && !ApproachDir.IsNearlyZero())
        {
            AddMovementInput(ApproachDir, 1.0f);
        }
        if (Weapon != nullptr)
        {
            Weapon->StopFire(); // holster while moving in to arrest
        }
        return; // skip the gunfight plan this tick
    }

    // Suppression bleeds off each tick; while high it lowers the officer's perceived
    // health (seek cover sooner) and stretches its fire interval (shoot less).
    Suppression = FSuppression::Decay(Suppression, DeltaSeconds);
    const double HealthFrac = FSuppression::EffectiveHealthFrac(Vitals.Fraction(), Suppression);
    const int32 Ammo = (Weapon != nullptr) ? Weapon->CurrentAmmoInMag() : 0;
    const bool bCooldownReady = FireTimer <= 0.0;

    // A shielded SWAT pushes forward behind the shield rather than hiding: it plans as
    // if at full health, so the tactical AI never has it take cover or retreat — the
    // frontal armor lets it survive the advance, and you have to flank it.
    const double PlanHealth = bShielded ? 1.0 : HealthFrac;
    const FPoliceCombat::FCombatPlan Plan = FPoliceCombat::Plan(
        DistanceM, bLos, FacingCore, ToTargetCore, PlanHealth, CachedStars, Ammo, bCooldownReady);

    // --- Movement: walk the chosen action's intent through CharacterMovement -----
    // Top speed rises with heat (a 5-star response closes faster than a 1-star one);
    // the per-action MoveSpeed/ChaseRun ratio is the unitless AddMovementInput scale.
    const double ChaseRun = FPoliceCombat::ChaseSpeed(RunSpeed, CachedStars);
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = static_cast<float>(ChaseRun);
    }

    // TakeCover heads to a REAL cover spot (a nearby position whose line to the
    // player is blocked), not just a backpedal; everything else uses the tested
    // per-action intent. Cover is searched on a throttle and dropped once the
    // sightline reopens (the wall no longer shields it) or the action changes.
    FVector MoveDir;
    CoverSearchTimer = FMath::Max(0.0, CoverSearchTimer - DeltaSeconds);
    if (Plan.Action == ECombatAction::TakeCover)
    {
        // Re-search cover when we have none, the sightline reopened while NOT peeking
        // (i.e. the cover was blown, not an intentional lean-out), or on the throttle.
        if (!bHasCover || (bLos && !bPeeking) || CoverSearchTimer <= 0.0)
        {
            FVector Found;
            bHasCover = FindCover(Target, TargetPos, Found);
            if (bHasCover)
            {
                CoverPos = Found;
            }
            CoverSearchTimer = 0.6;
        }
        if (bHasCover)
        {
            if (FVector::Dist2D(SelfPos, CoverPos) < 160.0)
            {
                // At cover: alternate leaning out to shoot and tucking back behind it.
                PeekTimer -= DeltaSeconds;
                if (PeekTimer <= 0.0)
                {
                    bPeeking = !bPeeking;
                    PeekTimer = bPeeking ? 1.0 : 1.4;
                }
                if (bPeeking)
                {
                    FVector ToThreat = TargetPos - CoverPos;
                    ToThreat.Z = 0.0;
                    ToThreat = ToThreat.GetSafeNormal();
                    const FVector Side(-ToThreat.Y, ToThreat.X, 0.0); // perpendicular lean
                    MoveDir = (CoverPos + Side * 140.0 - SelfPos).GetSafeNormal2D();
                }
                else
                {
                    MoveDir = (CoverPos - SelfPos).GetSafeNormal2D();
                }
            }
            else
            {
                bPeeking = false;
                FVector ToCover = CoverPos - SelfPos;
                ToCover.Z = 0.0;
                MoveDir = ToCover.GetSafeNormal();
            }
        }
        else
        {
            MoveDir = FromCorePlanar(
                FCombatAi::DesiredMove(Plan.Action, ToCorePlanar(SelfPos), ToCorePlanar(TargetPos), StrafeSign));
        }
    }
    else
    {
        bHasCover = false; // left cover; resume normal movement
        bPeeking = false;
        MoveDir = FromCorePlanar(
            FCombatAi::DesiredMove(Plan.Action, ToCorePlanar(SelfPos), ToCorePlanar(TargetPos), StrafeSign));
    }
    const double Speed = FCombatAi::MoveSpeed(Plan.Action, ChaseRun);
    const double Scale = ChaseRun > 0.0 ? Speed / ChaseRun : 0.0;
    if (Scale > 0.0 && !MoveDir.IsNearlyZero())
    {
        AddMovementInput(MoveDir, static_cast<float>(Scale));
    }

    // --- Firing -----------------------------------------------------------------
    if (Weapon == nullptr)
    {
        return;
    }

    // Reload an empty mag so the officer can keep up the pressure.
    if (Ammo <= 0)
    {
        Weapon->Reload();
    }

    // Aim straight at the target's chest every tick so shots track even between
    // trigger pulls; the spread cone comes from the weapon's own ballistics.
    FVector EyeLoc;
    FRotator EyeRot;
    GetActorEyesViewPoint(EyeLoc, EyeRot);
    const FVector AimDir = (TargetPos + FVector(0.0, 0.0, 50.0) - EyeLoc).GetSafeNormal();
    Weapon->SetAimOverride(AimDir);

    // Also fire while leaning out of cover (the plan only says Engage with feet
    // planted; a peeking officer still has the shot).
    const bool bPeekShoot = Plan.Action == ECombatAction::TakeCover
        && bHasCover && bPeeking && bLos && bCooldownReady;
    if (Plan.bFire || bPeekShoot)
    {
        // Press-then-release fires exactly one aimed round this pulse and leaves the
        // trigger UP, so an automatic can't free-run between pulses on the weapon's
        // own component tick — the police fire timer alone spaces the shots.
        Weapon->StartFire();
        Weapon->StopFire();
        FireTimer = FPoliceCombat::FireCooldown(CachedStars) * FSuppression::FireRateMult(Suppression);
    }
    else
    {
        Weapon->StopFire();
    }

    // SWAT lob a frag at mid-range to flush a player camping in cover.
    GrenadeCooldown = FMath::Max(0.0, GrenadeCooldown - DeltaSeconds);
    if (UnitType == EPoliceUnitType::Swat && bLos && GrenadeCooldown <= 0.0
        && DistanceM >= 8.0 && DistanceM <= 26.0)
    {
        if (UWorld* World = GetWorld())
        {
            const FVector Origin = GetActorLocation() + GetActorForwardVector() * 60.0 + FVector(0.0, 0.0, 60.0);
            FVector Lob = TargetPos - Origin;
            Lob.Z = 0.0;
            Lob = Lob.GetSafeNormal();
            Lob.Z = 0.5; // arc it up so it lobs rather than flies flat
            Lob = Lob.GetSafeNormal();

            TSubclassOf<AGTCThrowable> Cls = GrenadeClass;
            if (!Cls)
            {
                Cls = AGTCThrowable::StaticClass();
            }
            AGTCThrowable::SpawnAndThrow(
                World, Origin, Lob, this, /*ThrowSpeed*/ 1500.0, /*FuseSeconds*/ 2.5,
                /*CookSeconds*/ 0.0, Cls, /*bIncendiary*/ false);
            GrenadeCooldown = GrenadeIntervalSec;
        }
    }
}

// --- Damage / death ----------------------------------------------------------

float AGTCPoliceOfficer::TakeDamage(
    float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (bDead || DamageAmount <= 0.0f)
    {
        return Applied;
    }

    FVector BulletTravel = FVector::ZeroVector;
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& Pt = static_cast<const FPointDamageEvent&>(DamageEvent);
        BulletTravel = Pt.ShotDirection;
        BulletTravel.Z = 0.0;
    }

    // Heat only rises when the PLAYER attacks the law (not when officers trade fire
    // with each other or the player is hit by a stray cop round).
    bool bByPlayer = false;
    if (const UWorld* World = GetWorld())
    {
        bByPlayer = EventInstigator != nullptr && EventInstigator == World->GetFirstPlayerController();
    }

    // Riot shield: a hit into the officer's front is mostly absorbed — the bullet
    // travels roughly opposite the way the officer faces, so the player must flank.
    double Damage = static_cast<double>(DamageAmount);
    if (bShielded && !BulletTravel.IsNearlyZero())
    {
        const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
        if (FVector::DotProduct(Fwd, BulletTravel.GetSafeNormal()) < -0.3)
        {
            Damage *= (1.0 - FMath::Clamp(ShieldFrontReduction, 0.0, 1.0));
        }
    }

    ApplyGunshot(Damage, BulletTravel, bByPlayer);
    return Applied;
}

void AGTCPoliceOfficer::ApplyGunshot(double Damage, const FVector& BulletTravel, bool bByPlayer)
{
    FVector Travel = BulletTravel;
    Travel.Z = 0.0;
    Travel = Travel.GetSafeNormal();
    if (Travel.IsNearlyZero())
    {
        if (const APawn* Target = ResolveTarget())
        {
            Travel = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
        }
    }
    if (Travel.IsNearlyZero())
    {
        Travel = GetActorForwardVector().GetSafeNormal2D();
    }

    const ENpcHitResult Result = Vitals.Apply(Damage);
    if (Result == ENpcHitResult::Ignored)
    {
        return;
    }

    if (Result == ENpcHitResult::Killed)
    {
        EnterDeath(Travel, bByPlayer);
        return;
    }

    // Wounded but up: attacking a cop is a crime (raise heat), then stagger.
    if (bByPlayer)
    {
        ReportCrimeToWanted(/*bKilled=*/false);
    }

    const double Sev = FNpcVitals::WoundSeverity(Damage, Vitals.MaxHealth);
    Suppression = FSuppression::Add(Suppression, FSuppression::FromHit(Sev));
    if (Sev >= StaggerWoundSeverity)
    {
        FVector Launch = Travel * 220.0;
        Launch.Z = 120.0;
        LaunchCharacter(Launch, /*bXYOverride*/ false, /*bZOverride*/ true);
    }
    OnGunshotWound(static_cast<float>(Sev), -Travel);
}

void AGTCPoliceOfficer::EnterDeath(const FVector& BulletTravel, bool bByPlayer)
{
    if (bDead)
    {
        return;
    }
    bDead = true;

    // Killing a cop is a serious crime — report before the body goes.
    if (bByPlayer)
    {
        ReportCrimeToWanted(/*bKilled=*/true);
    }

    if (Weapon != nullptr)
    {
        Weapon->StopFire();
    }

    // Stop the controller and movement so nothing keeps driving the body.
    if (AController* C = GetController())
    {
        C->StopMovement();
    }
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->StopMovementImmediately();
        Move->DisableMovement();
    }

    // Ragdoll: simulate the body, drop the capsule out of the world, shove the
    // corpse off the round so the death reads as a hit.
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
            Body->AddImpulse(Shove * 650.0f, NAME_None, /*bVelChange*/ true);
        }
    }

    // Loot: a downed officer may leave an armor vest behind — SWAT always do, beat
    // cops on a roll — so taking on the law pays off. One-shot (not a respawner).
    if (UWorld* World = GetWorld())
    {
        const bool bAlwaysDrops = (UnitType == EPoliceUnitType::Swat);
        if (bAlwaysDrops || Rng.FRand() < LootDropChance)
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
                Loot->Kind = EGTCPickupKind::Armor;
                Loot->Amount = bAlwaysDrops ? 50.0f : 25.0f;
                Loot->bRespawns = false;
                Loot->FinishSpawning(Xform);
            }
        }
    }

    OnKilled(-BulletTravel.GetSafeNormal());
    SetLifeSpan(static_cast<float>(CorpseLingerSeconds));
}

void AGTCPoliceOfficer::ReportCrimeToWanted(bool bKilled) const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->ReportCrime(bKilled);
        }
    }
}
