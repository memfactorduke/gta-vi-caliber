// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GTCPlayerLook.h"
#include "../Offense/PlayerOffenseLoadout.h"
#include "GTCPlayerCharacter.generated.h"

class UPlayerHealthComponent;
class UGTCInteractionComponent;
class UGTCWeaponComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimInstance;
class UAnimSequence;
class UAnimMontage;
class UGTCAppearanceSet;
enum EMovementMode : int;
struct FInputActionValue;

/**
 * AGTCPlayerCharacter — the player pawn (ACharacter with CharacterMovementComponent).
 *
 * Owns a UPlayerHealthComponent (HEALTH ONLY — its model is constructed with
 * ArmorMax=0 so the armor pool is neutralized per the W3 armor-ownership
 * resolution). Armor + money live on the StatsComponent owned by
 * AGTCPlayerState.
 *
 * Input: binds IA_Move / IA_Look / IA_Jump via UEnhancedInputComponent in
 * SetupPlayerInputComponent. The input ASSETS are soft-referenced by path
 * (/Game/Input/IA_*) so the editor-closed headless build has no hard editor
 * dependency. The IA_Move / IA_Look Axis2D *composite mapping* on IMC_Default is
 * editor-authored and is finalized when the editor reopens — the C++ handlers
 * are bound now; the composite wiring is DEFERRED to the editor-reopen phase.
 *
 * Damage routing: TakeDamageRouted feeds the single GtcDamage::ApplyToPlayer
 * entry point (stats armor soaks first, overflow hits health-only model).
 */
UCLASS()
class GTC_UE5_API AGTCPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGTCPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    /** Drives the looping swim pose: start it on entering MOVE_Swimming (a water
     *  volume), stop it on leaving. The capsule's swim physics is engine-handled. */
    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

    /** Health store (health-only; armor neutralized via ArmorMax=0). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player")
    TObjectPtr<UPlayerHealthComponent> HealthComponent;

    /** Context-sensitive interaction (gathers/scores IGTCInteractable targets). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player")
    TObjectPtr<UGTCInteractionComponent> InteractionComponent;

    /** Holds + fires the player's weapons (attaches to the hand, line-traces shots). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Weapon")
    TObjectPtr<UGTCWeaponComponent> WeaponComponent;

    /** Collision-aware third-person camera boom (controller drives its rotation). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    /** Follow camera at the boom end — gives PIE a framed view (non-black capture). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    /** The player's face — a separate head mesh riding the body as a leader-pose
     *  follower. Point FaceMesh at a MetaHuman (or scanned) head and this is "you". */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TObjectPtr<USkeletalMeshComponent> HeadMesh;

    /** The player's hair, leader-pose follower of the body (matches the NPC rig). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TObjectPtr<USkeletalMeshComponent> HairMesh;

    /** The player's outfit/clothing, leader-pose follower of the body. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TObjectPtr<USkeletalMeshComponent> OutfitMesh;

    // --- Appearance (soft-referenced so the headless build never hard-links them) ---

    /** The player's body skeletal mesh. Defaults to a placeholder path; repoint at
     *  a MetaHuman body in a BP child or here. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TSoftObjectPtr<USkeletalMesh> BodyMesh;

    /** The player's face/head skeletal mesh (drives HeadMesh). A MetaHuman head goes
     *  here — this is the "actual human face for me" slot. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TSoftObjectPtr<USkeletalMesh> FaceMesh;

    /** Animation Blueprint for the body (locomotion). Soft class ref. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TSoftClassPtr<UAnimInstance> BodyAnimClass;

    /** The player's hair mesh (drives HairMesh). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TSoftObjectPtr<USkeletalMesh> HairSkeletalMesh;

    /** The player's outfit/clothing mesh (drives OutfitMesh). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TSoftObjectPtr<USkeletalMesh> OutfitSkeletalMesh;

    /** The data-authored wardrobe the character creator draws from — the SAME pools
     *  the NPC crowd uses (body / head / hair / outfit / skin). When assigned, the
     *  creator's per-slot indices resolve against these pools; when absent (or a pool
     *  is empty) the single soft refs above are used as the fallback, so an
     *  unauthored wardrobe never breaks the build. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GTC|Player|Appearance")
    TSoftObjectPtr<UGTCAppearanceSet> AppearanceSet;

    // --- Runtime character creator API -----------------------------------------

    /** Step one appearance slot (GTCLookSlot ids) by Delta (-1 / +1) and re-apply.
     *  Wraps within the slot's pool when the pool size is known. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Appearance")
    void CycleAppearanceSlot(int32 Slot, int32 Delta);

    /** Roll every appearance slot to a random valid value and re-apply. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Appearance")
    void RandomizeAppearance();

    /** Live label for a slot, e.g. "Face   2 / 6" (or "Face   #2" when the pool
     *  size is unknown). Drives the creator widget's row text. */
    FText GetAppearanceSlotText(int32 Slot) const;

    /** Persist the current look onto the GameInstance so it survives level travel
     *  (the built character then follows the player onto every map). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Appearance")
    void CommitLook();

    /** Console: reset every appearance slot to 0 and re-apply. */
    UFUNCTION(Exec)
    void GTC_ResetLook();

    /**
     * Route incoming damage through the W3 resolution: stats armor (on the
     * PlayerState) soaks first, the overflow hits this pawn's health-only model.
     * Returns the overflow that reached health.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player")
    float TakeDamageRouted(float Amount);

    // --- Console test commands (callable from the ~ console while possessed, so
    //     weapons are usable even before IMC_Default input mapping is authored) ---

    /** Pull the trigger — one shot for semi-autos, hold-to-fire for automatics. */
    UFUNCTION(Exec)
    void GTC_Fire();

    /** Release the trigger (stops an automatic that GTC_Fire started). */
    UFUNCTION(Exec)
    void GTC_StopFire();

    /** Reload the equipped weapon from reserve. */
    UFUNCTION(Exec)
    void GTC_Reload();

    /** Cycle to the next weapon (pistol → SMG → rifle → shotgun → …). */
    UFUNCTION(Exec)
    void GTC_NextWeapon();

    /** Re-grant the default pistol/SMG/rifle/shotgun arsenal with full ammo. */
    UFUNCTION(Exec)
    void GTC_GiveWeapons();

    /** Play the "hey" wave/greeting emote (also bound to a key via the runtime IMC).
     *  No-op until a WaveAnim clip is present. */
    UFUNCTION(Exec)
    void GTC_Wave();

    /** Play the flip-the-bird (middle finger) emote (also key-bound via the runtime
     *  IMC). No-op until a MiddleFingerAnim clip is present. */
    UFUNCTION(Exec)
    void GTC_MiddleFinger();

    /** Play the urinate ("piss") emote. No-op until a PissAnim clip is present. */
    UFUNCTION(Exec)
    void GTC_Piss();

    /** Lob a fragmentation grenade from the camera: a short-fused throwable that
     *  arcs, bounces, and detonates (AGTCThrowable over FThrowable/FExplosionModel). */
    UFUNCTION(Exec)
    void GTC_ThrowGrenade();

    /** Lob a molotov from the camera: a throwable that detonates and leaves a
     *  spreading fire (AGTCThrowable incendiary + AGTCFire). */
    UFUNCTION(Exec)
    void GTC_ThrowMolotov();

    /** Front-arc melee strike at the nearest pawn ahead (FMeleeCombat damage) — rounds
     *  out player offense alongside guns and throwables. */
    UFUNCTION(Exec)
    void GTC_Melee();

    /** Lob a flashbang: stuns nearby enemies for a few seconds (no damage). */
    UFUNCTION(Exec)
    void GTC_ThrowFlashbang();

    /** Write a savegame to the default slot (wanted level + look + ammo + position). */
    UFUNCTION(Exec)
    void GTC_SaveGame();

    /** Load the default-slot savegame and re-apply it to the live player. */
    UFUNCTION(Exec)
    void GTC_LoadGame();

    // --- Throwable ammo / action pacing (FPlayerOffenseLoadout) -----------------
    // The throw/melee execs consume a finite, rate-limited loadout, so they're real
    // abilities rather than spammable cheats.

    UFUNCTION(BlueprintPure, Category = "GTC|Offense")
    int32 GetFlashbangAmmo() const { return OffenseLoadout.GetAmmo(EThrowableKind::Flashbang); }

    UFUNCTION(BlueprintPure, Category = "GTC|Offense")
    int32 GetGrenadeAmmo() const { return OffenseLoadout.GetAmmo(EThrowableKind::Grenade); }

    UFUNCTION(BlueprintPure, Category = "GTC|Offense")
    int32 GetMolotovAmmo() const { return OffenseLoadout.GetAmmo(EThrowableKind::Molotov); }

    /** Refill a throwable (KindIndex 0=flashbang, 1=grenade, 2=molotov), clamped to its
     *  cap — for ammo pickups / debug. Fires OnThrowablesChanged. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Offense")
    void AddThrowableAmmo(int32 KindIndex, int32 Count);

    /** True if a throwable kind is already at its carry cap (a pickup leaves itself on
     *  the ground rather than be wasted). Out-of-range index reads as full. */
    UFUNCTION(BlueprintPure, Category = "GTC|Offense")
    bool IsThrowableAtCap(int32 KindIndex) const;

    /** HUD seam: fired whenever a throwable count changes (a throw or a refill). */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|Offense")
    void OnThrowablesChanged();

    /** Play an emote by its index in GetEmoteNames() (the order the emote panel shows).
     *  The single entry point the emote panel calls; out-of-range is a safe no-op. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Player|Anim")
    void PlayEmote(int32 Index);

    /** One emote's wheel data: display name, a one-line description for the wheel hub
     *  (the "center description"), and a short glyph drawn on the slice (optional;
     *  empty renders the name alone, font-safe). Index order matches PlayEmote(). */
    struct FGTCEmoteInfo
    {
        FText Name;
        FText Description;
        FString Glyph;
    };

    /** Every emote for the wheel, in PlayEmote() index order. Single source of truth:
     *  GetEmoteNames() is derived from it so the picker, hub text, and PlayEmote can
     *  never drift apart. Adding an emote here (plus its clip) lights up the wheel. */
    static TArray<FGTCEmoteInfo> GetEmoteInfos();

    /** Display names of the player's emotes, in panel order. Derived from
     *  GetEmoteInfos() so the picker and PlayEmote stay in sync from one list. */
    static TArray<FText> GetEmoteNames();

protected:
    /** Finite throwable ammo + throw/melee cooldowns the offense execs consume. Plain
     *  pure-core (not a UPROPERTY); seeded from its own defaults. */
    FPlayerOffenseLoadout OffenseLoadout;

    // --- Enhanced Input actions (soft-referenced by path) ----------------------

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> FireAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> ReloadAction;

    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputAction> SwitchWeaponAction;

    // Crouch + sprint use input actions created in code (no editor-authored
    // IA_Crouch / IA_Sprint assets exist), so they work in the headless build and
    // are immune to the same IMC asset mess HandleMove documents. Kept alive by
    // these UPROPERTYs (and by the runtime IMC that maps keys to them).
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> RuntimeCrouchAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> RuntimeSprintAction;

    // Aim is a code-created action too (no editor IA_Aim asset). Hold to pull the
    // body to face the camera/mouse (GTA-style over-the-shoulder aim); release to
    // return to free-run orientation. Mapped to Right Mouse / Left Trigger below.
    UPROPERTY(Transient)
    TObjectPtr<UInputAction> RuntimeAimAction;

    // Emotes (wave / middle finger / piss) are no longer per-key: they are played
    // through PlayEmote(Index) from the single-key emote panel (SGTCEmoteWheel), so
    // no RuntimeWave/MiddleFinger/Piss input actions or hard keys exist anymore.

    // --- Combat one-shot anims (soft-referenced AnimSequences, played through the
    //     ABP's "DefaultSlot" as dynamic montages so the slot always matches) -----

    /** Fired (shoot) gesture. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Anim")
    TSoftObjectPtr<UAnimSequence> FireAnim;

    /** Flinch played when incoming damage lands. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Anim")
    TSoftObjectPtr<UAnimSequence> HitReactAnim;

    /** "Hey" wave/greeting emote, played one-shot through the ABP's "DefaultSlot"
     *  (same path as the combat one-shots). Soft ref + guarded load, so it is a
     *  no-op until a wave clip is authored/sourced at the configured path. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Anim")
    TSoftObjectPtr<UAnimSequence> WaveAnim;

    /** Flip-the-bird (middle finger) emote, one-shot through "DefaultSlot". Soft ref +
     *  guarded load → no-op until a clip is authored/sourced at the configured path. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Anim")
    TSoftObjectPtr<UAnimSequence> MiddleFingerAnim;

    /** Urinate ("piss") emote, one-shot through "DefaultSlot". Soft ref + guarded load
     *  → no-op until a clip is authored/sourced at the configured path. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Anim")
    TSoftObjectPtr<UAnimSequence> PissAnim;

    /** Looping crouch pose, played on "DefaultSlot" while the crouch key is held (the
     *  capsule already shrinks via ACharacter::Crouch). Full-body for now; a layered
     *  crouch-walk blend can replace it later. Soft ref → no-op until the clip exists. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Anim")
    TSoftObjectPtr<UAnimSequence> CrouchPoseAnim;

    /** Looping swim pose, played on "DefaultSlot" while the movement mode is Swimming
     *  (needs a water volume in the level to trigger). Soft ref → no-op until present. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Anim")
    TSoftObjectPtr<UAnimSequence> SwimPoseAnim;

    // --- Spawn safety net (stopgap until the level PlayerStart is moved into the
    //     city; self-disables once it is) -----------------------------------------

    /** If the pawn spawns far east of the city (X >= FarSpawnXThreshold — where the
     *  stray PlayerStarts currently sit, near X 0..150), relocate it into the built
     *  city on BeginPlay so the player lands among the buildings instead of empty
     *  beach/terrain. Self-disabling: once a PlayerStart is placed in the city
     *  (X < threshold) this never fires. A safety net, NOT the canonical fix — that
     *  is moving the PlayerStart in-editor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Player|Spawn")
    bool bRelocateIfSpawnedFarFromCity = true;

    /** Where the spawn-fallback drops the player (world cm). Spawned slightly high so
     *  gravity settles the capsule onto whatever ground streams in below. Tune in the
     *  BP if the city layout shifts. Default sits just in front of Building 01. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Player|Spawn")
    FVector CityFallbackSpawn = FVector(-1300.0, 650.0, 600.0);

    /** Spawn X at or beyond this (cm) counts as "far east of the city" and triggers
     *  the fallback. City mass is at negative X (~-1700..-7000); stray starts ~X 0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Player|Spawn")
    double FarSpawnXThreshold = -1000.0;

    // --- Input handlers --------------------------------------------------------

    void HandleMove(const FInputActionValue& Value);
    void HandleLook(const FInputActionValue& Value);
    void HandleJumpStarted(const FInputActionValue& Value);
    void HandleJumpCompleted(const FInputActionValue& Value);
    void HandleInteract(const FInputActionValue& Value);
    void HandleFireStarted(const FInputActionValue& Value);
    void HandleFireCompleted(const FInputActionValue& Value);
    void HandleReload(const FInputActionValue& Value);
    void HandleSwitchWeapon(const FInputActionValue& Value);
    void HandleCrouchStarted(const FInputActionValue& Value);
    void HandleCrouchCompleted(const FInputActionValue& Value);
    void HandleSprintStarted(const FInputActionValue& Value);
    void HandleSprintCompleted(const FInputActionValue& Value);
    void HandleAimStarted(const FInputActionValue& Value);
    void HandleAimCompleted(const FInputActionValue& Value);

    /** Recompute the body-orientation posture from the current aim sources. While
     *  aiming (right-mouse held) or firing, the body yaw is locked to the controller
     *  (camera/mouse) so the character faces where it shoots; otherwise the body
     *  orients to its movement direction (free run). */
    void RefreshAimPosture();

    /** True while the dedicated aim button (right-mouse / left-trigger) is held. */
    bool bAimButtonHeld = false;

    /** True while the fire button is held (firing also pulls the body to aim). */
    bool bFireHeld = false;

private:
    /** Push the current throwable ammo up to the GameInstance (so it persists across
     *  travel + into a save) and fire OnThrowablesChanged for the HUD. Called wherever
     *  ammo changes (a throw or a refill). */
    void SyncThrowablesChanged();

    /** Re-apply the GameInstance's persistent player state (look + ammo + transform) to
     *  this pawn. Used on spawn (BeginPlay) and after a mid-play load. Returns true if a
     *  saved transform was applied (so BeginPlay can skip the spawn-relocation net). */
    bool RestorePersistentState();

    /** Load + apply the configured body/face/anim (guarded; absent assets are a
     *  no-op so the headless build and an un-authored BP both still run). Prefers
     *  the AppearanceSet pools (indexed by Look), falling back to the single soft
     *  refs per slot when the wardrobe is absent or a pool is empty. */
    void ApplyAppearance();

    /** Push the chosen skin tone into the body + head materials' colour params
     *  (SkinTone / Tint / BaseColor / Color), mirroring how AGTCCitizen tints. */
    void ApplySkinTint(const FLinearColor& Tone);

    /** Number of options in a slot's pool, or 0 when unknown (no wardrobe / empty). */
    int32 GetAppearanceSlotCount(int32 Slot) const;

    /** Pointer to the Look field backing a slot id, or null for an invalid slot. */
    int32* SlotValuePtr(int32 Slot);

    /** Resolve the skin tone colour for an index (wardrobe palette, else built-in). */
    FLinearColor ResolveSkinTone(int32 Index) const;

    /** Play a one-shot AnimSequence on a named slot of the body's anim graph as a
     *  dynamic montage. SlotName defaults to "DefaultSlot" (the slot the ABP already
     *  exposes); pass "UpperBody" once the layered-blend slot is authored so the clip
     *  plays on the torso while the legs keep their locomotion pose. No-op if the
     *  sequence, anim instance, or slot is missing, so an un-authored anim or slot
     *  never breaks the build. */
    void PlayCombatAnim(const TSoftObjectPtr<UAnimSequence>& Anim,
                        FName SlotName = FName(TEXT("DefaultSlot")), float BlendTime = 0.1f);

    /** Start a LOOPING full-body pose on "DefaultSlot" for a held state (crouch/swim),
     *  stopping any previous one. Tracks the montage so StopLoopingPose can end it.
     *  No-op if the clip or anim instance is missing. */
    void StartLoopingPose(const TSoftObjectPtr<UAnimSequence>& Anim);

    /** Stop the active looping held-state pose (crouch release / leaving water). */
    void StopLoopingPose();

    /** The montage backing the currently-held looping pose (crouch/swim), so it can be
     *  stopped when the held state ends. Null when no held pose is active. */
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveLoopingPose;

    /** True while the sprint key is held (start/stop kept symmetric). */
    bool bIsSprinting = false;

    /** MaxWalkSpeed captured at sprint start, restored verbatim on sprint stop —
     *  no dependency on BeginPlay timing or a non-zero baseline. */
    float PreSprintSpeed = 0.0f;

    /** Sprint multiplies the current walk speed (blendspace then shows the run pose).
     *  At the 200 cm/s base this reaches ~450 cm/s, a real run inside the NPC crowd's
     *  run band (RunSpeed ~380-480), so holding Shift visibly shifts walk -> run. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Player|Movement")
    float SprintSpeedMultiplier = 2.25f;

    /** The player's chosen appearance, restored from the GameInstance on spawn. */
    UPROPERTY()
    FGTCPlayerLook Look;
};
