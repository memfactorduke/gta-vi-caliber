// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCWeaponComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

#include "../Firing/WeaponFireController.h"
#include "../Ballistics/WeaponBallistics.h"
#include "../Core/WeaponStats.h"

namespace
{
    // FWeaponStats distances are metres (Godot frame); Unreal world units are cm.
    constexpr double MetresToCm = 100.0;

    // Default placeholder so the player visibly holds a gun-shaped object with no
    // game content authored. The engine cube is 100cm; we scale it to a barrel.
    const TCHAR* const DefaultPlaceholderPath = TEXT("/Engine/BasicShapes/Cube.Cube");
}

UGTCWeaponComponent::UGTCWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    PlaceholderWeaponMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultPlaceholderPath));
}

void UGTCWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    EnsureWeaponMeshAttached();

    if (Arsenal.Num() == 0)
    {
        GiveDefaultArsenal();
    }
    else
    {
        OnEquippedChanged();
    }
}

void UGTCWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (FWeaponFireController* Ctrl = Equipped())
    {
        Ctrl->ReleaseTrigger();
    }
    Super::EndPlay(EndPlayReason);
}

void UGTCWeaponComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (FWeaponFireController* Ctrl = Equipped())
    {
        Ctrl->Tick(DeltaTime);
        // No-op unless the trigger is held and the cadence/ammo allow a shot;
        // this is what makes automatic weapons keep firing while held.
        FireOneShot();
    }
}

// --- Input-facing API --------------------------------------------------------

void UGTCWeaponComponent::StartFire()
{
    if (FWeaponFireController* Ctrl = Equipped())
    {
        Ctrl->PressTrigger();
        // Fire instantly on the press for a responsive trigger; automatics then
        // continue in TickComponent, semi-autos latch until release.
        FireOneShot();
    }
}

void UGTCWeaponComponent::StopFire()
{
    if (FWeaponFireController* Ctrl = Equipped())
    {
        Ctrl->ReleaseTrigger();
    }
}

void UGTCWeaponComponent::Reload()
{
    if (FWeaponFireController* Ctrl = Equipped())
    {
        Ctrl->Reload();
    }
}

void UGTCWeaponComponent::EquipNext()
{
    if (Arsenal.Num() > 0)
    {
        EquipIndex((EquippedIndex + 1) % Arsenal.Num());
    }
}

void UGTCWeaponComponent::EquipPrevious()
{
    if (Arsenal.Num() > 0)
    {
        EquipIndex((EquippedIndex - 1 + Arsenal.Num()) % Arsenal.Num());
    }
}

bool UGTCWeaponComponent::EquipIndex(int32 Index)
{
    if (!Arsenal.IsValidIndex(Index))
    {
        return false;
    }
    // Releasing the old trigger stops a held automatic from "carrying over".
    if (FWeaponFireController* Ctrl = Equipped())
    {
        Ctrl->ReleaseTrigger();
    }
    EquippedIndex = Index;
    OnEquippedChanged();
    return true;
}

void UGTCWeaponComponent::GiveDefaultArsenal()
{
    Arsenal.Reset();
    WeaponNames.Reset();

    auto Add = [this](const FWeaponStats& Stats)
    {
        Arsenal.Add(MakeShared<FWeaponFireController>(Stats));
        WeaponNames.Add(Stats.DisplayName);
    };
    Add(FWeaponStats::Pistol());
    Add(FWeaponStats::Smg());
    Add(FWeaponStats::Rifle());
    Add(FWeaponStats::Shotgun());

    EquippedIndex = 0;
    OnEquippedChanged();
}

void UGTCWeaponComponent::SetSingleWeapon(const FWeaponStats& Stats)
{
    Arsenal.Reset();
    WeaponNames.Reset();
    Arsenal.Add(MakeShared<FWeaponFireController>(Stats));
    WeaponNames.Add(Stats.DisplayName);
    EquippedIndex = 0;
    OnEquippedChanged();
}

void UGTCWeaponComponent::SetAimOverride(const FVector& WorldAimDir)
{
    const FVector Dir = WorldAimDir.GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        return; // keep the previous aim rather than firing at world origin
    }
    AimOverrideDir = Dir;
    bHasAimOverride = true;
}

void UGTCWeaponComponent::ClearAimOverride()
{
    bHasAimOverride = false;
}

// --- HUD queries -------------------------------------------------------------

FString UGTCWeaponComponent::CurrentWeaponName() const
{
    return WeaponNames.IsValidIndex(EquippedIndex) ? WeaponNames[EquippedIndex] : FString();
}

int32 UGTCWeaponComponent::CurrentAmmoInMag() const
{
    const FWeaponFireController* Ctrl = Equipped();
    return Ctrl ? Ctrl->AmmoInMag() : 0;
}

int32 UGTCWeaponComponent::CurrentReserveAmmo() const
{
    const FWeaponFireController* Ctrl = Equipped();
    return Ctrl ? Ctrl->Reserve() : 0;
}

int32 UGTCWeaponComponent::WeaponCount() const
{
    return Arsenal.Num();
}

TArray<FGTCWeaponWheelEntry> UGTCWeaponComponent::WeaponWheelEntries() const
{
    TArray<FGTCWeaponWheelEntry> Out;
    Out.Reserve(Arsenal.Num());
    for (int32 i = 0; i < Arsenal.Num(); ++i)
    {
        const FWeaponFireController* Ctrl = Arsenal[i].Get();
        if (!Ctrl)
        {
            continue;
        }
        const FWeaponStats& S = Ctrl->Weapon.Stats;

        FGTCWeaponWheelEntry E;
        E.Name = WeaponNames.IsValidIndex(i) ? WeaponNames[i] : S.DisplayName;
        E.bAutomatic = S.bAutomatic;
        E.AmmoInMag = Ctrl->AmmoInMag();
        E.Reserve = Ctrl->Reserve();

        // Brief one-liner for the wheel hub (the "center description").
        const TCHAR* Mode = S.bAutomatic ? TEXT("Automatic") : TEXT("Semi-auto");
        if (S.Pellets > 1)
        {
            E.Blurb = FString::Printf(
                TEXT("%s · %d-round mag · %d pellets"), Mode, S.MagSize, S.Pellets);
        }
        else
        {
            E.Blurb = FString::Printf(
                TEXT("%s · %d-round mag · %.0f dmg"), Mode, S.MagSize, S.Damage);
        }
        Out.Add(MoveTemp(E));
    }
    return Out;
}

// --- Internals ---------------------------------------------------------------

FWeaponFireController* UGTCWeaponComponent::Equipped() const
{
    return Arsenal.IsValidIndex(EquippedIndex) ? Arsenal[EquippedIndex].Get() : nullptr;
}

UCameraComponent* UGTCWeaponComponent::FindOwnerCamera() const
{
    const AActor* OwnerActor = GetOwner();
    return OwnerActor ? OwnerActor->FindComponentByClass<UCameraComponent>() : nullptr;
}

void UGTCWeaponComponent::EnsureWeaponMeshAttached()
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor == nullptr)
    {
        return;
    }

    if (WeaponMesh == nullptr)
    {
        WeaponMesh = NewObject<UStaticMeshComponent>(OwnerActor, TEXT("GTCWeaponMesh"));
        WeaponMesh->SetMobility(EComponentMobility::Movable);
        // The held mesh is cosmetic — never let it block our own shot traces.
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponMesh->SetCastShadow(true);
        WeaponMesh->RegisterComponent();
    }

    if (UStaticMesh* Mesh = PlaceholderWeaponMesh.LoadSynchronous())
    {
        WeaponMesh->SetStaticMesh(Mesh);
    }

    // Attach to the character's hand socket when available; otherwise fall back to
    // the actor root so the mesh is still parented to the pawn.
    USceneComponent* AttachParent = OwnerActor->GetRootComponent();
    if (const ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor))
    {
        if (USkeletalMeshComponent* BodyMesh = OwnerChar->GetMesh())
        {
            AttachParent = BodyMesh;
        }
    }
    if (AttachParent != nullptr)
    {
        WeaponMesh->AttachToComponent(
            AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
        // Pose the placeholder cube into a barrel pointing out of the fist.
        WeaponMesh->SetRelativeLocation(FVector(-2.0f, 5.0f, 0.0f));
        WeaponMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
        WeaponMesh->SetRelativeScale3D(FVector(0.35f, 0.06f, 0.06f));
    }
}

void UGTCWeaponComponent::OnEquippedChanged()
{
    if (WeaponMesh != nullptr)
    {
        // Index 0 is the pistol in the default arsenal; everything is visible.
        WeaponMesh->SetVisibility(Equipped() != nullptr);
    }
    OnWeaponChanged.Broadcast(CurrentWeaponName());
}

void UGTCWeaponComponent::FireOneShot()
{
    FWeaponFireController* Ctrl = Equipped();
    if (Ctrl == nullptr)
    {
        return;
    }

    // The pure-core cadence decides whether a round actually leaves the barrel
    // (released trigger / semi-auto latch / cooldown / empty mag all block here).
    if (!Ctrl->TryFire())
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();
    if (OwnerActor == nullptr || World == nullptr)
    {
        return;
    }

    // Aim source: the third-person follow camera if present (the player), else the
    // pawn eyes. For a camera-less owner (an NPC/police officer), an AI aim override
    // points the shot at its target; absent that, the eyes view direction is used.
    // The camera always wins, so this path never perturbs player aim.
    FVector CamLoc;
    FVector CamForward;
    if (const UCameraComponent* Cam = FindOwnerCamera())
    {
        CamLoc = Cam->GetComponentLocation();
        CamForward = Cam->GetForwardVector();
    }
    else
    {
        FRotator ViewRot;
        OwnerActor->GetActorEyesViewPoint(CamLoc, ViewRot);
        CamForward = bHasAimOverride ? AimOverrideDir : ViewRot.Vector();
    }

    // Visual muzzle origin for tracers/flash (falls back to the camera).
    FVector MuzzleLoc = CamLoc;
    if (WeaponMesh != nullptr)
    {
        MuzzleLoc = WeaponMesh->DoesSocketExist(MuzzleSocketName)
            ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
            : WeaponMesh->GetComponentLocation();
    }

    const FWeaponStats& Stats = Ctrl->Weapon.Stats;
    const double RangeCm = Stats.Range * MetresToCm;
    const double Spread = Ctrl->CurrentSpread();
    const int32 Pellets = FMath::Max(Stats.Pellets, 1);

    AController* Instigator = nullptr;
    if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
    {
        Instigator = OwnerPawn->GetController();
    }

    FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(GTCWeaponShot), /*bTraceComplex=*/true, OwnerActor);
    TraceParams.bReturnPhysicalMaterial = false;

    FVector PrimaryImpact = CamLoc + CamForward * RangeCm;
    bool bAnyHit = false;

    for (int32 Pellet = 0; Pellet < Pellets; ++Pellet)
    {
        const FVector Dir = FWeaponBallistics::SpreadDirection(CamForward, Spread, SpreadRng);
        const FVector TraceEnd = CamLoc + Dir * RangeCm;

        FHitResult Hit;
        const bool bHit = World->LineTraceSingleByChannel(
            Hit, CamLoc, TraceEnd, TraceChannel.GetValue(), TraceParams);

        const FVector Impact = bHit ? Hit.ImpactPoint : TraceEnd;
        if (Pellet == 0)
        {
            PrimaryImpact = Impact;
            bAnyHit = bHit;
        }

        if (bHit && Hit.GetActor() != nullptr && Hit.GetActor() != OwnerActor)
        {
            const double DistanceM = Hit.Distance / MetresToCm;
            const FString BodyPart = BodyPartFromBone(Hit.BoneName);
            const double Damage = FWeaponBallistics::EffectiveDamage(
                Stats.Damage, DistanceM, BodyPart, Stats.DamageFalloffStart, Stats.DamageFalloffEnd,
                Stats.MinDamageFraction);

            UGameplayStatics::ApplyPointDamage(
                Hit.GetActor(), static_cast<float>(Damage), Dir, Hit, Instigator, OwnerActor, nullptr);
        }

#if ENABLE_DRAW_DEBUG
        if (bDrawDebugShots)
        {
            DrawDebugLine(World, MuzzleLoc, Impact, bHit ? FColor::Red : FColor::Yellow, false, 0.4f, 0, 0.5f);
            if (bHit)
            {
                DrawDebugPoint(World, Impact, 8.0f, FColor::Red, false, 0.4f);
            }
        }
#endif
    }

    // Upward recoil kick on the controller pitch (RecoilKick is radians; negative
    // pitch input looks up). Camera-driven aim only: an AI owner aims via the world
    // override (no camera), so kicking its controller pitch would just accumulate
    // unused drift on the AIController's control rotation.
    if (FindOwnerCamera() != nullptr)
    {
        if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
        {
            OwnerPawn->AddControllerPitchInput(-FMath::RadiansToDegrees(Stats.RecoilKick));
        }
    }

    OnWeaponFired.Broadcast(MuzzleLoc, PrimaryImpact, bAnyHit);
}

FString UGTCWeaponComponent::BodyPartFromBone(FName BoneName)
{
    const FString Bone = BoneName.ToString().ToLower();
    if (Bone.Contains(TEXT("head")) || Bone.Contains(TEXT("neck")))
    {
        return TEXT("head");
    }
    if (Bone.Contains(TEXT("arm")) || Bone.Contains(TEXT("hand")) || Bone.Contains(TEXT("clavicle"))
        || Bone.Contains(TEXT("leg")) || Bone.Contains(TEXT("thigh")) || Bone.Contains(TEXT("calf"))
        || Bone.Contains(TEXT("foot")))
    {
        return TEXT("limb");
    }
    // Spine/pelvis/chest and unrecognised bones resolve to the torso (x1.0).
    return TEXT("torso");
}
