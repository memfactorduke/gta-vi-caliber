// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Archetypes/NpcArchetypes.h"
#include "../Decision/NpcNeeds.h"
#include "../Decision/NpcMind.h"
#include "../Decision/NpcBrain.h"
#include "../Decision/NpcContact.h"
#include "../Decision/NpcAcquaintance.h"
#include "../Voice/NpcVoice.h"
#include "../Vitals/NpcVitals.h"
#include "GTCCitizen.generated.h"

class UGTCCrowdSubsystem;
class UGTCPlaceRegistrySubsystem;
class USkeletalMeshComponent;
class UGTCAppearanceSet;
class UGTCVoiceComponent;
struct FNpcCitizenRecord;

/**
 * Blueprint-facing mirror of the pure-core ENpcContactReaction, so an Anim
 * Blueprint / montage layer can Switch on the reaction the C++ chose and play
 * matching animation (stumble, square-up, recoil, fall...). Kept value-for-value
 * in lockstep with the pure-core enum by static_assert in the .cpp — the model
 * stays scene-free; this is the only reflected copy.
 */
UENUM(BlueprintType)
enum class EGTCContactReaction : uint8
{
    Ignore,
    Excuse,
    Annoyed,
    Flinch,
    Confront,
    Flee,
    Knockdown,
};

/**
 * Blueprint-facing mirror of the pure-core FNpcVoiceProfile (NPC/Voice/NpcVoice.h)
 * — a citizen's vocal signature, exposed so a MetaHuman/Audio2Face component, a
 * pitch-shifted SoundCue, or a TTS bridge can set the voice's pitch/rate/timbre.
 * Populated once at identity time and handed to Blueprints via OnVoiceAssigned;
 * the model stays scene-free, this is the only reflected copy.
 */
USTRUCT(BlueprintType)
struct FGTCVoiceProfile
{
    GENERATED_BODY()

    /** Median speaking fundamental (Hz). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|NPC|Voice")
    float BasePitchHz = 140.0f;

    /** Half-width of natural pitch movement, in semitones. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|NPC|Voice")
    float PitchJitterSemitones = 2.0f;

    /** Delivery tempo (syllables per second). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|NPC|Voice")
    float RateSyllablesPerSec = 4.2f;

    /** Airy/whispery weight, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|NPC|Voice")
    float Breathiness = 0.2f;

    /** Gravel/creak weight, 0..1. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|NPC|Voice")
    float Roughness = 0.15f;

    /** Vocal-tract size proxy that scales formants (<1 brighter, >1 darker). */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|NPC|Voice")
    float FormantScale = 1.0f;

    /** True for a silent citizen (e.g. the mime): play no audio, still drive timing. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|NPC|Voice")
    bool bMute = false;

    FGTCVoiceProfile() = default;

    /** Lift a pure-core profile into the reflected mirror. */
    explicit FGTCVoiceProfile(const FNpcVoiceProfile& In)
        : BasePitchHz(In.BasePitchHz)
        , PitchJitterSemitones(In.PitchJitterSemitones)
        , RateSyllablesPerSec(In.RateSyllablesPerSec)
        , Breathiness(In.Breathiness)
        , Roughness(In.Roughness)
        , FormantScale(In.FormantScale)
        , bMute(In.bMute)
    {
    }
};

/**
 * AGTCCitizen — a living pedestrian. THIS is the "Wave-3 adapter" that every
 * pure-core NPC header pointed at and deferred: the actor that owns the mutable
 * state and, every frame, runs the whole tested decision stack and walks the
 * result out into the world through CharacterMovement.
 *
 * One citizen is one person with a day. Its identity is drawn deterministically
 * from a spawn seed (FNpcArchetypes::Pick) so the same slot is always the same
 * character across reloads: a schedule, a discipline, a voice, a tint, a quirk.
 * On top of that scripted routine ride live drives (FNpcNeeds) that decay hour by
 * hour; when one boils over, FNpcMind hijacks the plan and sends the citizen to
 * eat / sleep / unwind. Reaching the destination satisfies the drive, and the
 * routine resumes — the loop that makes a crowd read as inhabited rather than
 * looping idle anims.
 *
 * Per frame the pipeline is:
 *   clock -> decay needs -> (re)decide intent (FNpcMind) -> resolve the named
 *   place to a world point (UGTCPlaceRegistrySubsystem) -> read the player and
 *   react (FNpcReaction + FNpcMemory) -> brain FSM (FNpcBrain::NextState) ->
 *   desired ground velocity (FNpcLocomotion, fusing seek/arrive + separation +
 *   traffic dodge) -> movement input. The body turns to face travel
 *   (bOrientRotationToMovement), so it looks like someone walking somewhere on
 *   purpose, dodging cars and giving other pedestrians space.
 *
 * The actor OWNS its mutable state and reads shared context (the time of day, the
 * neighbour list, the player threat snapshot) from UGTCCrowdSubsystem; it queries
 * UGTCPlaceRegistrySubsystem for destinations. It depends on neither sibling at
 * the header level — both are forward-declared and resolved in the .cpp — so this
 * stays a thin engine shell over the scene-free pure-core models.
 */
UCLASS()
class GTC_UE5_API AGTCCitizen : public ACharacter
{
    GENERATED_BODY()

public:
    AGTCCitizen();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    /**
     * Stamp this citizen's identity from a deterministic seed (archetype, traits,
     * speeds, starting drives, RNG stream). The spawner calls this on the deferred-
     * spawned actor before FinishSpawning; BeginPlay calls it too if it never ran,
     * so a hand-placed citizen still comes alive. Idempotent guard via bInitialized.
     */
    void InitializeFromSeed(int32 Seed);

    /**
     * Stamp this citizen's identity from a persistent roster record (the crowd
     * director embodies a census member this way): same seeded identity as
     * InitializeFromSeed, but the record's CARRIED drives and current intent are
     * restored so the person resumes the day it was living off-screen. The stored
     * StableId binds the body back to the record so EndPlay writes its state home.
     */
    void InitializeFromRecord(const FNpcCitizenRecord& Record);

    /** The roster member this body embodies, or INDEX_NONE for a hand-placed /
     *  non-persistent citizen. */
    int32 GetStableId() const { return StableId; }

    /**
     * Promote/demote this citizen's face LOD: high detail swaps the head to the
     * appearance set's hero (MetaHuman) head, low returns it to the crowd head. The
     * crowd director calls this by distance/contact so only the few faces the player
     * can actually see pay the rigged-head cost. No-op without a hero pool, or when
     * the tier is unchanged. */
    void SetFaceHighDetail(bool bHighDetail);

    bool IsFaceHighDetail() const { return bFaceHighDetail; }

    /** The full intent (activity/place/reason/urgency) — for roster write-back. */
    const FNpcIntent& GetCurrentIntent() const { return CurrentIntent; }

    /**
     * Burn in a memory of player mayhem this citizen just witnessed (0..1). The
     * wanted/crime layer drives this; it makes the citizen recognise and call out
     * the culprit while the memory lasts (FNpcMemory), then fade.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    void WitnessPlayerMayhem(double Intensity);

    // --- Read-only introspection (debug HUD / dialogue / save) ------------------

    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    FString GetArchetypeName() const { return Archetype.Name; }

    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    FString GetCurrentActivity() const { return CurrentIntent.Activity; }

    /** "schedule" or "need:<drive>" — why the citizen is doing what it's doing. */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    FString GetIntentReason() const { return CurrentIntent.Reason; }

    /** 0 Idle / 1 Wander / 2 Flee — the locomotion state, for debug overlays. */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    int32 GetBrainState() const { return static_cast<int32>(BrainState); }

    int32 GetSeed() const { return CitizenSeed; }
    const FNpcNeeds& GetNeeds() const { return Needs; }
    double GetMemoryIntensity() const { return MemoryIntensity; }

    // --- Social (group conversations) -------------------------------------------

    /** True when this citizen is loitering, calm, and free to start a chat. */
    bool IsAvailableToChat() const;

    /** Begin a conversation with Partner (the crowd pairs both sides). */
    void BeginChatWith(AGTCCitizen* Partner);

    /** End the current conversation (timeout, partner gone, or a threat). */
    void EndChat();

    bool IsChatting() const { return bChatting; }

    /** True if this citizen recognises `Other` (a remembered acquaintance, not a
     *  stranger). Used by the crowd to find a familiar face passing by, so friends
     *  wave/greet in the street. Keyed the same way meetings are recorded. */
    bool Recognizes(const AGTCCitizen* Other) const;

    // --- Physical contact (the player bumped / shoved / struck this citizen) -----

    /**
     * Register a deliberate blow on this citizen from the player — a punch, a
     * melee swing, a weapon bash, a car clip. The combat/vehicle layer calls this
     * the way the crime layer calls WitnessPlayerMayhem. `ImpactSpeedMetersPerSec`
     * is the fist/weapon/vehicle closing speed (drives how hard the hit lands),
     * `ImpactDirection` is the world push direction (player -> citizen); pass zero
     * to have it derived from the player's position. Body bumps are detected
     * automatically each tick and need no caller.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    void ApplyPlayerImpact(float ImpactSpeedMetersPerSec, bool bStrike, FVector ImpactDirection);

    // --- Gunfire & lethal damage -------------------------------------------------

    /** True once killed — the body is ragdolling and counting down to despawn. */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC|Combat")
    bool IsDead() const { return bDead; }

    /** Current health as a 0..1 fraction (1 = unhurt), for a HUD/debug overlay. */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC|Combat")
    float GetHealthFraction() const;

    /**
     * Engine damage entry point. The player's weapon (UGTCWeaponComponent) lands a
     * hitscan and calls UGameplayStatics::ApplyPointDamage, which routes here: draw
     * down the citizen's vitals, then either play a wound reaction or die. Honours
     * the standard TakeDamage contract (returns the damage actually taken).
     */
    virtual float TakeDamage(
        float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    /**
     * Fired when a non-lethal round lands — the seam for a pain/recoil/stumble
     * montage. `Severity` is 0..1 (how hard the round hit relative to toughness);
     * `FromDirection` is the planar world direction the shot came from. The C++ has
     * already staggered the body, raised heat, and set the panic going; the montage
     * is the only thing left, so leaving this unbound is harmless.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Combat")
    void OnGunshotWound(float Severity, FVector FromDirection);

    /**
     * Fired once when this citizen is killed — the seam for a death montage or blood
     * FX before the body ragdolls. `FromDirection` is the planar world direction the
     * killing shot came from. The C++ has already entered the death state (ragdoll,
     * AI stop, despawn timer, max heat); leaving this unbound is harmless.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Combat")
    void OnKilled(FVector FromDirection);

    /**
     * Fired the instant a contact reaction is chosen, for an Anim Blueprint to play
     * a matching montage (stumble, square-up, recoil, fall, get-up...). The C++
     * already drives the gameplay consequence (face/halt/knockback/flee/memory);
     * this is purely the animation hook, so leaving it unbound still "works".
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Reaction")
    void OnContactReaction(EGTCContactReaction Reaction, float Severity, FVector ImpactDirection);

    /**
     * Fired the instant this citizen voices ANY line (ambient bark, contact retort,
     * pass-by greeting, idle mutter, chatter) — the lip-sync seam. An Anim Blueprint
     * or a MetaHuman/Audio2Face audio component binds this to drive jaw/viseme motion
     * on the HeadMesh for `EstimatedSeconds`, so faces move when people speak. The
     * C++ has already chosen and logged the line; leaving this unbound is harmless.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Voice")
    void OnSpeak(const FString& Line, float EstimatedSeconds);

    /**
     * Fired once at identity time with this citizen's fixed vocal signature, so a
     * MetaHuman/Audio2Face component, a pitch-shifted SoundCue, or a TTS bridge can
     * configure the voice (pitch/rate/timbre) for the whole session. Deterministic
     * from the spawn seed, so the same person always sounds the same; leaving it
     * unbound is harmless (the C++ still tracks the profile for SpeakLine pacing).
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Voice")
    void OnVoiceAssigned(const FGTCVoiceProfile& Profile);

    /** This citizen's vocal signature (pitch/rate/timbre), for audio/HUD/debug. */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC|Voice")
    FGTCVoiceProfile GetVoiceProfile() const { return FGTCVoiceProfile(VoiceProfile); }

    /** The contact reaction currently playing (Ignore when none), for debug/HUD. */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    EGTCContactReaction GetActiveContactReaction() const;

    /**
     * React to the current sky. `Severity` is the weather severity index (0 clear
     * .. 4 storm — FWeatherSystem::SeverityIndex). The C++ decides the behavioural
     * response (FNpcWeatherReaction) from severity + the clock hour + this citizen's
     * bravery, may voice a weather grumble, and fires OnWeatherResponse for the anim/
     * behaviour layer (hunch, umbrella, hurry, head for cover). Call this from the
     * weather controller / a crowd broadcast as conditions change; safe to call
     * repeatedly. Off-census or unbound, it's harmless.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC|Weather")
    void ReactToWeather(int32 Severity);

    /**
     * Fired by ReactToWeather with the chosen response: whether to head for cover,
     * pick up the pace, and how unpleasant it feels (0..1). The anim/behaviour layer
     * binds this to hunch posture, raise an umbrella, speed the walk, or path to
     * shelter. The C++ has already voiced any grumble; leaving this unbound is fine.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Weather")
    void OnWeatherResponse(bool bSeekShelter, bool bHurry, float Discomfort);

    /**
     * Fired when this citizen resolves a destination of `PlaceKind`, reporting how
     * busy it is: `Busyness` is the ENpcBusyness tier (0 Empty, 1 Quiet, 2 Busy,
     * 3 Packed) from the place's occupancy vs capacity. The anim/behaviour layer can
     * react — hang back at a packed spot, brighten in a lively one. The C++ has
     * already chosen the destination (the registry spreads crowds); leaving this
     * unbound is harmless.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Crowd")
    void OnPlaceBusyness(FName PlaceKind, uint8 Busyness);

    /**
     * Fired once on the rising edge of a traffic near-miss (a car closing fast and
     * near, past TrafficStartleThreshold): the seam for a flinch/jump-back montage.
     * `ThreatDirection` is the planar world direction to the car. The C++ already
     * halts at the kerb and yelps; this is the animation hook. Unbound is fine.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Traffic")
    void OnTrafficStartle(FVector ThreatDirection);

    /**
     * Fired when this citizen, walking past, spots someone it knows and waves — the
     * seam for a wave/nod montage so friendships are visible in the street (no stop,
     * just a greeting in passing). `Direction` is the planar world direction to the
     * acquaintance. The C++ already voices a greeting; unbound is fine.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Social")
    void OnWaveTo(FVector Direction);

    /**
     * Fired when a citizen, walking somewhere, takes a brief curious glance at the
     * world around it (a shop window, a passer-by) — the seam for a head-turn so
     * people don't beeline like automatons. `Direction` is a planar world direction
     * to look. It keeps walking; this is just the look. Unbound is fine.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Idle")
    void OnCuriousGlance(FVector Direction);

    /**
     * Fired when the player passes close by without contact and this citizen, going
     * calmly about its day, clocks them — the seam for a head-look/eye-track or a
     * nod montage so the crowd visibly registers the player walking through. The
     * C++ side keeps giving the player room (separation) and may mutter a greeting;
     * this is the look-at hook. `PlayerDirection` is the planar world direction to
     * the player; leaving it unbound is fine.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Reaction")
    void OnPlayerNoticed(FVector PlayerDirection);

    /**
     * Fired when a lingering citizen performs a contextual idle action — the seam
     * for an Anim Blueprint to play the matching montage/additive (check phone,
     * smoke, people-watch, check watch, stretch...). `Action` is a stable FName
     * token from FNpcIdleAction keyed to what the citizen is doing. Purely the
     * animation hook; the C++ just chose the action. Leaving it unbound is fine.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Reaction")
    void OnIdleAction(FName Action);

    /**
     * Fired repeatedly while a brave citizen is squaring up to the player (the
     * Confront reaction) and throwing a retaliatory shove — the seam for a
     * shove/swing montage. `TowardPlayer` is the planar world direction at the
     * player. The C++ drives a short forward lunge and the angry voice; whether the
     * shove actually knocks the player back is the montage/combat layer's call.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Reaction")
    void OnConfrontShove(FVector TowardPlayer);

    /**
     * Fired once when a citizen freezes at gunpoint (the Cower duress response) — the
     * seam for a hands-up / surrender montage. `TowardThreat` is the planar world
     * direction at the armed player; the citizen holds the pose (and faces the gun)
     * while IsCowering() stays true. The C++ drives the freeze + facing + fearful voice.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|NPC|Reaction")
    void OnCower(FVector TowardThreat);

    /** True while frozen at gunpoint (hands up). Drives a held cower pose / HUD. */
    UFUNCTION(BlueprintCallable, Category = "GTC|NPC")
    bool IsCowering() const { return bCowering; }

protected:
    // --- Tuning (per-archetype variance is layered on top in InitializeFromSeed) -

    /** Game-time speed: how many in-game hours pass per real second when no crowd
     *  clock is present (the crowd subsystem normally owns the master clock). */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Time")
    double FallbackHoursPerSecond = 24.0 / (20.0 * 60.0); // a 20-minute day.

    /** Seconds between intent re-evaluations (drives change slowly; no need to
     *  re-plan every frame). Arrival also forces an immediate re-plan. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Decision")
    double ReplanIntervalSec = 1.5;

    /** Planar distance (cm) at which the citizen counts as "arrived" at its goal. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Decision")
    double ArriveToleranceCm = 90.0;

    /** Brain flee/calm radii (cm) fed to FNpcBrain::NextState hysteresis. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Reaction")
    double FleeRadiusCm = 600.0;

    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Reaction")
    double CalmRadiusCm = 1100.0;

    /** How far (cm) the transient flee goal is planted ahead of the citizen. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Reaction")
    double FleeGoalDistanceCm = 1500.0;

    /** Curb safety: a car passing within this radius (cm) of the citizen's spot
     *  within SafeGapSec makes it halt and wait rather than step into traffic. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Traffic")
    double CurbDangerRadiusCm = 300.0;

    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Traffic")
    double CurbSafeGapSec = 1.8;

    /** Threat (0..1, FPedestrianTraffic::NearestThreat) at/above which a near-miss
     *  makes the citizen flinch and shout, not just quietly halt. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Traffic", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double TrafficStartleThreshold = 0.6;

    /** Bark cooldown (s) so a citizen doesn't chatter every frame. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Voice")
    double BarkCooldownSec = 6.0;

    /** Two loitering citizens within this range (cm) may strike up a chat. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Social")
    double ChatStartRadiusCm = 220.0;

    /** Radius (cm) within which a passing acquaintance gets a wave/greeting. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Social")
    double WaveRadiusCm = 500.0;

    /** Minimum seconds between street waves, so friends don't wave every frame. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Social")
    double WaveCooldownSec = 6.0;

    /** How long (s) a chat lasts before the pair drifts apart. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Social")
    double ChatDurationSec = 12.0;

    /** Seconds between conversational lines while chatting. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Social")
    double ChatLineIntervalSec = 3.0;

    /** While loitering, re-target a nearby spot this often (s) so the NPC mills
     *  about the POI instead of standing frozen. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Idle")
    double IdleWanderIntervalSec = 3.5;

    /** Radius (cm) of the loiter shuffle around the POI anchor. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Idle")
    double IdleWanderRadiusCm = 350.0;

    /** Chance, each loiter cycle, that the citizen stands and performs a contextual
     *  idle action (check phone, smoke, people-watch...) instead of drifting to a
     *  new spot. The rest of the time it mills as before. At exactly 1.0 the citizen
     *  always performs actions in place and never drifts — keep below 1.0 for milling. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Idle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double IdleActionChance = 0.5;

    /** Chance that a (talkative) idle action is accompanied by a quiet mutter. Kept
     *  modest so loiterers murmur occasionally, not every fidget. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Idle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double IdleBarkChance = 0.4;

    /** A player this close (cm) counts as touching the citizen — roughly the sum
     *  of the two capsule radii plus a little margin. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Contact")
    double ContactRadiusCm = 95.0;

    /** The player must be closing on the citizen faster than this (cm/s) for a
     *  proximity to count as a real bump — so brushing past or standing beside
     *  them doesn't keep triggering. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Contact")
    double ContactMinApproachCmS = 60.0;

    /** Minimum gap (s) between contact reactions, so one shove is one reaction. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Contact")
    double ContactCooldownSec = 1.0;

    /** While squaring up (Confront), seconds between retaliatory shoves. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Contact")
    double ConfrontShoveIntervalSec = 1.0;

    /** Forward lunge speed (cm/s) of a retaliatory shove — a committed step-in at the player. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Contact")
    double ConfrontLungeSpeed = 220.0;

    /** The player passing within this radius (cm) — beyond touching distance — is
     *  "right there": the citizen gives way and may glance/greet. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Contact")
    double PassByRadiusCm = 320.0;

    /** Minimum gap (s) between pass-by acknowledgements, so a citizen doesn't greet
     *  the player every frame they linger nearby. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Contact")
    double PassByCooldownSec = 9.0;

    // --- Combat tuning ----------------------------------------------------------

    /** Starting/maximum health. Tougher archetypes (a cop) can be seeded higher. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Combat")
    double CitizenMaxHealth = 100.0;

    /** Wound severity (0..1, FNpcVitals::WoundSeverity) at/above which a non-lethal
     *  round floors the citizen (Knockdown) instead of a Flinch recoil. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double KnockdownWoundSeverity = 0.5;

    /** Seconds a killed body lingers, ragdolled, before it despawns. */
    UPROPERTY(EditAnywhere, Category = "GTC|NPC|Combat")
    double CorpseLingerSeconds = 12.0;

private:
    // --- Identity (seeded, immutable after init) --------------------------------

    UPROPERTY(EditAnywhere, Category = "GTC|NPC")
    int32 CitizenSeed = 0;

    /** The persistent roster member this body embodies (INDEX_NONE if unbound). */
    int32 StableId = INDEX_NONE;

    /** Separate head/face mesh, attached to the body and following its pose. Empty
     *  until a head asset (modular or MetaHuman) is supplied for the citizen's
     *  HeadVariant — the slot that turns the crowd into distinct human faces. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|NPC|Appearance", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> HeadMesh;

    /** Modular hair, riding the body as a leader-pose follower. Empty until an
     *  appearance set supplies hair meshes for the citizen's HairVariant. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|NPC|Appearance", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> HairMesh;

    /** Modular outfit/clothing, riding the body as a leader-pose follower. Empty
     *  until an appearance set supplies outfit meshes for the citizen's OutfitVariant. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|NPC|Appearance", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> OutfitMesh;

    /** The citizen's audible voice: synthesises + plays a stylised vocalisation in
     *  this person's register whenever they speak (SpeakLine). Configured from the
     *  seeded VoiceProfile in ApplyIdentity; silent for a mute (mime) profile. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|NPC|Voice", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGTCVoiceComponent> VoiceComp;

    /** Data-authored wardrobe (body/head/skin pools). When unset, the citizen falls
     *  back to the stock mannequin + head paths baked into the .cpp. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GTC|NPC|Appearance", meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<UGTCAppearanceSet> AppearanceSet;

    /** Seed-derived face index, stored so the face-LOD swap can re-pick the matching
     *  crowd/hero head for this same person. */
    int32 HeadVariant = 0;

    /** Seed-derived hair index. */
    int32 HairVariant = 0;

    /** Seed-derived outfit index. */
    int32 OutfitVariant = 0;

    /** True while wearing the high-detail (hero/MetaHuman) head. */
    bool bFaceHighDetail = false;

    FNpcArchetype Archetype;
    double Discipline = 0.0;
    double Bravery = 0.5;
    double Curiosity = 0.5;
    double WalkSpeed = 150.0; // cm/s
    double RunSpeed = 420.0;  // cm/s
    TMap<FString, double> DecayRates;
    FRandomStream Rng;

    // --- Live mutable state -----------------------------------------------------

    FNpcNeeds Needs;
    FNpcIntent CurrentIntent;
    FNpcBrain::EState BrainState = FNpcBrain::EState::Idle;

    FVector CurrentGoal = FVector::ZeroVector;
    /** The POI the goal was resolved to — the centre micro-wander shuffles around. */
    FVector GoalAnchor = FVector::ZeroVector;
    bool bHasGoal = false;
    bool bGoalSynthesized = false;
    /** The place kind currently claimed for occupancy (released on change/death). */
    FName ClaimedPlaceKind = NAME_None;
    FVector ClaimedPlaceLocation = FVector::ZeroVector;
    bool bHasClaim = false;

    double MemoryIntensity = 0.0;

    // --- Vitals (gunfire / death) -----------------------------------------------
    /** This citizen's life pool; seeded from CitizenMaxHealth in ApplyIdentity. */
    FNpcVitals Vitals;
    /** True once killed — Tick early-outs and EndPlay skips the resume write-back. */
    bool bDead = false;

    // --- Social conversation state ----------------------------------------------
    TWeakObjectPtr<AGTCCitizen> ChatPartner;
    bool bChatting = false;
    double ChatTimer = 0.0;     // total time in the current chat
    double ChatLineTimer = 0.0; // time since the last conversational line
    int32 LastChatTurn = -1;    // last conversation turn this side voiced (floor turn-taking)
    double WaveCooldown = 0.0;  // time left before this citizen will wave at a friend again
    double GlanceTimer = 0.0;   // time until the next curious-glance opportunity
    int32 GlanceCounter = 0;    // varies the per-glance roll/seed, off the wander stream
    bool bChatPartnerFamiliar = false; // current chat partner is a recognised face (warm opener)

    /** Who this citizen has come to recognise — a small, decaying social memory so
     *  regulars greet each other warmly and strangers get the wary opener. Updated
     *  on each chat (Meet), keyed by the partner's stable id. Live-only for now
     *  (resets on respawn); persisting it across the census is a later concern. */
    FNpcAcquaintance Acquaintances;

    // --- Physical-contact reaction state ----------------------------------------
    /** The reaction currently overriding the baseline; Ignore when none active. */
    ENpcContactReaction ContactReaction = ENpcContactReaction::Ignore;
    /** Seconds left in the active reaction before the citizen resumes its day. */
    double ReactionTimer = 0.0;
    /** Counts down between retaliatory shoves while squaring up (Confront). */
    double ConfrontShoveTimer = 0.0;
    /** Debounce so one bump produces one reaction, not one per frame. */
    double ContactCooldown = 0.0;
    /** Severity (0..1) of the contact that started the active reaction. */
    double ContactSeverity = 0.0;
    /** Planar world push direction of the last contact (player -> citizen). */
    FVector ContactPushDir = FVector::ZeroVector;
    /** Where the player was at the moment of contact — the flee-from / face-at point. */
    FVector ContactThreatPos = FVector::ZeroVector;
    /** Debounce between pass-by acknowledgements (counts down each tick). */
    double PassByCooldown = 0.0;
    /** Dedicated counter for the pass-by greeting roll, so deciding whether to greet
     *  never perturbs the identity/wander RNG stream (keeps wander a pure function of
     *  seed + time, independent of where the player walks). */
    int32 PassByCounter = 0;

    // --- Navigation path (when a NavMesh exists; else direct steering) ----------
    /** Waypoints from the nav system to the current goal; empty => steer direct. */
    TArray<FVector> PathPoints;
    int32 PathIndex = 0;
    bool bUsePath = false;
    /** True while gawking — stopped, turned to face the player. */
    bool bGawking = false;
    /** True while cowering at gunpoint — frozen, hands up, facing the threat. */
    bool bCowering = false;
    /** Previous-frame cower state, so OnCower fires once on the rising edge. */
    bool bWasCowering = false;

    /** Previous-frame traffic-startle state, so OnTrafficStartle + the yelp fire once
     *  on the rising edge of a near-miss (not every frame the car is close). */
    bool bWasTrafficStartled = false;

    double ReplanAccum = 0.0;
    double IdleWanderAccum = 0.0;
    /** Counter for the idle-action roll, independent of the wander RNG stream. */
    int32 IdleActionCounter = 0;
    double TimeSinceBark = 1000.0;
    double FallbackHour = 8.0;
    int32 BarkCounter = 0;
    /** Monotonic per-utterance counter so each spoken line gets a distinct voice
     *  seed (pitch contour / breath), independent of the bark cadence counter. */
    int32 SpeakCounter = 0;
    FString LastBark;

    /** This citizen's vocal signature, derived once from its seed+voice persona in
     *  ApplyIdentity and used to pace spoken lines (SpeakLine) and configure audio. */
    FNpcVoiceProfile VoiceProfile;

    bool bInitialized = false;
    /** True once arrived at the current goal and performing the activity. */
    bool bArrived = false;

    /** Apply the seeded identity (archetype, traits, decay rates, body/mesh) shared
     *  by both initialize entry points. Does not touch live drives/intent. */
    void ApplyIdentity(const FNpcCitizenRecord& Record);

    // --- Pipeline steps ---------------------------------------------------------

    /**
     * Resolve the master hour-of-day and the game-hours elapsed this frame. Uses
     * the crowd subsystem's shared clock when present, else advances an internal
     * clock at FallbackHoursPerSecond so a lone citizen still lives a day.
     */
    double ResolveHour(float DeltaSeconds, UGTCCrowdSubsystem* Crowd, double& OutElapsedHours);

    /** Re-evaluate intent and resolve its place to a world goal. */
    void Replan(double Hour, UGTCPlaceRegistrySubsystem* Places);

    /** Ask the nav system for a path to CurrentGoal; falls back to direct steering. */
    void RecomputePath();

    /** The point to steer at this frame: the next path waypoint, or the goal. */
    FVector CurrentSteerGoal();

    /** Top up the drive the just-reached activity serves. */
    void SatisfyArrivalNeed();

    /** Decide the brain state from the player threat / panic, returning the threat
     *  position for flee steering. Out param bThreat tells the caller a threat is live. */
    FNpcBrain::EState DecideBrainState(
        UGTCCrowdSubsystem* Crowd, bool& bThreatActive, FVector& OutThreatPos);

    /** Compose + apply this frame's movement. */
    void DriveLocomotion(float DeltaSeconds, UGTCCrowdSubsystem* Crowd, bool bThreatActive, const FVector& ThreatPos);

    /** Maybe say something, gated by cooldown + situation. */
    void MaybeBark(float DeltaSeconds, bool bThreatActive);

    /** Record + announce a spoken line: stores it as LastBark and fires OnSpeak (the
     *  lip-sync hook) with an estimated duration. Does NOT touch the bark cadence
     *  timer — callers own TimeSinceBark so existing pacing is unchanged. */
    void SpeakLine(const FString& Line);

    /** Release any occupancy claim (on goal change / despawn). */
    void ReleaseClaim();

    /** Tick the contact cooldown/timer and sniff for a fresh body bump this frame. */
    void UpdateContactReaction(float DeltaSeconds, UGTCCrowdSubsystem* Crowd);

    /** Proximity + approach test: is the player barging into this citizen right now? */
    void DetectBodyContact(UGTCCrowdSubsystem* Crowd);

    /** Resolve a contact into a reaction and apply its consequences (knockback,
     *  memory, voice, animation hook). Debounced by ContactCooldown. */
    void ApplyContact(double ImpactSpeedMps, bool bStrike, const FVector& PushDir, const FVector& ThreatPos);

    /** Route a resolved gunshot: draw down vitals, then wound-react or die.
     *  `BulletTravel` is the planar world direction the round was travelling. */
    void ApplyGunshot(double Damage, const FVector& BulletTravel);

    /** Enter the death state: end social ties, stop the AI/brain, ragdoll the body,
     *  start the despawn countdown. `BulletTravel` shoves the corpse off the shot. */
    void EnterDeath(const FVector& BulletTravel);

    /** Voice the moment of contact with a line that fits the reaction. */
    void BarkForContact(ENpcContactReaction Reaction);

    /** A calm citizen clocks the player passing close: glance hook + maybe a greeting. */
    void MaybeNoticePlayer(float DeltaSeconds, UGTCCrowdSubsystem* Crowd);

    /** Wave + greet an acquaintance passing nearby (calm, walking, off cooldown), so
     *  friendships show in ambient street life without stopping for a full chat. */
    void MaybeGreetAcquaintance(float DeltaSeconds, UGTCCrowdSubsystem* Crowd);

    /** Occasionally take a curious glance while walking somewhere (head-turn + rare
     *  murmur), so citizens read as looking around, not beelining. Curiosity-driven,
     *  self-throttled; doesn't interrupt travel. */
    void MaybeGlance(float DeltaSeconds);
};
