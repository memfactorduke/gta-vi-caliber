// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "../Health/PlayerHealthComponent.h"
#include "../Health/PlayerHealthModel.h"
#include "../../World/Interaction/InteractionComponent.h"
#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Weapons/Throwables/GTCThrowable.h"
#include "../../Weapons/Melee/MeleeCombat.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "../../NPC/Appearance/GTCAppearanceSet.h"
#include "../../Core/GTCGameInstance.h"
#include "../../Systems/Save/SaveSubsystem.h"
#include "../Stats/PlayerStats.h"
#include "GTCPlayerState.h"
#include "GtcDamageRouting.h"
#include "../../World/Surfaces/SurfaceImpact.h"
#include "../../World/Surfaces/SurfaceImpactFX.h"

namespace
{
    /** Fallback skin palette so the Skin slot is always cyclable even before an
     *  AppearanceSet (with its own SkinTones) is authored. */
    static const TArray<FLinearColor>& GTCDefaultSkinTones()
    {
        static const TArray<FLinearColor> Tones = {
            FLinearColor(0.96f, 0.80f, 0.69f), // light
            FLinearColor(0.91f, 0.72f, 0.58f),
            FLinearColor(0.80f, 0.60f, 0.46f),
            FLinearColor(0.65f, 0.46f, 0.33f),
            FLinearColor(0.48f, 0.33f, 0.22f),
            FLinearColor(0.32f, 0.21f, 0.14f), // deep
        };
        return Tones;
    }

    const TCHAR* GTCSlotName(int32 Slot)
    {
        switch (Slot)
        {
        case GTCLookSlot::Body:   return TEXT("Body");
        case GTCLookSlot::Face:   return TEXT("Face");
        case GTCLookSlot::Hair:   return TEXT("Hair");
        case GTCLookSlot::Outfit: return TEXT("Outfit");
        case GTCLookSlot::Skin:   return TEXT("Skin");
        default:                  return TEXT("?");
        }
    }
}

AGTCPlayerCharacter::AGTCPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    // Health-only store (its FPlayerHealthModel is built with ArmorMax=0 inside
    // the component; armor lives on the PlayerState's stats component).
    HealthComponent = CreateDefaultSubobject<UPlayerHealthComponent>(TEXT("HealthComponent"));
    InteractionComponent = CreateDefaultSubobject<UGTCInteractionComponent>(TEXT("InteractionComponent"));

    // Weapon adapter: holds the equipped gun in the hand socket and converts
    // trigger input into camera-aimed line-trace shots (GTA-style). Gives itself
    // the default pistol/SMG/rifle/shotgun arsenal on BeginPlay.
    WeaponComponent = CreateDefaultSubobject<UGTCWeaponComponent>(TEXT("WeaponComponent"));

    // Third-person camera so PIE renders a framed (non-black) view — the W3
    // per-merge recording standard captures the viewport, so the pawn must own a
    // camera. The boom collision-springs toward the pawn; the controller drives
    // its rotation (bUsePawnControlRotation), and the body orients to movement.
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 80.0f);

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // The player's face: a separate head mesh that rides the body and follows its
    // pose (leader-pose follower, wired in ApplyAppearance once a face asset loads).
    HeadMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(GetMesh());

    // Hair + outfit, same leader-pose follower rig as the NPC citizens, for parity.
    HairMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HairMesh"));
    HairMesh->SetupAttachment(GetMesh());
    OutfitMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OutfitMesh"));
    OutfitMesh->SetupAttachment(GetMesh());

    bUseControllerRotationYaw = false;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bOrientRotationToMovement = true;

        // Base movement is a WALK, not a run. Without this the component keeps UE's
        // 600 cm/s default, which is ~3.7x the speed the Manny walk clip was authored
        // for: the same clip plays massively over-cranked (feet skate -> "weird") and
        // already sits at the run end of the blendspace, so a walk looks like a fast
        // run. 200 cm/s matches the authored walk cadence and the NPC crowd's walk
        // band (see GTCCitizen: WalkSpeed ~150, RunSpeed ~420), so the player animates
        // the same way the citizens already do. Sprint (LeftShift) scales this up to a
        // real run via SprintSpeedMultiplier.
        Movement->MaxWalkSpeed = 200.0f;
        // Ease into and out of motion instead of snapping to full speed in one frame,
        // which also reads as "weird" on the first step. Snappier than the NPC 900 so
        // the player still feels responsive, smoother than the 2048 default.
        Movement->MaxAcceleration = 1500.0f;
        Movement->BrakingDecelerationWalking = 1500.0f;

        // Allow ACharacter::Crouch() to take effect (shrinks the capsule). The
        // crouched POSE still needs a crouch clip the project doesn't ship yet —
        // until then the capsule lowers but the upper body stays in the idle/locomotion
        // pose. bCanSwim is on by default; swimming additionally needs a water
        // PhysicsVolume in the level and swim clips (also not yet authored).
        Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
    }

    // Default appearance slots, soft-referenced. Repoint FaceMesh at a MetaHuman (or
    // scanned) head — that is the "actual human face for me". Guarded on load, so
    // placeholder paths that don't exist yet simply leave the BP-authored mesh be.
    BodyMesh = TSoftObjectPtr<USkeletalMesh>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Characters/Player/SK_PlayerBody.SK_PlayerBody")));
    FaceMesh = TSoftObjectPtr<USkeletalMesh>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Characters/Player/SK_PlayerFace.SK_PlayerFace")));
    BodyAnimClass = TSoftClassPtr<UAnimInstance>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Characters/Player/ABP_Player.ABP_Player_C")));
    HairSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Characters/Player/SK_PlayerHair.SK_PlayerHair")));
    OutfitSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Characters/Player/SK_PlayerOutfit.SK_PlayerOutfit")));

    // The creator's wardrobe. Soft path so the headless build never hard-links it;
    // guarded on load, so until this data asset is authored the per-slot soft refs
    // above remain the fallback. Point this at the same asset the NPC crowd uses to
    // share one wardrobe between the player and the population.
    AppearanceSet = TSoftObjectPtr<UGTCAppearanceSet>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Characters/DA_PlayerAppearance.DA_PlayerAppearance")));

    // Soft paths so the editor-closed headless build never hard-links the
    // editor-authored input assets. Resolved when input is set up.
    //
    // These MUST point at the only loadable InputAction family,
    // /Game/GTCaliberAssets/Content/Input/Actions/IA_*. The old /Game/Input/IA_*
    // paths are broken redirectors that LoadSynchronous to null, which silently
    // kills movement/look/jump unless the BP CDO happens to override them — a
    // fragile dependency that has regressed before (see the IA_Move=null,
    // can't-walk signature). Defaulting to the real family makes the pawn
    // self-sufficient even with a bare/reset BP CDO.
    MoveAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_Move.IA_Move")));
    LookAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_Look.IA_Look")));
    JumpAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_Jump.IA_Jump")));
    // Interact/Fire/Reload/SwitchWeapon have no authored asset in either family yet,
    // so these resolve to null and their bindings no-op until the assets are created
    // (the runtime IMC simply skips an unmapped action). Kept under the same Actions/
    // folder so they pick up automatically once authored.
    InteractAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_Interact.IA_Interact")));
    FireAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_Fire.IA_Fire")));
    ReloadAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_Reload.IA_Reload")));
    SwitchWeaponAction = TSoftObjectPtr<UInputAction>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Input/Actions/IA_SwitchWeapon.IA_SwitchWeapon")));

    // Combat one-shot anims. Existing UE5 mannequin clips (AnimSequences); played
    // through the ABP's "DefaultSlot". Soft paths so the headless build never
    // hard-links them and a missing asset is just a no-op.
    FireAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire.MM_Pistol_Fire")));
    HitReactAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Rifle/HitReact/"
             "MM_HitReact_Front_Med_01.MM_HitReact_Front_Med_01")));

    // Emote one-shots (hey / middle finger / piss). The project ships ONLY the stock
    // UE5 mannequin combat+locomotion pack — no gesture clips exist for these, and the
    // repo rule forbids lifting them from GTA or any commercial game. So each points at
    // a path under a dedicated Emotes/ folder; the guarded load in PlayCombatAnim makes
    // every one a harmless no-op until a CC0/authored clip is dropped at its path (each
    // needs a provenance row per docs/ASSETS.md). Drop a clip in and the existing key +
    // console command light up with zero further code.
    WaveAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Emotes/MM_Wave.MM_Wave")));
    MiddleFingerAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Emotes/MM_MiddleFinger.MM_MiddleFinger")));
    PissAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Emotes/MM_Piss.MM_Piss")));

    // Held-state looping poses: crouch (CC0 mocap, retargeted to Manny) and swim. Played
    // on "DefaultSlot" while the state is held — the capsule already shrinks (crouch) /
    // engine swim-physics drives motion. Soft refs so a missing clip is a harmless no-op.
    CrouchPoseAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Locomotion/MM_Crouch_Idle.MM_Crouch_Idle")));
    SwimPoseAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Locomotion/MM_Swim_Fwd.MM_Swim_Fwd")));

    // Living pawn -> "creature" surface: rounds into the player spray blood, not a
    // generic puff. See Source/GTC_UE5/World/Surfaces/SurfaceImpact.h.
    Tags.Add(GTCSurfaceTags::CreatureTag());
}

void AGTCPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Re-apply any persistent player state the GameInstance carries (look + ammo +
    // position) — from the creator, level travel, or a loaded save. The GameInstance
    // survives travel, so the character carries across every map.
    const bool bRestoredSavedTransform = RestorePersistentState();

    ApplyAppearance();

    // Spawn safety net: if we came up far east of the city (where the stray
    // PlayerStarts currently sit, X ~0..150), drop into the built city so the
    // player isn't staring at empty terrain/ocean with no buildings in sight.
    // Self-disabling once a PlayerStart is placed in the city (X < threshold).
    // Stopgap until the level's PlayerStart is relocated in-editor. Skipped entirely when
    // a saved transform was just restored — the save wins over the relocation net.
    if (!bRestoredSavedTransform && bRelocateIfSpawnedFarFromCity && GetActorLocation().X >= FarSpawnXThreshold)
    {
        SetActorLocation(CityFallbackSpawn, /*bSweep=*/false);
        // Face roughly toward the city mass (which lies in -X) so the camera looks
        // at the buildings, not away from them.
        const FRotator FaceCity(0.0, 180.0, 0.0);
        SetActorRotation(FaceCity);
        if (AController* C = GetController())
        {
            C->SetControlRotation(FaceCity);
        }
    }
}

bool AGTCPlayerCharacter::RestorePersistentState()
{
    bool bRestoredTransform = false;
    UWorld* W = GetWorld();
    if (W == nullptr)
    {
        return false;
    }
    UGTCGameInstance* GI = W->GetGameInstance<UGTCGameInstance>();
    if (GI == nullptr)
    {
        return false;
    }

    if (GI->HasSavedLook())
    {
        Look = GI->GetSavedLook();
    }
    if (GI->HasSavedThrowableAmmo())
    {
        int32 SavedF = 0, SavedG = 0, SavedM = 0;
        GI->GetSavedThrowableAmmo(SavedF, SavedG, SavedM);
        OffenseLoadout.RestoreAmmo(SavedF, SavedG, SavedM);
        OnThrowablesChanged();
    }
    if (GI->HasSavedTransform())
    {
        FVector SavedLoc = FVector::ZeroVector;
        float SavedYaw = 0.0f;
        GI->GetSavedTransform(SavedLoc, SavedYaw);
        SetActorLocation(SavedLoc, /*bSweep=*/false);
        const FRotator SavedRot(0.0, SavedYaw, 0.0);
        SetActorRotation(SavedRot);
        if (AController* C = GetController())
        {
            C->SetControlRotation(SavedRot);
        }
        bRestoredTransform = true;
    }
    return bRestoredTransform;
}

void AGTCPlayerCharacter::GTC_SaveGame()
{
    if (UGameInstance* GIBase = GetGameInstance())
    {
        if (USaveSubsystem* Save = GIBase->GetSubsystem<USaveSubsystem>())
        {
            Save->SaveToFile(USaveSubsystem::DefaultSavePath());
        }
    }
}

void AGTCPlayerCharacter::GTC_LoadGame()
{
    UGameInstance* GIBase = GetGameInstance();
    if (GIBase == nullptr)
    {
        return;
    }
    USaveSubsystem* Save = GIBase->GetSubsystem<USaveSubsystem>();
    if (Save == nullptr)
    {
        return;
    }
    // Load applies each section's hook (wanted heat restores live; the GameInstance's
    // "player" hook updates its held look/ammo/transform). Then push that player state
    // onto THIS live pawn and refresh its appearance.
    if (Save->LoadFromFile(USaveSubsystem::DefaultSavePath()))
    {
        RestorePersistentState();
        ApplyAppearance();
    }
}

void AGTCPlayerCharacter::SyncThrowablesChanged()
{
    if (UWorld* W = GetWorld())
    {
        if (UGTCGameInstance* GI = W->GetGameInstance<UGTCGameInstance>())
        {
            GI->SetSavedThrowableAmmo(GetFlashbangAmmo(), GetGrenadeAmmo(), GetMolotovAmmo());
        }
    }
    OnThrowablesChanged();
}

void AGTCPlayerCharacter::ApplyAppearance()
{
    USkeletalMeshComponent* Body = GetMesh();
    if (!Body)
    {
        return;
    }

    // Prefer the wardrobe pools (indexed by Look); fall back to the single soft ref
    // per slot when the wardrobe is absent or a pool is empty. Each step is
    // independently guarded so a missing asset leaves whatever the BP already set.
    UGTCAppearanceSet* Set = AppearanceSet.LoadSynchronous();

    // Body mesh.
    USkeletalMesh* BodySkel = Set ? Set->ResolveBody(Look.Body) : nullptr;
    if (!BodySkel)
    {
        BodySkel = BodyMesh.LoadSynchronous();
    }
    if (BodySkel)
    {
        Body->SetSkeletalMesh(BodySkel);
        Body->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -90.0), FRotator(0.0, -90.0, 0.0));
    }

    // Locomotion ABP.
    UClass* AnimClass = Set ? Set->ResolveBodyAnimClass() : nullptr;
    if (!AnimClass)
    {
        AnimClass = BodyAnimClass.LoadSynchronous();
    }
    if (AnimClass)
    {
        Body->SetAnimInstanceClass(AnimClass);
    }

    // Face: drive the head from the body's pose so it animates with the walk. Prefer
    // the high-detail hero head, then the crowd head, then the single soft ref.
    if (HeadMesh)
    {
        USkeletalMesh* FaceSkel = nullptr;
        if (Set)
        {
            FaceSkel = Set->ResolveHeroHead(Look.Face);
            if (!FaceSkel)
            {
                FaceSkel = Set->ResolveHead(Look.Face);
            }
        }
        if (!FaceSkel)
        {
            FaceSkel = FaceMesh.LoadSynchronous();
        }
        if (FaceSkel)
        {
            HeadMesh->SetSkeletalMesh(FaceSkel);
            HeadMesh->SetLeaderPoseComponent(Body);
        }
    }

    // Hair + outfit: same leader-pose follower setup as the face, guarded.
    if (HairMesh)
    {
        USkeletalMesh* HairSkel = Set ? Set->ResolveHair(Look.Hair) : nullptr;
        if (!HairSkel)
        {
            HairSkel = HairSkeletalMesh.LoadSynchronous();
        }
        if (HairSkel)
        {
            HairMesh->SetSkeletalMesh(HairSkel);
            HairMesh->SetLeaderPoseComponent(Body);
        }
    }
    if (OutfitMesh)
    {
        USkeletalMesh* OutfitSkel = Set ? Set->ResolveOutfit(Look.Outfit) : nullptr;
        if (!OutfitSkel)
        {
            OutfitSkel = OutfitSkeletalMesh.LoadSynchronous();
        }
        if (OutfitSkel)
        {
            OutfitMesh->SetSkeletalMesh(OutfitSkel);
            OutfitMesh->SetLeaderPoseComponent(Body);
        }
    }

    // Skin tone: tint the body + head materials (matches the NPC tinting path).
    ApplySkinTint(ResolveSkinTone(Look.Skin));
}

void AGTCPlayerCharacter::ApplySkinTint(const FLinearColor& Tone)
{
    auto Tint = [&Tone](USkeletalMeshComponent* Comp)
    {
        if (!Comp)
        {
            return;
        }
        const int32 Num = Comp->GetNumMaterials();
        for (int32 i = 0; i < Num; ++i)
        {
            if (UMaterialInstanceDynamic* MID = Comp->CreateAndSetMaterialInstanceDynamic(i))
            {
                // Set every common name; the material uses whichever it declares.
                MID->SetVectorParameterValue(TEXT("SkinTone"), Tone);
                MID->SetVectorParameterValue(TEXT("Tint"), Tone);
                MID->SetVectorParameterValue(TEXT("BaseColor"), Tone);
                MID->SetVectorParameterValue(TEXT("Color"), Tone);
            }
        }
    };
    Tint(GetMesh()); // body skin
    Tint(HeadMesh);  // face skin
}

int32 AGTCPlayerCharacter::GetAppearanceSlotCount(int32 Slot) const
{
    UGTCAppearanceSet* Set = AppearanceSet.LoadSynchronous();
    switch (Slot)
    {
    case GTCLookSlot::Body:   return Set ? Set->BodyMeshes.Num() : 0;
    // Face must mirror ApplyAppearance's resolution order: it prefers the hero pool
    // (wrapping modulo its size) and only falls back to the crowd pool when hero is
    // empty. Counting max(hero,crowd) would let the index run past the hero pool and
    // desync the label from the shown mesh.
    case GTCLookSlot::Face:   return Set ? (Set->HeroHeadMeshes.Num() > 0 ? Set->HeroHeadMeshes.Num() : Set->HeadMeshes.Num()) : 0;
    case GTCLookSlot::Hair:   return Set ? Set->HairMeshes.Num() : 0;
    case GTCLookSlot::Outfit: return Set ? Set->OutfitMeshes.Num() : 0;
    case GTCLookSlot::Skin:   return (Set && Set->SkinTones.Num() > 0) ? Set->SkinTones.Num() : GTCDefaultSkinTones().Num();
    default:                  return 0;
    }
}

int32* AGTCPlayerCharacter::SlotValuePtr(int32 Slot)
{
    switch (Slot)
    {
    case GTCLookSlot::Body:   return &Look.Body;
    case GTCLookSlot::Face:   return &Look.Face;
    case GTCLookSlot::Hair:   return &Look.Hair;
    case GTCLookSlot::Outfit: return &Look.Outfit;
    case GTCLookSlot::Skin:   return &Look.Skin;
    default:                  return nullptr;
    }
}

FLinearColor AGTCPlayerCharacter::ResolveSkinTone(int32 Index) const
{
    if (UGTCAppearanceSet* Set = AppearanceSet.LoadSynchronous())
    {
        if (Set->SkinTones.Num() > 0)
        {
            return Set->ResolveSkinTone(Index, FLinearColor::White);
        }
    }
    const TArray<FLinearColor>& Pal = GTCDefaultSkinTones();
    const int32 N = Pal.Num();
    return N > 0 ? Pal[((Index % N) + N) % N] : FLinearColor::White;
}

void AGTCPlayerCharacter::CycleAppearanceSlot(int32 Slot, int32 Delta)
{
    int32* Val = SlotValuePtr(Slot);
    if (!Val)
    {
        return;
    }
    const int32 Count = GetAppearanceSlotCount(Slot);
    if (Count > 0)
    {
        *Val = ((*Val + Delta) % Count + Count) % Count; // wrap within the pool
    }
    else
    {
        *Val = FMath::Max(0, *Val + Delta); // unknown pool size: stay non-negative
    }
    ApplyAppearance();
}

void AGTCPlayerCharacter::RandomizeAppearance()
{
    for (int32 Slot = 0; Slot < GTCLookSlot::Count; ++Slot)
    {
        if (int32* Val = SlotValuePtr(Slot))
        {
            const int32 Count = GetAppearanceSlotCount(Slot);
            *Val = (Count > 0) ? FMath::RandRange(0, Count - 1) : FMath::RandRange(0, 7);
        }
    }
    ApplyAppearance();
}

FText AGTCPlayerCharacter::GetAppearanceSlotText(int32 Slot) const
{
    int32 Value = 0;
    switch (Slot)
    {
    case GTCLookSlot::Body:   Value = Look.Body;   break;
    case GTCLookSlot::Face:   Value = Look.Face;   break;
    case GTCLookSlot::Hair:   Value = Look.Hair;   break;
    case GTCLookSlot::Outfit: Value = Look.Outfit; break;
    case GTCLookSlot::Skin:   Value = Look.Skin;   break;
    default: break;
    }
    const int32 Count = GetAppearanceSlotCount(Slot);
    if (Count > 0)
    {
        return FText::FromString(FString::Printf(TEXT("%s    %d / %d"), GTCSlotName(Slot), Value + 1, Count));
    }
    return FText::FromString(FString::Printf(TEXT("%s    #%d"), GTCSlotName(Slot), Value));
}

void AGTCPlayerCharacter::CommitLook()
{
    Look.bValid = true;
    if (UWorld* W = GetWorld())
    {
        if (UGTCGameInstance* GI = W->GetGameInstance<UGTCGameInstance>())
        {
            GI->SetSavedLook(Look);
        }
    }
}

void AGTCPlayerCharacter::GTC_ResetLook()
{
    Look = FGTCPlayerLook();
    ApplyAppearance();
}

void AGTCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // LoadSynchronous tolerates a missing asset (returns null) in the
        // headless build. The IA_Move/IA_Look Axis2D composite on IMC_Default is
        // editor-authored and finalized at editor reopen; the handlers bind now.
        if (UInputAction* Move = MoveAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Move, ETriggerEvent::Triggered, this, &AGTCPlayerCharacter::HandleMove);
        }
        if (UInputAction* LookAct = LookAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(LookAct, ETriggerEvent::Triggered, this, &AGTCPlayerCharacter::HandleLook);
        }
        if (UInputAction* Jump = JumpAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Jump, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleJumpStarted);
            EnhancedInput->BindAction(Jump, ETriggerEvent::Completed, this, &AGTCPlayerCharacter::HandleJumpCompleted);
        }
        if (UInputAction* Interact = InteractAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Interact, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleInteract);
        }
        if (UInputAction* Fire = FireAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(Fire, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleFireStarted);
            EnhancedInput->BindAction(Fire, ETriggerEvent::Completed, this, &AGTCPlayerCharacter::HandleFireCompleted);
        }
        if (UInputAction* ReloadInput = ReloadAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(ReloadInput, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleReload);
        }
        if (UInputAction* Switch = SwitchWeaponAction.LoadSynchronous())
        {
            EnhancedInput->BindAction(
                Switch, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleSwitchWeapon);
        }

        // Crouch + sprint actions are created in code (no editor asset needed) and
        // mapped to keys on the runtime IMC below. Hold-to-crouch, hold-to-sprint.
        if (!RuntimeCrouchAction)
        {
            RuntimeCrouchAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeCrouch"));
        }
        if (!RuntimeSprintAction)
        {
            RuntimeSprintAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeSprint"));
        }
        if (!RuntimeAimAction)
        {
            RuntimeAimAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeAim"));
        }
        EnhancedInput->BindAction(
            RuntimeCrouchAction, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleCrouchStarted);
        EnhancedInput->BindAction(
            RuntimeCrouchAction, ETriggerEvent::Completed, this, &AGTCPlayerCharacter::HandleCrouchCompleted);
        EnhancedInput->BindAction(
            RuntimeSprintAction, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleSprintStarted);
        EnhancedInput->BindAction(
            RuntimeSprintAction, ETriggerEvent::Completed, this, &AGTCPlayerCharacter::HandleSprintCompleted);
        EnhancedInput->BindAction(
            RuntimeAimAction, ETriggerEvent::Started, this, &AGTCPlayerCharacter::HandleAimStarted);
        EnhancedInput->BindAction(
            RuntimeAimAction, ETriggerEvent::Completed, this, &AGTCPlayerCharacter::HandleAimCompleted);

        // Emotes are no longer per-key — they play through PlayEmote(Index) from the
        // single-key emote panel (B → SGTCEmoteWheel), so no emote actions bind here.
    }

    // Build the mapping context IN CODE from this pawn's own action objects. The
    // editor-authored IMC_Default this project ships is unreliable (no usable key
    // mappings; its actions point at a duplicate asset family), which silently
    // kills all Enhanced Input while controller-level key binds (Esc) keep working.
    // Mapping the keys to the exact same UInputAction objects the handlers above
    // bind makes movement/look/jump immune to that asset mess — no editor IMC and
    // no asset-identity match required.
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            UInputMappingContext* Imc = NewObject<UInputMappingContext>(this, TEXT("IMC_RuntimeDefault"));

            if (UInputAction* Move = MoveAction.LoadSynchronous())
            {
                // WASD → Axis2D, matching the UE third-person template composite.
                // SwizzleAxis default order (YXZ) routes the key's 1.0 onto Y (forward/back);
                // Negate flips the sign for back/left. D is the unmodified +X.
                FEnhancedActionKeyMapping& W = Imc->MapKey(Move, EKeys::W);
                W.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Imc));
                FEnhancedActionKeyMapping& S = Imc->MapKey(Move, EKeys::S);
                S.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Imc));
                S.Modifiers.Add(NewObject<UInputModifierNegate>(Imc));
                FEnhancedActionKeyMapping& A = Imc->MapKey(Move, EKeys::A);
                A.Modifiers.Add(NewObject<UInputModifierNegate>(Imc));
                Imc->MapKey(Move, EKeys::D);
                Imc->MapKey(Move, EKeys::Gamepad_Left2D);
            }
            if (UInputAction* LookAct = LookAction.LoadSynchronous())
            {
                Imc->MapKey(LookAct, EKeys::Mouse2D);
                Imc->MapKey(LookAct, EKeys::Gamepad_Right2D);
            }
            if (UInputAction* Jump = JumpAction.LoadSynchronous())
            {
                Imc->MapKey(Jump, EKeys::SpaceBar);
                Imc->MapKey(Jump, EKeys::Gamepad_FaceButton_Bottom);
            }
            if (UInputAction* Interact = InteractAction.LoadSynchronous())
            {
                Imc->MapKey(Interact, EKeys::E);
            }
            if (UInputAction* Fire = FireAction.LoadSynchronous())
            {
                Imc->MapKey(Fire, EKeys::LeftMouseButton);
            }
            if (UInputAction* ReloadInput = ReloadAction.LoadSynchronous())
            {
                Imc->MapKey(ReloadInput, EKeys::R);
                Imc->MapKey(ReloadInput, EKeys::Gamepad_FaceButton_Top);
            }
            if (UInputAction* Switch = SwitchWeaponAction.LoadSynchronous())
            {
                // EquipNext cycle. Mouse wheel + Q on KB&M, DPad-right on gamepad.
                // (Tab/LB is the radial weapon wheel; kept distinct from quick-cycle.)
                Imc->MapKey(Switch, EKeys::Q);
                Imc->MapKey(Switch, EKeys::MouseScrollUp);
                Imc->MapKey(Switch, EKeys::Gamepad_DPad_Right);
            }
            if (RuntimeCrouchAction)
            {
                Imc->MapKey(RuntimeCrouchAction, EKeys::C);
                Imc->MapKey(RuntimeCrouchAction, EKeys::LeftControl);
                Imc->MapKey(RuntimeCrouchAction, EKeys::Gamepad_FaceButton_Right);
            }
            if (RuntimeSprintAction)
            {
                Imc->MapKey(RuntimeSprintAction, EKeys::LeftShift);
                Imc->MapKey(RuntimeSprintAction, EKeys::Gamepad_LeftShoulder);
            }
            if (RuntimeAimAction)
            {
                // Hold-to-aim: right mouse on KB&M, left trigger on gamepad (GTA scheme).
                Imc->MapKey(RuntimeAimAction, EKeys::RightMouseButton);
                Imc->MapKey(RuntimeAimAction, EKeys::Gamepad_LeftTrigger);
            }
            // (No emote key mappings — emotes are picked from the one-key panel.)

            // High priority so it wins over any (empty/divergent) context the
            // controller may also have added. The subsystem keeps applied contexts
            // referenced, so the transient Imc stays alive without a UPROPERTY.
            Subsystem->AddMappingContext(Imc, 100);
        }
    }
}

void AGTCPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    if (Controller == nullptr)
    {
        return;
    }
    // Move relative to the controller yaw (standard third/first-person move).
    const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    AddMovementInput(Forward, Axis.Y);
    AddMovementInput(Right, Axis.X);
}

void AGTCPlayerCharacter::HandleLook(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AGTCPlayerCharacter::HandleJumpStarted(const FInputActionValue& /*Value*/)
{
    Jump();
}

void AGTCPlayerCharacter::HandleJumpCompleted(const FInputActionValue& /*Value*/)
{
    StopJumping();
}

void AGTCPlayerCharacter::HandleInteract(const FInputActionValue& /*Value*/)
{
    // Act on the interaction component's currently selected target (the component
    // refreshes selection each tick via its live overlap/score). No-op when
    // nothing is in reach.
    if (InteractionComponent != nullptr)
    {
        InteractionComponent->TriggerInteract();
    }
}

void AGTCPlayerCharacter::HandleFireStarted(const FInputActionValue& /*Value*/)
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->StartFire();
        // Visible full-body for now; change "DefaultSlot" -> "UpperBody" after the
        // layered-blend slot is authored so firing keeps the legs walking/running.
        PlayCombatAnim(FireAnim, FName(TEXT("DefaultSlot")));
    }
    // Firing also pulls the body to face the mouse/camera (hip-fire still aims at
    // the crosshair), so shots and the character line up even without the aim button.
    bFireHeld = true;
    RefreshAimPosture();
}

void AGTCPlayerCharacter::HandleFireCompleted(const FInputActionValue& /*Value*/)
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->StopFire();
    }
    bFireHeld = false;
    RefreshAimPosture();
}

void AGTCPlayerCharacter::HandleAimStarted(const FInputActionValue& /*Value*/)
{
    bAimButtonHeld = true;
    RefreshAimPosture();
}

void AGTCPlayerCharacter::HandleAimCompleted(const FInputActionValue& /*Value*/)
{
    bAimButtonHeld = false;
    RefreshAimPosture();
}

void AGTCPlayerCharacter::RefreshAimPosture()
{
    // GTA-style aim turn: while aiming or firing, lock the body yaw to the controller
    // (camera) so the character snaps to face the mouse direction; otherwise let
    // movement steer the body. The camera itself already follows the mouse via
    // HandleLook. Tracking both sources (aim button + fire) so releasing one while
    // the other is still held does not prematurely drop the aim posture.
    const bool bAiming = bAimButtonHeld || bFireHeld;
    bUseControllerRotationYaw = bAiming;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bOrientRotationToMovement = !bAiming;
    }
}

void AGTCPlayerCharacter::HandleReload(const FInputActionValue& /*Value*/)
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->Reload();
    }
}

void AGTCPlayerCharacter::HandleSwitchWeapon(const FInputActionValue& /*Value*/)
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->EquipNext();
    }
}

void AGTCPlayerCharacter::HandleCrouchStarted(const FInputActionValue& /*Value*/)
{
    Crouch();
    // Show the actual crouch pose (CC0 mocap) while the capsule is shrunk.
    StartLoopingPose(CrouchPoseAnim);
}

void AGTCPlayerCharacter::HandleCrouchCompleted(const FInputActionValue& /*Value*/)
{
    UnCrouch();
    StopLoopingPose();
}

void AGTCPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

    // Drive the swim pose off the engine's movement mode: entering water (MOVE_Swimming)
    // starts the looping swim clip, leaving it stops. A water PhysicsVolume in the level
    // is what flips the mode to Swimming.
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        if (Movement->MovementMode == MOVE_Swimming)
        {
            StartLoopingPose(SwimPoseAnim);
        }
        else if (PrevMovementMode == MOVE_Swimming)
        {
            StopLoopingPose();
        }
    }
}

void AGTCPlayerCharacter::HandleSprintStarted(const FInputActionValue& /*Value*/)
{
    if (bIsSprinting)
    {
        return; // already sprinting — don't re-capture a boosted speed as the baseline
    }
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        PreSprintSpeed = Movement->MaxWalkSpeed;
        Movement->MaxWalkSpeed = PreSprintSpeed * SprintSpeedMultiplier;
        bIsSprinting = true;
    }
}

void AGTCPlayerCharacter::HandleSprintCompleted(const FInputActionValue& /*Value*/)
{
    if (!bIsSprinting)
    {
        return;
    }
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = PreSprintSpeed;
    }
    bIsSprinting = false;
}

TArray<AGTCPlayerCharacter::FGTCEmoteInfo> AGTCPlayerCharacter::GetEmoteInfos()
{
    // Order MUST match PlayEmote()'s indices and is the order the emote wheel shows.
    // Glyphs are left empty so the slice renders the name in the stock font (the
    // shipped Roboto has no colour-emoji table); drop in an emoji-capable font and
    // fill Glyph to light them up. Descriptions are the hub "brief description".
    return {
        { NSLOCTEXT("GTCEmotes", "Wave", "Wave"),
          NSLOCTEXT("GTCEmotes", "WaveDesc", "A friendly hello"), FString() },
        { NSLOCTEXT("GTCEmotes", "MiddleFinger", "Middle Finger"),
          NSLOCTEXT("GTCEmotes", "MiddleFingerDesc", "Flip 'em off"), FString() },
        { NSLOCTEXT("GTCEmotes", "Piss", "Piss"),
          NSLOCTEXT("GTCEmotes", "PissDesc", "When you gotta go"), FString() },
    };
}

TArray<FText> AGTCPlayerCharacter::GetEmoteNames()
{
    TArray<FText> Names;
    for (const FGTCEmoteInfo& Info : GetEmoteInfos())
    {
        Names.Add(Info.Name);
    }
    return Names;
}

void AGTCPlayerCharacter::PlayEmote(int32 Index)
{
    // Full-body one-shots on "DefaultSlot" (switch to a layered "UpperBody" slot later
    // so emotes can play while walking). Index order matches GetEmoteNames().
    switch (Index)
    {
    case 0: PlayCombatAnim(WaveAnim, FName(TEXT("DefaultSlot"))); break;
    case 1: PlayCombatAnim(MiddleFingerAnim, FName(TEXT("DefaultSlot"))); break;
    case 2: PlayCombatAnim(PissAnim, FName(TEXT("DefaultSlot"))); break;
    default: break; // out of range = safe no-op
    }
}

void AGTCPlayerCharacter::PlayCombatAnim(const TSoftObjectPtr<UAnimSequence>& Anim, FName SlotName, float BlendTime)
{
    USkeletalMeshComponent* Body = GetMesh();
    if (!Body)
    {
        return;
    }
    UAnimInstance* AnimInstance = Body->GetAnimInstance();
    UAnimSequence* Seq = Anim.LoadSynchronous();
    if (!AnimInstance || !Seq)
    {
        return;
    }
    // Play through the named slot pose node so the clip is visible regardless of
    // whether it was authored as a montage. Sequences need not loop.
    AnimInstance->PlaySlotAnimationAsDynamicMontage(Seq, SlotName, BlendTime, BlendTime);
}

void AGTCPlayerCharacter::StartLoopingPose(const TSoftObjectPtr<UAnimSequence>& Anim)
{
    USkeletalMeshComponent* Body = GetMesh();
    if (!Body)
    {
        return;
    }
    UAnimInstance* AnimInstance = Body->GetAnimInstance();
    UAnimSequence* Seq = Anim.LoadSynchronous();
    if (!AnimInstance || !Seq)
    {
        return;
    }
    StopLoopingPose();
    // Large finite LoopCount = effectively held until StopLoopingPose ends it (0/-1 loop
    // semantics vary across engine versions, so don't rely on them). Played on the ABP's
    // "DefaultSlot" so the held pose is visible over the locomotion graph.
    ActiveLoopingPose = AnimInstance->PlaySlotAnimationAsDynamicMontage(
        Seq, FName(TEXT("DefaultSlot")), 0.2f, 0.2f, 1.0f, /*LoopCount=*/1000000);
}

void AGTCPlayerCharacter::StopLoopingPose()
{
    USkeletalMeshComponent* Body = GetMesh();
    if (!Body || !ActiveLoopingPose)
    {
        return;
    }
    if (UAnimInstance* AnimInstance = Body->GetAnimInstance())
    {
        AnimInstance->Montage_Stop(0.2f, ActiveLoopingPose);
    }
    ActiveLoopingPose = nullptr;
}

void AGTCPlayerCharacter::GTC_Fire()
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->StartFire();
        // Visible full-body for now; change "DefaultSlot" -> "UpperBody" after the
        // layered-blend slot is authored so firing keeps the legs walking/running.
        PlayCombatAnim(FireAnim, FName(TEXT("DefaultSlot")));
    }
}

void AGTCPlayerCharacter::GTC_StopFire()
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->StopFire();
    }
}

void AGTCPlayerCharacter::GTC_Reload()
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->Reload();
    }
}

void AGTCPlayerCharacter::GTC_NextWeapon()
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->EquipNext();
    }
}

void AGTCPlayerCharacter::GTC_GiveWeapons()
{
    if (WeaponComponent != nullptr)
    {
        WeaponComponent->GiveDefaultArsenal();
    }
}

void AGTCPlayerCharacter::GTC_Wave()
{
    PlayCombatAnim(WaveAnim, FName(TEXT("DefaultSlot")));
}

void AGTCPlayerCharacter::GTC_MiddleFinger()
{
    PlayCombatAnim(MiddleFingerAnim, FName(TEXT("DefaultSlot")));
}

void AGTCPlayerCharacter::GTC_Piss()
{
    PlayCombatAnim(PissAnim, FName(TEXT("DefaultSlot")));
}

void AGTCPlayerCharacter::AddThrowableAmmo(int32 KindIndex, int32 Count)
{
    if (KindIndex < 0 || KindIndex >= static_cast<int32>(EThrowableKind::Count))
    {
        return;
    }
    OffenseLoadout.AddAmmo(static_cast<EThrowableKind>(KindIndex), Count);
    SyncThrowablesChanged();
}

bool AGTCPlayerCharacter::IsThrowableAtCap(int32 KindIndex) const
{
    if (KindIndex < 0 || KindIndex >= static_cast<int32>(EThrowableKind::Count))
    {
        return true;
    }
    const EThrowableKind K = static_cast<EThrowableKind>(KindIndex);
    return OffenseLoadout.GetAmmo(K) >= OffenseLoadout.GetMaxAmmo(K);
}

void AGTCPlayerCharacter::GTC_ThrowGrenade()
{
    UWorld* World = GetWorld();
    if (World == nullptr || FollowCamera == nullptr)
    {
        return;
    }
    // Consume a grenade + arm the throw cooldown; refused if empty or still cooling down.
    if (!OffenseLoadout.TryThrow(EThrowableKind::Grenade, World->GetTimeSeconds()))
    {
        return;
    }
    const FVector AimDir = FollowCamera->GetForwardVector();
    // Spawn in front of the PAWN at chest height: the camera sits on a long spring arm
    // BEHIND the pawn, so spawning at the camera would drop the grenade behind the
    // player and bounce it off the player's own capsule.
    const FVector Origin = GetActorLocation() + FVector(0.0, 0.0, 50.0) + AimDir * 100.0;
    AGTCThrowable::SpawnAndThrow(
        World, Origin, AimDir, this, /*ThrowSpeed cm/s*/ 1500.0,
        /*FuseSeconds*/ 3.5, /*CookSeconds*/ 0.0, AGTCThrowable::StaticClass());
    SyncThrowablesChanged();
}

void AGTCPlayerCharacter::GTC_ThrowMolotov()
{
    UWorld* World = GetWorld();
    if (World == nullptr || FollowCamera == nullptr)
    {
        return;
    }
    if (!OffenseLoadout.TryThrow(EThrowableKind::Molotov, World->GetTimeSeconds()))
    {
        return;
    }
    const FVector AimDir = FollowCamera->GetForwardVector();
    // Spawn in front of the pawn at chest height (camera is behind on a spring arm).
    const FVector Origin = GetActorLocation() + FVector(0.0, 0.0, 50.0) + AimDir * 100.0;
    AGTCThrowable::SpawnAndThrow(
        World, Origin, AimDir, this, /*ThrowSpeed cm/s*/ 1300.0,
        /*FuseSeconds*/ 1.5, /*CookSeconds*/ 0.0, AGTCThrowable::StaticClass(), /*bIncendiary*/ true);
    SyncThrowablesChanged();
}

void AGTCPlayerCharacter::GTC_ThrowFlashbang()
{
    UWorld* World = GetWorld();
    if (World == nullptr || FollowCamera == nullptr)
    {
        return;
    }
    // The throw cooldown (1.2s) outlasts nothing, but it's the gate that stops the
    // flashbang from chain-stunning a whole squad faster than the 3s stun wears off.
    if (!OffenseLoadout.TryThrow(EThrowableKind::Flashbang, World->GetTimeSeconds()))
    {
        return;
    }
    const FVector AimDir = FollowCamera->GetForwardVector();
    const FVector Origin = GetActorLocation() + FVector(0.0, 0.0, 50.0) + AimDir * 100.0;
    AGTCThrowable::SpawnAndThrow(
        World, Origin, AimDir, this, /*ThrowSpeed cm/s*/ 1500.0,
        /*FuseSeconds*/ 1.5, /*CookSeconds*/ 0.0, AGTCThrowable::StaticClass(),
        /*bIncendiary*/ false, /*bFlashbang*/ true);
    SyncThrowablesChanged();
}

void AGTCPlayerCharacter::GTC_Melee()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    // Melee cooldown (0.8s): stops chaining takedowns down a stunned line of enemies.
    if (!OffenseLoadout.TryMelee(World->GetTimeSeconds()))
    {
        return;
    }
    const FVector Self = GetActorLocation();
    const FVector Forward =
        (FollowCamera != nullptr ? FollowCamera->GetForwardVector() : GetActorForwardVector()).GetSafeNormal2D();

    // Pick the nearest pawn in a ~70-degree front arc within reach and strike it.
    constexpr double ReachSq = 220.0 * 220.0; // ~2.2 m
    constexpr double FrontArcDot = 0.35;
    APawn* Best = nullptr;
    double BestDistSq = ReachSq;
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* Other = *It;
        if (Other == nullptr || Other == this)
        {
            continue;
        }
        FVector To = Other->GetActorLocation() - Self;
        To.Z = 0.0;
        const double DistSq = To.SizeSquared();
        if (DistSq > BestDistSq)
        {
            continue;
        }
        if (FVector::DotProduct(Forward, To.GetSafeNormal()) < FrontArcDot)
        {
            continue; // outside the swing arc
        }
        BestDistSq = DistSq;
        Best = Other;
    }
    if (Best != nullptr)
    {
        const FVector HitDir = (Best->GetActorLocation() - Self).GetSafeNormal();
        // A strike into an enemy's back is a stealth takedown — lethal; otherwise a
        // normal melee blow.
        const FVector TargetFwd = Best->GetActorForwardVector().GetSafeNormal2D();
        const FVector TargetToPlayer = (Self - Best->GetActorLocation()).GetSafeNormal2D();
        const bool bFromBehind = FVector::DotProduct(TargetFwd, TargetToPlayer) < -0.5;
        const double Damage = bFromBehind ? 1.0e4 : FMeleeCombat::BaseDamage(EMeleeStrike::Heavy);
        UGameplayStatics::ApplyPointDamage(
            Best, static_cast<float>(Damage), HitDir, FHitResult(), GetController(), this, nullptr);

        // Blood on a melee blow: the strike only ever lands on a pawn (every pawn carries the
        // Creature tag), so a synthetic hit at chest height resolves to the blood burst through
        // the same registry the guns use. See Source/GTC_UE5/World/Surfaces/SurfaceImpactFX.h.
        FHitResult MeleeHit;
        MeleeHit.HitObjectHandle = FActorInstanceHandle(Best);
        MeleeHit.ImpactPoint = Best->GetActorLocation() + FVector(0.0, 0.0, 40.0);
        MeleeHit.ImpactNormal = -HitDir;
        FGTCImpactFX::PlayImpact(World, MeleeHit);

        // A frontal blow shoves the target back (creating space); a back-takedown just
        // drops them.
        if (!bFromBehind)
        {
            if (ACharacter* HitChar = Cast<ACharacter>(Best))
            {
                FVector Launch = HitDir * 45000.0;
                Launch.Z = 25000.0;
                HitChar->LaunchCharacter(Launch, true, true);
            }
        }
    }
}

float AGTCPlayerCharacter::TakeDamageRouted(float Amount)
{
    // The single damage-routing entry point, mirroring GtcDamage::ApplyToPlayer
    // (the tested pure logic) through the component mutators so the
    // server-authoritative broadcasts + replicated mirrors fire:
    //   1. stats armor (the SOLE pool, on the PlayerState) soaks first,
    //   2. the overflow hits this pawn's health-only model (ArmorMax=0 -> no
    //      second soak). Exactly one soak path, no double-soak, no second pool.
    AGTCPlayerState* PS = GetPlayerState<AGTCPlayerState>();
    if (PS == nullptr || PS->StatsComponent == nullptr || HealthComponent == nullptr)
    {
        return Amount;
    }
    const float Overflow = PS->StatsComponent->SoakDamage(Amount);
    HealthComponent->ApplyDamage(Overflow);

    // Flinch when damage actually lands (after armor) and the player is still up.
    if (Overflow > 0.0f && !HealthComponent->IsDead())
    {
        PlayCombatAnim(HitReactAnim);
    }
    return Overflow;
}
