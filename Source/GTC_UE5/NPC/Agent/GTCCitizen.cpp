// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCCitizen.h"

#include "GTCCrowdSubsystem.h"
#include "../Population/NpcPopulation.h"
#include "../Appearance/GTCAppearanceSet.h"
#include "../../World/Places/GTCPlaceRegistrySubsystem.h"
#include "../Locomotion/NpcLocomotion.h"
#include "../Decision/NpcReaction.h"
#include "../Decision/NpcMemory.h"
#include "../Decision/NpcIdleAction.h"
#include "../Decision/NpcOccupation.h"
#include "../Decision/NpcWeatherReaction.h"
#include "../Decision/NpcCrowding.h"
#include "../Decision/NpcScheduleJitter.h"
#include "../Decision/NpcDetour.h"
#include "../Decision/NpcDuress.h"
#include "../Dialogue/NpcDialogue.h"
#include "../Dialogue/BarkPool.h"
#include "../Dialogue/ContactBark.h"
#include "../Dialogue/IdleActionBark.h"
#include "../Dialogue/WorkBark.h"
#include "../Dialogue/NpcHail.h"
#include "../Dialogue/TrafficBark.h"
#include "../Dialogue/ChatBark.h"
#include "../Dialogue/ConversationFloor.h"
#include "../Dialogue/NpcConversation.h"
#include "../Dialogue/NpcTopic.h"
#include "../Voice/GTCVoiceComponent.h"

#include "../Steering/NpcSteering.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Math/UnrealMathUtility.h"

namespace
{
    // The project's stock mannequins + their locomotion AnimBlueprint (blends
    // idle/walk/run from speed), soft-referenced by path so the editor-closed
    // headless build never hard-links them. Citizens pick one for variety; the
    // ABP makes them visibly stride, turn, and idle as they move — turning the
    // bare capsule into something that reads as a walking person.
    const TCHAR* MannyMeshPath = TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
    const TCHAR* QuinnMeshPath = TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");
    const TCHAR* LocomotionAbpPath = TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");

    // Candidate head/face meshes, indexed by the citizen's HeadVariant. These are
    // the slots a modular or MetaHuman head drops into: author SK_Head_0N (skinned
    // to the same skeleton as the body so it follows the leader pose) at these
    // paths and the crowd grows distinct faces. Soft-loaded + guarded, so until the
    // assets exist a citizen simply wears the body mesh's built-in head, never a crash.
    const TCHAR* HeadMeshPaths[] = {
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_00.SK_Head_00"),
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_01.SK_Head_01"),
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_02.SK_Head_02"),
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_03.SK_Head_03"),
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_04.SK_Head_04"),
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_05.SK_Head_05"),
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_06.SK_Head_06"),
        TEXT("/Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_07.SK_Head_07"),
    };
}

namespace
{
    // Drive-decay rates and the activity->drive satisfaction map are shared with the
    // off-screen population sim (FNpcPopulation) so an embodied citizen and an
    // unembodied roster record live by the exact same rules. See NpcPopulation.h.

    // Activities where the citizen lingers at a spot and should mill about rather
    // than stand stock-still (vs. "sleep"/"work" which read fine as stationary).
    bool ActivityLingers(const FString& Activity)
    {
        return Activity == TEXT("loiter") || Activity == TEXT("goof_off")
            || Activity == TEXT("socialize") || Activity == TEXT("commute")
            || Activity == TEXT("eat");
    }

    // The Blueprint-facing reaction enum is kept byte-identical to the pure-core
    // one (asserted here), so this cast is a free reinterpret, not a lookup.
    static_assert((uint8)EGTCContactReaction::Ignore    == (uint8)ENpcContactReaction::Ignore,    "contact enum drift");
    static_assert((uint8)EGTCContactReaction::Excuse    == (uint8)ENpcContactReaction::Excuse,    "contact enum drift");
    static_assert((uint8)EGTCContactReaction::Annoyed   == (uint8)ENpcContactReaction::Annoyed,   "contact enum drift");
    static_assert((uint8)EGTCContactReaction::Flinch    == (uint8)ENpcContactReaction::Flinch,    "contact enum drift");
    static_assert((uint8)EGTCContactReaction::Confront  == (uint8)ENpcContactReaction::Confront,  "contact enum drift");
    static_assert((uint8)EGTCContactReaction::Flee      == (uint8)ENpcContactReaction::Flee,      "contact enum drift");
    static_assert((uint8)EGTCContactReaction::Knockdown == (uint8)ENpcContactReaction::Knockdown, "contact enum drift");

    EGTCContactReaction ToBp(ENpcContactReaction R)
    {
        return static_cast<EGTCContactReaction>(static_cast<uint8>(R));
    }
}

AGTCCitizen::AGTCCitizen()
{
    PrimaryActorTick.bCanEverTick = true;

    // A default AI controller possesses each citizen so CharacterMovement consumes
    // AddMovementInput; placed-or-spawned covers both hand-placed and crowd-spawned.
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    // Walk where you're going, face where you walk. The controller does not steer
    // the body — the movement direction does.
    bUseControllerRotationYaw = false;
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = true;
        Move->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
        Move->MaxWalkSpeed = 150.0f;
        Move->MaxAcceleration = 900.0f;
        Move->BrakingDecelerationWalking = 1200.0f;
        // Pedestrians don't need fancy network movement; keep it cheap.
        Move->bUseControllerDesiredRotation = false;
    }

    // Make a citizen shootable. The weapon's hitscan line-traces on ECC_Visibility
    // (UGTCWeaponComponent::TraceChannel), so the body must answer that channel or
    // rounds pass straight through. The capsule blocks it as the always-hit
    // fallback (body shots register everywhere); the skeletal mesh also blocks it
    // with query collision so, where the mesh carries a physics asset, the hit
    // reports a BoneName and headshots resolve. (Editor checklist: confirm the
    // body mesh has a Physics Asset for per-bone headshots.)
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
    if (USkeletalMeshComponent* Body = GetMesh())
    {
        Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }

    // A separate head/face mesh that rides the body and follows its animation (set
    // up as a leader-pose follower once a head asset is assigned). Empty by default
    // so a citizen with no head asset just uses the body mesh's built-in head.
    HeadMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(GetMesh());

    // Modular hair, likewise a leader-pose follower of the body. Empty until an
    // appearance set supplies hair meshes.
    HairMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HairMesh"));
    HairMesh->SetupAttachment(GetMesh());

    // Modular outfit/clothing, same follower setup. Empty until an appearance set
    // supplies outfit meshes.
    OutfitMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OutfitMesh"));
    OutfitMesh->SetupAttachment(GetMesh());

    // The citizen's voice. Created for everyone; it only builds its audio path the
    // first time the person actually speaks, and stays silent for mute profiles.
    VoiceComp = CreateDefaultSubobject<UGTCVoiceComponent>(TEXT("VoiceComp"));

    // Default the whole crowd to the SAME wardrobe the player creator uses, so one
    // data asset drives every face/body/skin in the game. Soft path + guarded load,
    // so until DA_PlayerAppearance is authored each citizen still falls back to the
    // stock mannequins + head paths above (no crash, no behaviour change).
    AppearanceSet = TSoftObjectPtr<UGTCAppearanceSet>(
        FSoftObjectPath(TEXT("/Game/GTCaliberAssets/Content/Characters/DA_PlayerAppearance.DA_PlayerAppearance")));
}

void AGTCCitizen::BeginPlay()
{
    Super::BeginPlay();

    if (!bInitialized)
    {
        InitializeFromSeed(CitizenSeed);
    }

    if (UWorld* World = GetWorld())
    {
        if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
        {
            Crowd->RegisterCitizen(this);
        }
    }
}

void AGTCCitizen::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseClaim();
    if (UWorld* World = GetWorld())
    {
        if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
        {
            // Hand this person's lived state back to the census before the body goes,
            // so the roster member keeps its drives/intent and resumes off-screen
            // (covers retirement, death, and level teardown alike).
            if (StableId != INDEX_NONE)
            {
                // Also persist who this person had come to know, so a regular stays a
                // regular after the body despawns and is re-embodied later.
                Crowd->StoreCitizenAcquaintance(StableId, Acquaintances);
                // A killed citizen is NOT resumed off-screen — skip the write-back so
                // the roster doesn't re-embody the dead person walking the next street
                // over. (Tombstoning the census record outright is a later concern;
                // for now the body dies and is simply not handed back to resume.)
                if (!bDead)
                {
                    Crowd->ReleaseCitizenRecord(StableId, Needs, CurrentIntent, GetActorLocation());
                }
                StableId = INDEX_NONE;
            }
            Crowd->UnregisterCitizen(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AGTCCitizen::InitializeFromSeed(int32 Seed)
{
    // A hand-placed or non-persistent citizen: derive identity from the seed and
    // start the day on the record's fresh drives. Not bound to any roster record.
    const FNpcCitizenRecord Record = FNpcCitizenRecord::FromSeed(INDEX_NONE, Seed);
    ApplyIdentity(Record);
    Needs = Record.Needs;
    CurrentIntent = FNpcIntent();
    StableId = INDEX_NONE;
    bInitialized = true;
}

void AGTCCitizen::InitializeFromRecord(const FNpcCitizenRecord& Record)
{
    // An embodied roster member: same seeded identity, but its CARRIED inner state
    // (drives + current intent) is restored so the person resumes the day it was
    // living off-screen instead of being reborn fresh. StableId binds the body back
    // to the record for write-back on retire.
    ApplyIdentity(Record);
    Needs = Record.Needs;
    CurrentIntent = Record.CurrentIntent;
    Acquaintances = Record.Acquaintances; // resume this person's social ties
    StableId = Record.StableId;
    bInitialized = true;
}

void AGTCCitizen::ApplyIdentity(const FNpcCitizenRecord& Record)
{
    CitizenSeed = Record.Seed;
    HeadVariant = Record.HeadVariant;
    HairVariant = Record.HairVariant;
    OutfitVariant = Record.OutfitVariant;
    bFaceHighDetail = false; // (re)spawns start on the cheap crowd head.
    Archetype = FNpcArchetypes::Pick(Record.Seed);
    Discipline = Record.Discipline;
    Bravery = Record.Bravery;
    Curiosity = Record.Curiosity;
    WalkSpeed = Record.WalkSpeed;
    RunSpeed = Record.RunSpeed;
    DecayRates = Record.DecayRates;

    // (Re)embody at full health — pedestrian wounds aren't persisted across despawn.
    // Toughness is the per-actor tunable, so a BP subclass (a cop) soaks more rounds.
    Vitals = FNpcVitals(CitizenMaxHealth);
    bDead = false;

    // Give the person an actual voice: a fixed vocal signature (pitch/rate/timbre)
    // derived deterministically from the same seed + persona token that pick the
    // words they say, so two "influencer"s sound related but not identical and the
    // same citizen always sounds the same. Audio/lip-sync layers read this; the
    // C++ uses it to pace spoken lines (SpeakLine). Fired to BP once here.
    VoiceProfile = FNpcVoice::ProfileFor(Archetype.Voice, Record.Seed);
    OnVoiceAssigned(FGTCVoiceProfile(VoiceProfile));
    if (VoiceComp)
    {
        VoiceComp->Configure(VoiceProfile);
    }

    // Runtime RNG stream for the body's cosmetic/wander rolls, seeded deterministically
    // and independently of the identity derivation (which now lives in the pure-core
    // FromSeed) so the same seed is always the same look + wander.
    Rng = FRandomStream(Record.Seed);

    // Give the citizen an actual animated body so it reads as a walking person,
    // not an invisible capsule. Soft-loaded + guarded so a headless/cooked build
    // with the assets absent simply stays bodiless rather than failing.
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        // Prefer a data-authored appearance set when one is assigned (artists add
        // bodies/faces/skin tones by editing the asset, no recompile); otherwise
        // fall back to the stock mannequins + head paths so an un-configured citizen
        // still gets a body. Selection is by seed, so the same person looks the same.
        UGTCAppearanceSet* Appearance = AppearanceSet.LoadSynchronous();
        const FLinearColor SkinTone =
            Appearance ? Appearance->ResolveSkinTone(Record.Seed, FLinearColor::White) : FLinearColor::White;

        const bool bQuinn = (Rng.RandHelper(2) == 0);
        USkeletalMesh* Body = Appearance ? Appearance->ResolveBody(Record.Seed) : nullptr;
        if (!Body)
        {
            Body = LoadObject<USkeletalMesh>(nullptr, bQuinn ? QuinnMeshPath : MannyMeshPath);
        }
        if (Body)
        {
            MeshComp->SetSkeletalMesh(Body);
            // Stand the mesh on the capsule and face +X (the ACharacter convention).
            MeshComp->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -90.0), FRotator(0.0, -90.0, 0.0));

            UClass* AbpClass = Appearance ? Appearance->ResolveBodyAnimClass() : nullptr;
            if (!AbpClass)
            {
                AbpClass = LoadClass<UAnimInstance>(nullptr, LocomotionAbpPath);
            }
            if (AbpClass)
            {
                MeshComp->SetAnimInstanceClass(AbpClass);
            }

            // Tint the outfit by archetype + skin tone so a crowd reads as distinct people.
            if (MeshComp->GetMaterial(0))
            {
                if (UMaterialInstanceDynamic* Dyn = MeshComp->CreateDynamicMaterialInstance(0))
                {
                    Dyn->SetVectorParameterValue(TEXT("Tint"), Archetype.Tint);
                    Dyn->SetVectorParameterValue(TEXT("BaseColor"), Archetype.Tint);
                    Dyn->SetVectorParameterValue(TEXT("Color"), Archetype.Tint);
                    Dyn->SetVectorParameterValue(TEXT("SkinTone"), SkinTone);
                }
            }

            // Wear the seeded face: pick the head from the set (or the fallback path),
            // drive it from the body's pose (leader-pose follower) so it animates with
            // the walk. Guarded — no head asset means the body's built-in head stands in.
            if (HeadMesh)
            {
                USkeletalMesh* Head = Appearance ? Appearance->ResolveHead(Record.HeadVariant) : nullptr;
                if (!Head)
                {
                    const int32 Variant = Record.HeadVariant % UE_ARRAY_COUNT(HeadMeshPaths);
                    Head = LoadObject<USkeletalMesh>(nullptr, HeadMeshPaths[Variant]);
                }
                if (Head)
                {
                    HeadMesh->SetSkeletalMesh(Head);
                    HeadMesh->SetLeaderPoseComponent(MeshComp);
                    if (UMaterialInstanceDynamic* HeadDyn = HeadMesh->CreateDynamicMaterialInstance(0))
                    {
                        HeadDyn->SetVectorParameterValue(TEXT("Tint"), Archetype.Tint);
                        HeadDyn->SetVectorParameterValue(TEXT("BaseColor"), Archetype.Tint);
                        HeadDyn->SetVectorParameterValue(TEXT("Color"), Archetype.Tint);
                        HeadDyn->SetVectorParameterValue(TEXT("SkinTone"), SkinTone);
                    }
                }
            }

            // Seeded hair — a second variation axis. Set-only (stock mannequins have
            // no separate hair); leader-posed to the body so it rides head motion.
            if (HairMesh)
            {
                USkeletalMesh* Hair = Appearance ? Appearance->ResolveHair(Record.HairVariant) : nullptr;
                if (Hair)
                {
                    HairMesh->SetSkeletalMesh(Hair);
                    HairMesh->SetLeaderPoseComponent(MeshComp);
                }
            }

            // Seeded outfit — the third axis. Set-only; leader-posed to the body and
            // tinted by the archetype so a citizen's clothes match its palette.
            if (OutfitMesh)
            {
                USkeletalMesh* Outfit = Appearance ? Appearance->ResolveOutfit(Record.OutfitVariant) : nullptr;
                if (Outfit)
                {
                    OutfitMesh->SetSkeletalMesh(Outfit);
                    OutfitMesh->SetLeaderPoseComponent(MeshComp);
                    if (UMaterialInstanceDynamic* OutfitDyn = OutfitMesh->CreateDynamicMaterialInstance(0))
                    {
                        OutfitDyn->SetVectorParameterValue(TEXT("Tint"), Archetype.Tint);
                        OutfitDyn->SetVectorParameterValue(TEXT("BaseColor"), Archetype.Tint);
                        OutfitDyn->SetVectorParameterValue(TEXT("Color"), Archetype.Tint);
                    }
                }
            }
        }
    }
}

void AGTCCitizen::WitnessPlayerMayhem(double Intensity)
{
    MemoryIntensity = FMath::Max(MemoryIntensity, FMath::Clamp(Intensity, 0.0, 1.0));
}

double AGTCCitizen::ResolveHour(float DeltaSeconds, UGTCCrowdSubsystem* Crowd, double& OutElapsedHours)
{
    const double Hps = Crowd ? Crowd->GetHoursPerSecond() : FallbackHoursPerSecond;
    OutElapsedHours = static_cast<double>(DeltaSeconds) * Hps;

    if (Crowd)
    {
        return Crowd->GetHourOfDay();
    }
    FallbackHour = FNpcSchedule::WrapHour(FallbackHour + OutElapsedHours);
    return FallbackHour;
}

void AGTCCitizen::Replan(double Hour, UGTCPlaceRegistrySubsystem* Places)
{
    // Run this person's routine on their OWN clock: a fixed per-citizen shift
    // (~+/-45 min) so a shared template (nine-to-five, night owl) doesn't move the
    // whole city in lockstep — one's out the door at 6:42, another at 7:21. Only
    // the schedule decision is shifted; the world clock stays global.
    const double RoutineHour = FNpcScheduleJitter::Apply(Hour, CitizenSeed);
    const FNpcIntent NewIntent = FNpcMind::Decide(Archetype.Schedule, RoutineHour, Needs, Discipline);

    // If the destination kind changed, drop the old occupancy claim and resolve the
    // new place. Re-using the same kind keeps the existing goal (no thrashing).
    const FName NewKind(*NewIntent.Place.ToLower());
    const bool bKindChanged = (NewKind != ClaimedPlaceKind) || !bHasGoal;
    CurrentIntent = NewIntent;

    if (bKindChanged && Places)
    {
        ReleaseClaim();

        const FVector From = GetActorLocation();
        const FGTCPlaceQueryResult Q = Places->FindAvailable(NewKind, From);
        if (Q.bValid)
        {
            CurrentGoal = Q.Location;
            // Keep the citizen on its own ground height — the registry/synth point
            // may sit at a nominal Z; movement is planar anyway.
            CurrentGoal.Z = From.Z;
            GoalAnchor = CurrentGoal;
            bHasGoal = true;
            bGoalSynthesized = Q.bSynthesized;
            bArrived = false;

            if (!Q.bSynthesized)
            {
                Places->NotePlaceClaimed(NewKind, Q.Location);
                ClaimedPlaceKind = NewKind;
                ClaimedPlaceLocation = Q.Location;
                bHasClaim = true;

                // Read how busy the spot is and react. Curiosity stands in for
                // crowd-tolerance: the nosy don't mind a throng. The behaviour layer
                // gets the tier; the C++ also stops the citizen SHORT of a busy/packed
                // centre so a crowd rings/queues the spot instead of stacking bodies on
                // one point — the crowd-averse hang back further.
                const ENpcBusyness Busy = FNpcCrowding::Classify(Q.Occupancy, Q.Capacity);
                OnPlaceBusyness(NewKind, static_cast<uint8>(Busy));

                const float Standoff = FNpcCrowding::StandoffDistance(Busy, Curiosity);
                if (Standoff > 0.0f)
                {
                    // Stop on the approach side of the POI, backed off by Standoff.
                    FVector Approach = From - Q.Location;
                    Approach.Z = 0.0;
                    const FVector Dir = Approach.IsNearlyZero()
                        ? FVector(1.0, 0.0, 0.0) : Approach.GetSafeNormal();
                    CurrentGoal = Q.Location + Dir * Standoff;
                    CurrentGoal.Z = From.Z;
                    GoalAnchor = CurrentGoal;
                    UE_LOG(LogTemp, Verbose, TEXT("[%s>place %s is %s — hanging back %.0fcm]"),
                        *Archetype.Name, *NewKind.ToString(), FNpcCrowding::Name(Busy), Standoff);
                }
            }

            RecomputePath();
        }
    }
}

void AGTCCitizen::RecomputePath()
{
    PathPoints.Reset();
    PathIndex = 0;
    bUsePath = false;
    if (!bHasGoal)
    {
        return;
    }

    UWorld* World = GetWorld();
    UNavigationSystemV1* Nav = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
    if (!Nav)
    {
        return; // No NavMesh in this level — steer straight at the goal instead.
    }

    if (UNavigationPath* Path = Nav->FindPathToLocationSynchronously(World, GetActorLocation(), CurrentGoal, this))
    {
        if (Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num() > 1)
        {
            PathPoints = Path->PathPoints;
            PathIndex = 1; // [0] is our current position.
            bUsePath = true;
        }
    }
}

FVector AGTCCitizen::CurrentSteerGoal()
{
    if (!bUsePath || PathPoints.Num() == 0)
    {
        return CurrentGoal;
    }
    // Advance past any waypoints already reached, then steer at the next one.
    while (PathIndex < PathPoints.Num() - 1
        && FNpcLocomotion::HasArrived(GetActorLocation(), PathPoints[PathIndex], ArriveToleranceCm))
    {
        ++PathIndex;
    }
    PathIndex = FMath::Clamp(PathIndex, 0, PathPoints.Num() - 1);
    return PathPoints[PathIndex];
}

void AGTCCitizen::SatisfyArrivalNeed()
{
    FString Need;
    double Amount = 0.0;
    if (FNpcPopulation::ActivitySatisfies(CurrentIntent.Activity, Need, Amount))
    {
        Needs.Satisfy(Need, Amount);
    }
}

FNpcBrain::EState AGTCCitizen::DecideBrainState(
    UGTCCrowdSubsystem* Crowd, bool& bThreatActive, FVector& OutThreatPos)
{
    bThreatActive = false;
    OutThreatPos = FVector::ZeroVector;
    bGawking = false;
    bCowering = false;

    const FVector Self = GetActorLocation();

    // Baseline (no threat): purposeful walk toward the goal, or stand and perform
    // the activity once arrived.
    const FNpcBrain::EState Base = bArrived ? FNpcBrain::EState::Idle : FNpcBrain::EState::Wander;

    double ThreatDistance = TNumericLimits<double>::Max();

    if (Crowd)
    {
        const FGTCThreatSnapshot Threat = Crowd->GetPlayerThreat();
        if (Threat.bValid)
        {
            OutThreatPos = Threat.Position;
            ThreatDistance = FNpcLocomotion::GroundDistance(Self, Threat.Position);

            // FNpcReaction / FNpcMemory speak the Godot unit system (metres, m/s);
            // the world is in cm. Convert at the boundary.
            const double ThreatDistM = ThreatDistance / 100.0;
            const double ThreatScore = FNpcReaction::ThreatFrom(Threat.Speed / 100.0, Threat.bArmed);
            const FString Verb = FNpcReaction::Decide(ThreatDistM, ThreatScore, Bravery, Curiosity);

            // A witness who still remembers the player's mayhem treats them as a live
            // threat even unarmed — the city remembers.
            const bool bRecognizes = FNpcMemory::Recognizes(MemoryIntensity, ThreatDistM);
            bThreatActive = (Verb == TEXT("flee")) || (bRecognizes && MemoryIntensity >= FNpcMemory::Alarmed);
            // Not scared, but interested: stop and stare (handled as Idle + facing).
            bGawking = !bThreatActive && (Verb == TEXT("gawk"));

            // A weapon levelled point-blank is duress, not just a threat: a scared
            // citizen freezes and complies (Cower) instead of running; the steadier or
            // mid-range ones flee. Cower overrides flee so they don't bolt into the gun.
            const ENpcDuress Duress = FNpcDuress::Decide(ThreatDistM, Threat.bArmed, Bravery);
            if (Duress == ENpcDuress::Cower)
            {
                bCowering = true;
                bThreatActive = false; // freeze, hands up — running gets you shot
                bGawking = false;
            }
            else if (Duress == ENpcDuress::Flee)
            {
                bThreatActive = true;
            }
        }

        // Panic is contagious: a fleeing neighbour close by spooks all but the
        // steadiest (FNpcReaction::CatchesPanic gates on bravery). Skip it while
        // cowering — otherwise it would overwrite OutThreatPos with a bystander's
        // position, aiming the hands-up pose and facing away from the actual gun.
        if (!bThreatActive && !bCowering)
        {
            FVector PanicPos;
            if (Crowd->NearestPanicSource(Self, FNpcReaction::PanicRadius * 100.0, this, PanicPos))
            {
                const double PanicDist = FNpcLocomotion::GroundDistance(Self, PanicPos);
                if (FNpcReaction::CatchesPanic(PanicDist / 100.0, Bravery))
                {
                    bThreatActive = true;
                    OutThreatPos = PanicPos;
                    ThreatDistance = PanicDist;
                }
            }
        }
    }

    // A gun in the face freezes you: cower trumps everything, including the flee
    // hysteresis — a citizen the player corners point-blank stops running and complies.
    if (bCowering)
    {
        return FNpcBrain::EState::Idle;
    }

    // Layer flee (with NpcBrain hysteresis) over the purposeful baseline.
    FNpcBrain::EState Next =
        FNpcBrain::NextState(BrainState, bThreatActive, ThreatDistance, FleeRadiusCm, CalmRadiusCm);
    if (Next != FNpcBrain::EState::Flee)
    {
        // Gawking trumps the purposeful baseline: stop walking and rubberneck.
        Next = bGawking ? FNpcBrain::EState::Idle : Base;
    }
    return Next;
}

void AGTCCitizen::DriveLocomotion(
    float DeltaSeconds, UGTCCrowdSubsystem* Crowd, bool bThreatActive, const FVector& ThreatPos)
{
    const FVector Self = GetActorLocation();
    const double TargetSpeed = FNpcBrain::SpeedFor(BrainState, WalkSpeed, RunSpeed);
    if (TargetSpeed <= FNpcLocomotion::Eps)
    {
        return; // Idle — stand still (movement brakes to a halt on its own).
    }

    const FVector Goal = (BrainState == FNpcBrain::EState::Flee)
        ? FNpcLocomotion::FleeGoal(Self, ThreatPos, FleeGoalDistanceCm)
        : CurrentSteerGoal();

    if (BrainState != FNpcBrain::EState::Flee && !bHasGoal)
    {
        return; // Nowhere to be yet.
    }

    FNpcLocomotion::FParams Params;
    Params.ArriveRadius = ArriveToleranceCm;

    TArray<FVector> Neighbors;
    TArray<FPedestrianTraffic::FCar> Cars;
    if (Crowd)
    {
        Crowd->GatherNeighbors(Self, Params.SeparationRadius * 2.0, this, Neighbors);
        Crowd->GatherNearbyCars(Self, Params.CarReactRadius, Cars);

        // Give the player personal space too: feed them in as a separation neighbour
        // so a walking crowd bows and flows around the player instead of clipping
        // straight through them — "something happens" the instant you walk by. Reuses
        // the tested separation steering unchanged.
        const FGTCThreatSnapshot Player = Crowd->GetPlayerThreat();
        if (Player.bValid
            && FNpcLocomotion::GroundDistance(Self, Player.Position) <= Params.SeparationRadius * 2.0)
        {
            Neighbors.Add(Player.Position);
        }
    }

    // Curb safety: if a car is about to pass through this spot, stop and let it go
    // by instead of walking into it — the pedestrian waiting at the kerb. The dodge
    // reflex handles cars already beside them; this is the halt-before-stepping-off
    // instinct. Re-evaluated every frame, so the citizen resumes the moment it clears.
    if (BrainState != FNpcBrain::EState::Flee && Cars.Num() > 0)
    {
        TArray<FPedestrianTraffic::FCar> PlaneCars;
        PlaneCars.Reserve(Cars.Num());
        for (const FPedestrianTraffic::FCar& C : Cars)
        {
            FPedestrianTraffic::FCar PC;
            PC.Pos = FNpcLocomotion::ToPlane(C.Pos);
            PC.Vel = FNpcLocomotion::ToPlane(C.Vel);
            PlaneCars.Add(PC);
        }
        // Near-miss startle: if a car is closing fast and close (threat past the
        // startle bar), flinch and yelp — once, on the rising edge, so a lingering
        // car doesn't make them shout every frame. The kerb halt below still runs.
        const FVector SelfPlane = FNpcLocomotion::ToPlane(Self);
        const FPedestrianTraffic::FThreat Threat = FPedestrianTraffic::NearestThreat(
            SelfPlane, FNpcLocomotion::ToPlane(GetVelocity()),
            PlaneCars, CurbDangerRadiusCm, CurbSafeGapSec);
        const bool bStartled = Threat.Threat >= TrafficStartleThreshold;
        if (bStartled && !bWasTrafficStartled)
        {
            // Map the threat's plane position back to a UE world direction for the
            // flinch montage (which way to jump).
            const FVector ToCarPlane = Threat.Pos - SelfPlane;
            const FVector ThreatDir = ToCarPlane.IsNearlyZero()
                ? FVector::ZeroVector : FNpcLocomotion::FromPlane(ToCarPlane).GetSafeNormal();
            OnTrafficStartle(ThreatDir);
            if (FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec))
            {
                const FString Line = FTrafficBark::Line(Archetype.Voice, CitizenSeed + (BarkCounter++ * 7919));
                if (!Line.IsEmpty())
                {
                    SpeakLine(Line);
                    TimeSinceBark = 0.0;
                    UE_LOG(LogTemp, Verbose, TEXT("[%s>traffic!] %s"), *Archetype.Name, *Line);
                }
            }
        }
        bWasTrafficStartled = bStartled;

        if (!FPedestrianTraffic::SafeToCross(
                FNpcLocomotion::ToPlane(Self), PlaneCars, CurbDangerRadiusCm, CurbSafeGapSec))
        {
            return; // wait — don't step into traffic this frame.
        }
    }

    const FVector DesiredVel = FNpcLocomotion::DesiredVelocity(
        BrainState, Self, Goal, Neighbors, Cars, WalkSpeed, RunSpeed, Params);

    const double Speed = DesiredVel.Size();
    if (Speed > FNpcLocomotion::Eps)
    {
        if (UCharacterMovementComponent* Move = GetCharacterMovement())
        {
            Move->MaxWalkSpeed = static_cast<float>(TargetSpeed);
        }
        // Magnitude carries Arrive's easing; scale the input so a near-goal ease-in
        // actually slows the body instead of snapping to full speed.
        const double Scale = FMath::Clamp(Speed / TargetSpeed, 0.0, 1.0);
        AddMovementInput(DesiredVel / Speed, static_cast<float>(Scale));
    }
}

void AGTCCitizen::MaybeBark(float DeltaSeconds, bool bThreatActive)
{
    TimeSinceBark += DeltaSeconds;
    // Frozen at gunpoint: the only thing it says is the terrified line emitted on the
    // cower rising edge — never a calm activity bark. (Timer still advances above.)
    if (bCowering)
    {
        return;
    }
    if (!FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec))
    {
        return;
    }

    const int32 Seed = CitizenSeed + (BarkCounter++ * 7919);
    FString Line;

    if (BrainState == FNpcBrain::EState::Flee)
    {
        Line = FBarkPool::Line(EBarkSituation::FLEE, Seed);
    }
    else if (MemoryIntensity >= FNpcMemory::Alarmed)
    {
        Line = FNpcDialogue::WitnessBark(Seed);
    }
    else if (bThreatActive)
    {
        Line = FBarkPool::Line(EBarkSituation::ALARMED, Seed);
    }
    else if (!CurrentIntent.Activity.IsEmpty())
    {
        Line = FNpcDialogue::BarkForActivity(Archetype.Voice, CurrentIntent.Activity, Seed);
    }

    if (!Line.IsEmpty())
    {
        SpeakLine(Line);
        TimeSinceBark = 0.0;
        // The Label3D speech-bubble render is a later presentation concern; verbose
        // log keeps the city's voice observable in PIE/automation meanwhile.
        UE_LOG(LogTemp, Verbose, TEXT("[%s] %s"), *Archetype.Name, *Line);
    }
}

void AGTCCitizen::SetFaceHighDetail(bool bHighDetail)
{
    if (bHighDetail == bFaceHighDetail || !HeadMesh)
    {
        return; // tier unchanged, or no head component to swap.
    }

    // The hero tier only exists if an appearance set supplies a hero head pool;
    // without one we stay on whatever head we have (single-tier crowd).
    UGTCAppearanceSet* Appearance = AppearanceSet.LoadSynchronous();
    if (!Appearance)
    {
        return;
    }

    USkeletalMesh* Target = bHighDetail
        ? Appearance->ResolveHeroHead(HeadVariant)
        : Appearance->ResolveHead(HeadVariant);
    if (!Target)
    {
        return; // requested tier has no asset — keep the current head.
    }

    HeadMesh->SetSkeletalMesh(Target);
    if (USkeletalMeshComponent* Body = GetMesh())
    {
        HeadMesh->SetLeaderPoseComponent(Body); // keep following the body's pose.
    }
    bFaceHighDetail = bHighDetail;
}

void AGTCCitizen::SpeakLine(const FString& Line)
{
    LastBark = Line;

    // Delivery time the viseme/jaw driver spans: the line's syllables paced at THIS
    // citizen's own speaking rate (a fast influencer finishes sooner than a slow
    // philosopher), plus a beat per sentence stop. Beats the old char/12 guess; a
    // real audio clip's length still overrides this in the bound event.
    const float EstimatedSeconds = FNpcVoice::EstimateSpokenSeconds(Line, VoiceProfile);
    OnSpeak(Line, EstimatedSeconds);

    // Actually make a sound: synthesise + play the line in this citizen's voice. A
    // per-utterance seed varies the contour so repeats don't sound identical. The
    // component no-ops for a mute profile, so the lip-sync hook above still fires.
    if (VoiceComp)
    {
        const int32 LineSeed = static_cast<int32>(
            static_cast<uint32>(CitizenSeed) * 2654435761u + static_cast<uint32>(SpeakCounter++));
        VoiceComp->PlayLine(Line, LineSeed);
    }
}

bool AGTCCitizen::IsAvailableToChat() const
{
    return bInitialized && bArrived && !bChatting && !bGawking
        && BrainState != FNpcBrain::EState::Flee
        && ActivityLingers(CurrentIntent.Activity);
}

void AGTCCitizen::BeginChatWith(AGTCCitizen* Partner)
{
    if (!Partner)
    {
        return;
    }
    ChatPartner = Partner;
    bChatting = true;
    ChatTimer = 0.0;
    // Turn-taking is arbitrated per-beat by FConversationFloor off the shared clock
    // (ChatTimer), so we no longer hand-stagger the two timers — both sides slice
    // time into the same turns and exactly one speaks each turn. Reset the
    // last-voiced turn so this side will take its first turn when it comes up.
    LastChatTurn = -1;

    // Social memory: note that we've run into this person. If we already knew them
    // (a regular), the opener will be a warm greeting instead of the wary stranger
    // line. Key on the partner's stable id so it's the same person across the day.
    const int32 PartnerKey = (Partner->GetStableId() != INDEX_NONE)
        ? Partner->GetStableId() : Partner->GetSeed();
    bChatPartnerFamiliar = Acquaintances.TierWith(PartnerKey) != ENpcFamiliarity::Stranger;
    Acquaintances.Meet(PartnerKey);
}

void AGTCCitizen::EndChat()
{
    bChatting = false;
    ChatPartner = nullptr;
    ChatTimer = 0.0;
    LastChatTurn = -1;
}

EGTCContactReaction AGTCCitizen::GetActiveContactReaction() const
{
    return ToBp(ContactReaction);
}

void AGTCCitizen::ApplyPlayerImpact(float ImpactSpeedMetersPerSec, bool bStrike, FVector ImpactDirection)
{
    FVector Push = ImpactDirection;
    Push.Z = 0.0;
    FVector ThreatPos = FVector::ZeroVector;

    if (Push.IsNearlyZero())
    {
        // No direction given — derive it from where the player actually is.
        if (UWorld* World = GetWorld())
        {
            if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
            {
                const FGTCThreatSnapshot T = Crowd->GetPlayerThreat();
                if (T.bValid)
                {
                    ThreatPos = T.Position;
                    Push = GetActorLocation() - T.Position;
                    Push.Z = 0.0;
                }
            }
        }
    }
    else
    {
        // The push points player -> citizen, so the player sits behind it.
        ThreatPos = GetActorLocation() - Push.GetSafeNormal() * 100.0;
    }

    if (ThreatPos.IsZero())
    {
        // Player unresolved and no direction given: synthesize a phantom attacker
        // just off our front, so flee/face stay sensible relative to us and never
        // steer toward world origin.
        FVector ToAttacker = Push.IsNearlyZero() ? GetActorForwardVector() : -Push;
        ToAttacker.Z = 0.0;
        ToAttacker = ToAttacker.GetSafeNormal();
        if (ToAttacker.IsNearlyZero())
        {
            ToAttacker = GetActorForwardVector();
        }
        ThreatPos = GetActorLocation() + ToAttacker * 100.0;
    }

    ApplyContact(static_cast<double>(ImpactSpeedMetersPerSec), bStrike, Push, ThreatPos);
}

float AGTCCitizen::GetHealthFraction() const
{
    return static_cast<float>(Vitals.Fraction());
}

float AGTCCitizen::TakeDamage(
    float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (bDead || DamageAmount <= 0.0f)
    {
        return Applied;
    }

    // Recover the round's travel direction from a point-damage event (the weapon
    // fills ShotDirection on FPointDamageEvent). Leave it zero for other damage
    // types; ApplyGunshot then derives a sane push from the player's position.
    FVector BulletTravel = FVector::ZeroVector;
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& Pt = static_cast<const FPointDamageEvent&>(DamageEvent);
        BulletTravel = Pt.ShotDirection;
        BulletTravel.Z = 0.0;
    }

    ApplyGunshot(static_cast<double>(DamageAmount), BulletTravel);
    return Applied;
}

void AGTCCitizen::ApplyGunshot(double Damage, const FVector& BulletTravel)
{
    // Planar push direction (away from the shooter). With no direction on the event,
    // derive it from the player's tracked position so stagger/flee point somewhere
    // sane instead of world origin.
    FVector Travel = BulletTravel;
    Travel.Z = 0.0;
    Travel = Travel.GetSafeNormal();
    if (Travel.IsNearlyZero())
    {
        if (UWorld* World = GetWorld())
        {
            if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
            {
                const FGTCThreatSnapshot T = Crowd->GetPlayerThreat();
                if (T.bValid)
                {
                    Travel = (GetActorLocation() - T.Position).GetSafeNormal2D();
                }
            }
        }
    }
    if (Travel.IsNearlyZero())
    {
        Travel = GetActorForwardVector().GetSafeNormal2D();
    }

    const ENpcHitResult Result = Vitals.Apply(Damage);
    if (Result == ENpcHitResult::Ignored)
    {
        return; // already down, or a zero/negative hit — nothing to do
    }

    // Being shot is vivid: the victim maxes its memory of the player (recognise +
    // treat as a threat), and the whole street sees the shooting and panics via the
    // same FNpcMemory machinery a serious melee assault ripples out through the crowd.
    WitnessPlayerMayhem(1.0);
    if (UWorld* World = GetWorld())
    {
        if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
        {
            Crowd->BroadcastMayhem(GetActorLocation(), 1.0, this);
        }
    }

    if (Result == ENpcHitResult::Killed)
    {
        EnterDeath(Travel);
        return;
    }

    // Wounded but up: stagger now, bolt after. Route the recoil through the same
    // reaction state the contact system uses, so the brain forces Idle for the beat
    // and the anim layer gets OnContactReaction; the maxed memory then drives the
    // flee out of DecideBrainState once the recoil ends.
    const double Sev = FNpcVitals::WoundSeverity(Damage, Vitals.MaxHealth);
    const ENpcContactReaction R =
        (Sev >= KnockdownWoundSeverity) ? ENpcContactReaction::Knockdown : ENpcContactReaction::Flinch;

    ContactReaction = R;
    ReactionTimer = FNpcContact::Duration(R);
    ContactSeverity = Sev;
    ContactThreatPos = GetActorLocation() - Travel * 100.0; // the shooter sits behind the round
    ContactPushDir = Travel;
    ContactCooldown = FMath::Max(ContactCooldownSec, ReactionTimer);

    // Physical recoil along the bullet — a knockdown gets some air so the fall reads.
    const double KB = FNpcContact::Knockback(R, Sev);
    if (KB > FNpcLocomotion::Eps)
    {
        const bool bKnockdown = FNpcContact::IsKnockdown(R);
        FVector Launch = Travel * KB;
        Launch.Z = bKnockdown ? 350.0 : 100.0;
        LaunchCharacter(Launch, /*bXYOverride*/ bKnockdown, /*bZOverride*/ true);
    }

    BarkForContact(R);
    OnContactReaction(ToBp(R), static_cast<float>(Sev), -Travel);
    OnGunshotWound(static_cast<float>(Sev), -Travel);
}

void AGTCCitizen::EnterDeath(const FVector& BulletTravel)
{
    if (bDead)
    {
        return;
    }
    bDead = true;

    // Drop everything a living pedestrian was doing.
    if (bChatting)
    {
        if (AGTCCitizen* Partner = ChatPartner.Get())
        {
            if (Partner->ChatPartner.Get() == this)
            {
                Partner->EndChat();
            }
        }
        EndChat();
    }
    ReleaseClaim();
    ContactReaction = ENpcContactReaction::Ignore;
    ReactionTimer = 0.0;
    bCowering = false;
    bGawking = false;

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

    // Ragdoll: simulate the body, take the capsule out of the world, and shove the
    // corpse off the round so the death reads as a hit, not a faint.
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

    // Anim/FX seam, then fade the corpse after it has lain there a while.
    OnKilled(-BulletTravel.GetSafeNormal());
    SetLifeSpan(static_cast<float>(CorpseLingerSeconds));
}

void AGTCCitizen::UpdateContactReaction(float DeltaSeconds, UGTCCrowdSubsystem* Crowd)
{
    if (ContactCooldown > 0.0)
    {
        ContactCooldown = FMath::Max(0.0, ContactCooldown - DeltaSeconds);
    }

    if (FNpcContact::Reacts(ContactReaction))
    {
        ReactionTimer -= DeltaSeconds;
        if (ReactionTimer <= 0.0)
        {
            ContactReaction = ENpcContactReaction::Ignore;
            ReactionTimer = 0.0;
            // Don't let a just-shoved citizen cheerily glance/greet the culprit on
            // the very next tick — hold the pass-by acknowledgement off for a beat.
            PassByCooldown = FMath::Max(PassByCooldown, PassByCooldownSec);
        }
    }

    // A fresh body bump can start a reaction (or, past the cooldown, escalate one).
    DetectBodyContact(Crowd);
}

void AGTCCitizen::DetectBodyContact(UGTCCrowdSubsystem* Crowd)
{
    if (!Crowd || ContactCooldown > 0.0)
    {
        return;
    }

    const FGTCThreatSnapshot T = Crowd->GetPlayerThreat();
    if (!T.bValid)
    {
        return;
    }

    FVector ToSelf = GetActorLocation() - T.Position;
    ToSelf.Z = 0.0;
    const double Dist = ToSelf.Size();
    if (Dist > ContactRadiusCm)
    {
        return; // not touching
    }

    FVector Dir = ToSelf.GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        Dir = (-GetActorForwardVector()).GetSafeNormal2D(); // exactly overlapping — push off our facing
    }

    // Closing speed: how fast is the player driving into us along that direction?
    FVector Vel = T.Velocity;
    Vel.Z = 0.0;
    const double ApproachCmS = FVector::DotProduct(Vel, Dir);
    if (ApproachCmS < ContactMinApproachCmS)
    {
        return; // standing beside us, brushing past, or already moving away
    }

    ApplyContact(ApproachCmS / 100.0, /*bStrike*/ false, Dir, T.Position);
}

void AGTCCitizen::ApplyContact(
    double ImpactSpeedMps, bool bStrike, const FVector& PushDir, const FVector& ThreatPos)
{
    const double Sev = FNpcContact::Severity(ImpactSpeedMps, bStrike);
    const ENpcContactReaction R = FNpcContact::Decide(Sev, Bravery, MemoryIntensity);
    if (!FNpcContact::Reacts(R))
    {
        return;
    }

    // Debounce policy. While a reaction is still playing, only a STRICTLY more
    // intense contact may interrupt it (the enum is ordered by intensity) — so a
    // downed or fleeing citizen plays out, and recovers from, what already hit it
    // instead of being pinned by repeated equal taps. With nothing in flight, auto
    // body-bumps are rate-limited by the cooldown, but a deliberate strike always
    // lands (the combat layer is never silently swallowed).
    const bool bReacting = FNpcContact::Reacts(ContactReaction) && ReactionTimer > 0.0;
    if (bReacting)
    {
        if (static_cast<uint8>(R) <= static_cast<uint8>(ContactReaction))
        {
            return; // not an escalation — let the current reaction run its course
        }
    }
    else if (!bStrike && ContactCooldown > 0.0)
    {
        return; // one body-bump, one reaction
    }

    ContactReaction = R;
    ReactionTimer = FNpcContact::Duration(R);
    // First retaliatory shove comes a beat after squaring up (Confront), not on the
    // same frame as the contact — they bristle, then shove back.
    ConfrontShoveTimer = ConfrontShoveIntervalSec;
    ContactSeverity = Sev;
    ContactThreatPos = ThreatPos;
    // Hold the debounce across the whole reaction so a body bump can't re-fire
    // mid-reaction (DetectBodyContact gates on this too); a harder strike still
    // gets through via the escalation path above.
    ContactCooldown = FMath::Max(ContactCooldownSec, ReactionTimer);

    ContactPushDir = PushDir.GetSafeNormal2D();
    if (ContactPushDir.IsNearlyZero())
    {
        ContactPushDir = (-GetActorForwardVector()).GetSafeNormal2D();
    }

    // The city remembers being shoved or struck — feeds NpcMemory recognition so
    // the victim calls the player out and treats them as a threat for a while.
    const double Bump = FNpcContact::MemoryBump(R, Sev);
    if (Bump > 0.0)
    {
        WitnessPlayerMayhem(Bump);
    }

    // A visible assault — the whole street notices, not just the victim. Ripple the
    // mayhem out so nearby bystanders recognise, recoil from, and flee the player via
    // the existing FNpcMemory machinery. A mild brush/bump/flinch isn't "mayhem"; only
    // a squared-up, fleeing, or floored victim draws the crowd's eye.
    if (FNpcContact::IsPanic(R) || FNpcContact::IsHostile(R))
    {
        if (UWorld* World = GetWorld())
        {
            if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
            {
                Crowd->BroadcastMayhem(GetActorLocation(), Bump, this);
            }
        }
    }

    // You don't keep chatting after someone barges into you.
    if (bChatting)
    {
        if (AGTCCitizen* Partner = ChatPartner.Get())
        {
            if (Partner->ChatPartner.Get() == this)
            {
                Partner->EndChat();
            }
        }
        EndChat();
    }

    // Physical shove: throw the body along the push direction. A knockdown also
    // gets a little air so the fall reads; a flinch is just a recoil step.
    const double KB = FNpcContact::Knockback(R, Sev);
    if (KB > FNpcLocomotion::Eps)
    {
        const bool bKnockdown = FNpcContact::IsKnockdown(R);
        FVector Launch = ContactPushDir * KB;
        // Give the body a little air so the shove leaves Walking mode for a beat —
        // otherwise BrakingDecelerationWalking eats the planar recoil the same
        // frame (the reaction also forces Idle, so no movement input opposes it).
        Launch.Z = bKnockdown ? 350.0 : 100.0;
        LaunchCharacter(Launch, /*bXYOverride*/ bKnockdown, /*bZOverride*/ true);
    }

    BarkForContact(R);

    // Hand the reaction to the animation layer (montage / get-up); unbound is fine.
    OnContactReaction(ToBp(R), static_cast<float>(Sev), ContactPushDir);
}

void AGTCCitizen::BarkForContact(ENpcContactReaction R)
{
    const int32 Seed = CitizenSeed + (BarkCounter++ * 7919);
    // Dedicated contact lines specific to being brushed / bumped / shoved / floored
    // — so a shove no longer borrows the generic gunshot-panic bank.
    const FString Line = FContactBark::Line(Archetype.Voice, R, Seed);
    if (!Line.IsEmpty())
    {
        SpeakLine(Line);
        TimeSinceBark = 0.0; // this is this beat's line — don't let MaybeBark double up
        UE_LOG(LogTemp, Verbose, TEXT("[%s>contact] %s"), *Archetype.Name, *Line);
    }
}

void AGTCCitizen::MaybeNoticePlayer(float DeltaSeconds, UGTCCrowdSubsystem* Crowd)
{
    if (PassByCooldown > 0.0)
    {
        PassByCooldown = FMath::Max(0.0, PassByCooldown - DeltaSeconds);
    }

    // Only a calm citizen, going about its day, casually clocks a passer-by. Fear /
    // a live contact reaction / a chat / a gawk already own the citizen's attention.
    if (!Crowd || PassByCooldown > 0.0 || bChatting || bGawking
        || FNpcContact::Reacts(ContactReaction)
        || BrainState == FNpcBrain::EState::Flee)
    {
        return;
    }

    const FGTCThreatSnapshot T = Crowd->GetPlayerThreat();
    if (!T.bValid)
    {
        return;
    }
    const FVector Self = GetActorLocation();
    const double Dist = FNpcLocomotion::GroundDistance(Self, T.Position);
    // Close enough to register, but past touching distance (a bump is the contact
    // layer's job, not a glance).
    if (Dist > PassByRadiusCm || Dist <= ContactRadiusCm)
    {
        return;
    }

    FVector ToPlayer = T.Position - Self;
    ToPlayer.Z = 0.0;
    const FVector Dir = ToPlayer.GetSafeNormal();

    // Glance hook for the animation layer (head-look / nod).
    OnPlayerNoticed(Dir);
    PassByCooldown = PassByCooldownSec;

    // The nosier/chattier sort actually say something; most just look. Curiosity
    // gates it so a street isn't a chorus of greetings. The roll is drawn from a
    // dedicated stream so a passing player never perturbs the wander RNG.
    const FRandomStream GreetRoll(CitizenSeed + 104729 + PassByCounter++ * 7919);
    if (GreetRoll.FRand() < Curiosity * 0.5)
    {
        // On the clock with a trade, the job reaches out — the vendor sells, the
        // crossing guard scolds, the life coach pitches — so the player feels the
        // city's occupations. Off the clock (or a role that doesn't hail, e.g. the
        // mime), fall back to the generic pass-by acknowledgement.
        const int32 LineSeed = CitizenSeed + (BarkCounter++ * 7919);
        const FName Role = (CurrentIntent.Activity == TEXT("work"))
            ? FNpcOccupation::RoleFor(Archetype.Id) : NAME_None;
        const bool bHail = !Role.IsNone() && FNpcHail::Count(Role) > 0;
        const FString Line = bHail
            ? FNpcHail::Line(Role, LineSeed)
            : FContactBark::PassingLine(LineSeed);
        if (!Line.IsEmpty())
        {
            SpeakLine(Line);
            TimeSinceBark = 0.0;
            UE_LOG(LogTemp, Verbose, TEXT("[%s>%s] %s"),
                *Archetype.Name, bHail ? TEXT("hail") : TEXT("passby"), *Line);
        }
    }
}

bool AGTCCitizen::Recognizes(const AGTCCitizen* Other) const
{
    if (!Other)
    {
        return false;
    }
    // Key the same way meetings are recorded (stable id, else seed).
    const int32 Key = (Other->GetStableId() != INDEX_NONE) ? Other->GetStableId() : Other->GetSeed();
    return Acquaintances.TierWith(Key) != ENpcFamiliarity::Stranger;
}

void AGTCCitizen::MaybeGreetAcquaintance(float DeltaSeconds, UGTCCrowdSubsystem* Crowd)
{
    if (WaveCooldown > 0.0)
    {
        WaveCooldown = FMath::Max(0.0, WaveCooldown - DeltaSeconds);
        return;
    }
    // Only a calm, unoccupied citizen waves; fear / a chat / a gawk / a contact
    // reaction already own its attention.
    if (!Crowd || bChatting || bGawking || BrainState == FNpcBrain::EState::Flee
        || FNpcContact::Reacts(ContactReaction))
    {
        return;
    }

    AGTCCitizen* Friend = Crowd->FindNearestAcquaintance(GetActorLocation(), WaveRadiusCm, this);
    if (!Friend)
    {
        // Nobody known nearby — back off before scanning the crowd again so this
        // stays cheap (the scan is O(live count)).
        WaveCooldown = WaveCooldownSec * 0.5;
        return;
    }

    WaveCooldown = WaveCooldownSec;

    FVector To = Friend->GetActorLocation() - GetActorLocation();
    To.Z = 0.0;
    OnWaveTo(To.IsNearlyZero() ? FVector::ZeroVector : To.GetSafeNormal());

    // A greeting in passing (reuses the warm "greet" bank), gated by the bark
    // cadence so a wave doesn't pile onto a recent line.
    if (FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec))
    {
        const FString Line = FNpcConversation::Greeting(Archetype.Voice, CitizenSeed + (BarkCounter++ * 7919));
        if (!Line.IsEmpty())
        {
            SpeakLine(Line);
            TimeSinceBark = 0.0;
            UE_LOG(LogTemp, Verbose, TEXT("[%s>wave] %s"), *Archetype.Name, *Line);
        }
    }
}

void AGTCCitizen::MaybeGlance(float DeltaSeconds)
{
    if (GlanceTimer > 0.0)
    {
        GlanceTimer = FMath::Max(0.0, GlanceTimer - DeltaSeconds);
        return;
    }
    // Only while actually travelling somewhere, calm and unoccupied — not mid-chat,
    // gawk, flee, or contact, and not already standing at the goal.
    if (!bHasGoal || bArrived || bChatting || bGawking
        || BrainState == FNpcBrain::EState::Flee || FNpcContact::Reacts(ContactReaction))
    {
        return;
    }

    // A roll on its own stream so a glance never perturbs the wander RNG (mirrors
    // the idle-action stream discipline).
    const int32 GlanceSeed = CitizenSeed + 7919 * (GlanceCounter++);
    const FRandomStream Roll(GlanceSeed + 13);
    if (Roll.FRand() < FNpcDetour::GlanceChance(Curiosity))
    {
        // Look off to the side (a shop window, a passer-by). The body keeps walking.
        const FVector Side = GetActorRightVector() * (Roll.FRand() < 0.5f ? 1.0 : -1.0);
        FVector Dir = Side;
        Dir.Z = 0.0;
        OnCuriousGlance(Dir.IsNearlyZero() ? FVector::ZeroVector : Dir.GetSafeNormal());

        // Mostly silent; the occasional "ooh, what's that" murmur, cadence-gated.
        if (Roll.FRand() < 0.25f && FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec))
        {
            const FString Line = FBarkPool::Line(EBarkSituation::IDLE, GlanceSeed);
            if (!Line.IsEmpty())
            {
                SpeakLine(Line);
                TimeSinceBark = 0.0;
            }
        }
        // Longer spacing after an actual glance.
        GlanceTimer = FNpcDetour::GlanceCooldown(Curiosity, GlanceSeed);
    }
    else
    {
        // Missed the roll — wait a beat before the next opportunity (keeps this from
        // rolling every frame).
        GlanceTimer = 1.5;
    }
}

void AGTCCitizen::ReactToWeather(int32 Severity)
{
    // Current clock: prefer the shared crowd clock, else this citizen's fallback.
    double Hour = FallbackHour;
    if (UWorld* World = GetWorld())
    {
        if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
        {
            Hour = Crowd->GetHourOfDay();
        }
    }

    const FNpcWeatherResponse R = FNpcWeatherReaction::Assess(Severity, Hour, Bravery);
    OnWeatherResponse(R.bSeekShelter, R.bHurry, R.Discomfort);

    // Grumble about the weather when it's unpleasant enough, honouring the bark
    // cadence so a storm isn't a chorus. The voice's "weather" bank gives a
    // forecast/comment for the mapped condition.
    if (R.Discomfort >= 0.4f && FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec))
    {
        static const TCHAR* const Conditions[] = {
            TEXT("clear"), TEXT("cloudy"), TEXT("overcast"), TEXT("rain"), TEXT("storm") };
        const FString Line = FNpcDialogue::WeatherBark(
            Archetype.Voice, Conditions[FMath::Clamp(Severity, 0, 4)],
            CitizenSeed + (BarkCounter++ * 7919));
        if (!Line.IsEmpty())
        {
            SpeakLine(Line);
            TimeSinceBark = 0.0;
            UE_LOG(LogTemp, Verbose, TEXT("[%s>weather] %s"), *Archetype.Name, *Line);
        }
    }
}

void AGTCCitizen::ReleaseClaim()
{
    if (!bHasClaim)
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        if (UGTCPlaceRegistrySubsystem* Places = World->GetSubsystem<UGTCPlaceRegistrySubsystem>())
        {
            Places->NotePlaceReleased(ClaimedPlaceKind, ClaimedPlaceLocation);
        }
    }
    bHasClaim = false;
    ClaimedPlaceKind = NAME_None;
}

void AGTCCitizen::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // A corpse runs no decisions: it just ragdolls and counts down to despawn
    // (SetLifeSpan, set in EnterDeath). Bail before the whole pedestrian pipeline.
    if (bDead)
    {
        return;
    }

    if (!bInitialized)
    {
        InitializeFromSeed(CitizenSeed);
    }

    UWorld* World = GetWorld();
    UGTCCrowdSubsystem* Crowd = World ? World->GetSubsystem<UGTCCrowdSubsystem>() : nullptr;
    UGTCPlaceRegistrySubsystem* Places = World ? World->GetSubsystem<UGTCPlaceRegistrySubsystem>() : nullptr;

    // 1) Clock + drive decay.
    double ElapsedHours = 0.0;
    const double Hour = ResolveHour(DeltaSeconds, Crowd, ElapsedHours);
    if (ElapsedHours > 0.0)
    {
        Needs.Decay(ElapsedHours, DecayRates);
    }

    // 2) Re-decide intent periodically, on first run, or the moment we arrive.
    ReplanAccum += DeltaSeconds;
    if (!bHasGoal || bArrived || ReplanAccum >= ReplanIntervalSec)
    {
        ReplanAccum = 0.0;
        Replan(Hour, Places);
    }

    // 3) React to the player / panic, choosing the brain state.
    bool bThreatActive = false;
    FVector ThreatPos = FVector::ZeroVector;
    BrainState = DecideBrainState(Crowd, bThreatActive, ThreatPos);

    // 3a) Gunpoint duress: on the rising edge of cowering, throw hands up (anim hook)
    //     and blurt a terrified line; the held pose + facing run while IsCowering().
    if (bCowering && !bWasCowering)
    {
        FVector ToThreat = ThreatPos - GetActorLocation();
        ToThreat.Z = 0.0;
        FVector ToThreatDir = ToThreat.GetSafeNormal();
        if (ToThreatDir.IsNearlyZero())
        {
            ToThreatDir = GetActorForwardVector(); // exact overlap — fall back to our facing
        }
        OnCower(ToThreatDir);
        if (FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec))
        {
            const FString Line = FBarkPool::Line(EBarkSituation::FLEE, CitizenSeed + (BarkCounter++ * 7919));
            if (!Line.IsEmpty())
            {
                SpeakLine(Line);
                TimeSinceBark = 0.0;
                UE_LOG(LogTemp, Verbose, TEXT("[%s>cower] %s"), *Archetype.Name, *Line);
            }
        }
    }
    bWasCowering = bCowering;

    // 3b) Physical contact: advance any reaction in flight and sniff for a fresh
    //     body bump from the player this frame.
    UpdateContactReaction(DeltaSeconds, Crowd);

    // 4) Arrival bookkeeping (only when not fleeing — a fleeing NPC isn't "arriving").
    if (bHasGoal && BrainState != FNpcBrain::EState::Flee)
    {
        if (FNpcLocomotion::HasArrived(GetActorLocation(), CurrentGoal, ArriveToleranceCm))
        {
            if (!bArrived)
            {
                bArrived = true;
                SatisfyArrivalNeed();
                IdleWanderAccum = 0.0;
            }
            else if (!bGawking && ActivityLingers(CurrentIntent.Activity))
            {
                // Mill about the POI: every few seconds pick a fresh nearby spot and
                // walk to it, so a knot of loiterers drifts and reshuffles.
                IdleWanderAccum += DeltaSeconds;
                if (IdleWanderAccum >= IdleWanderIntervalSec)
                {
                    IdleWanderAccum = 0.0;

                    // First try to strike up a conversation with a nearby idler;
                    // if one's available, both stop and chat instead of wandering.
                    if (!bChatting && Crowd && !bThreatActive && !FNpcContact::Reacts(ContactReaction))
                    {
                        if (AGTCCitizen* P = Crowd->FindChatPartner(GetActorLocation(), ChatStartRadiusCm, this))
                        {
                            BeginChatWith(P);
                            P->BeginChatWith(this);
                        }
                    }
                    // Otherwise, either stand and DO something human this cycle
                    // (check phone, smoke, people-watch...) or mill to a fresh spot.
                    // The choice uses a dedicated roll so it never perturbs the wander
                    // RNG — where loiterers drift stays a pure function of seed + time.
                    if (!bChatting)
                    {
                        const int32 ActionSeed = CitizenSeed + (IdleActionCounter++ * 7919);
                        const FRandomStream ActionRoll(ActionSeed);
                        if (ActionRoll.FRand() < IdleActionChance)
                        {
                            // Perform a contextual idle action: stay put, hand the token
                            // to the anim layer (montage/additive), no wander this cycle.
                            // Draw the index from the SCRAMBLED stream, not the raw
                            // ActionSeed % N: the constant +7919 per-cycle stride would
                            // otherwise march the fidgets in a fixed cyclic order. This
                            // stays deterministic and never touches the wander Rng.
                            // On the clock with a known trade, do the JOB (a barista
                            // pulls shots, a crossing guard waves traffic) instead of a
                            // generic fidget; otherwise fall back to contextual idle
                            // business. Same scrambled stream either way, so the choice
                            // stays deterministic and never perturbs the wander Rng.
                            const bool bWorking = (CurrentIntent.Activity == TEXT("work"));
                            const FName Role = bWorking ? FNpcOccupation::RoleFor(Archetype.Id) : NAME_None;
                            const bool bDoJob = bWorking && !Role.IsNone();
                            const int32 N = bDoJob
                                ? FNpcOccupation::WorkActionCount(Role)
                                : FNpcIdleAction::Count(CurrentIntent.Activity);
                            const FName Action = (N <= 0)
                                ? NAME_None
                                : (bDoJob
                                    ? FNpcOccupation::WorkAction(Role, ActionRoll.RandHelper(N))
                                    : FNpcIdleAction::Pick(CurrentIntent.Activity, ActionRoll.RandHelper(N)));
                            if (!Action.IsNone())
                            {
                                OnIdleAction(Action);
                                UE_LOG(LogTemp, Verbose, TEXT("[%s>idle] %s"),
                                    *Archetype.Name, *Action.ToString());

                                // Some actions come with a spoken line: on the job it's a
                                // job-flavoured callout ("Oat-milk doom latte!"), otherwise
                                // a quiet idle mutter ("Ugh, no signal."). Keep it sparse
                                // (most cycles silent) so a workplace reads as occasional
                                // announcements over busywork, not a monologue. Draw the
                                // line from the scrambled stream so lines don't cycle in a
                                // fixed order (same reason as the action index above).
                                const int32 LineN = bDoJob
                                    ? FWorkBark::Count(Action)
                                    : FIdleActionBark::Count(Action);
                                // Honour the global bark cadence so a line can't pile
                                // straight onto a recent one (TimeSinceBark is one frame
                                // stale here — immaterial against the multi-second cooldown).
                                if (LineN > 0
                                    && FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec)
                                    && ActionRoll.FRand() < IdleBarkChance)
                                {
                                    const int32 LineIdx = ActionRoll.RandHelper(LineN);
                                    const FString Line = bDoJob
                                        ? FWorkBark::Line(Action, LineIdx)
                                        : FIdleActionBark::Line(Action, LineIdx);
                                    if (!Line.IsEmpty())
                                    {
                                        SpeakLine(Line);
                                        TimeSinceBark = 0.0;
                                        UE_LOG(LogTemp, Verbose, TEXT("[%s>%s] %s"),
                                            *Archetype.Name, bDoJob ? TEXT("work") : TEXT("mutter"), *Line);
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Mill: pick a fresh nearby spot and walk to it, so a knot
                            // of loiterers drifts and reshuffles.
                            const double Ang = Rng.FRandRange(0.0, UE_DOUBLE_TWO_PI);
                            const double R = Rng.FRandRange(IdleWanderRadiusCm * 0.3, IdleWanderRadiusCm);
                            CurrentGoal = GoalAnchor + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 0.0);
                            CurrentGoal.Z = GetActorLocation().Z;
                            bArrived = false;
                            bUsePath = false; // a short hop — steer direct, skip a path query.
                        }
                    }
                }
            }
        }
        else
        {
            bArrived = false;
        }
    }

    // 4b) Conversation: a chatting pair stands, faces each other, and trades lines
    // until it times out — or breaks off the instant something alarming happens.
    if (bChatting)
    {
        AGTCCitizen* Partner = ChatPartner.Get();
        const bool bPartnerGone = !Partner
            || FNpcLocomotion::GroundDistance(GetActorLocation(), Partner->GetActorLocation())
                   > ChatStartRadiusCm * 2.5;
        if (bThreatActive || BrainState == FNpcBrain::EState::Flee || bPartnerGone)
        {
            if (Partner && Partner->ChatPartner.Get() == this)
            {
                Partner->EndChat();
            }
            EndChat();
        }
        else
        {
            BrainState = FNpcBrain::EState::Idle; // stand and talk
            ChatTimer += DeltaSeconds;
            if (ChatTimer >= ChatDurationSec)
            {
                if (Partner->ChatPartner.Get() == this)
                {
                    Partner->EndChat();
                }
                EndChat();
            }
            else
            {
                // Face the person you're talking to.
                const FVector ToP = FNpcLocomotion::FromPlane(FNpcSteering::Ground(
                    FNpcLocomotion::ToPlane(Partner->GetActorLocation())
                    - FNpcLocomotion::ToPlane(GetActorLocation())));
                if (!ToP.IsNearlyZero())
                {
                    const FRotator Target(0.0, ToP.Rotation().Yaw, 0.0);
                    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, 8.0f));
                }
                // Trade conversational lines on a relaxed cadence.
                // Turn-taking via the conversation floor: time-in-chat is sliced into
                // turns, and on each turn exactly one of the pair holds the floor (the
                // lead takes even turns, the other odd). Both sides compute this from
                // the same two identities + shared ChatTimer, so they agree without any
                // messaging — no more talking over each other or unison chatter. We
                // voice once per turn we hold, then wait for our next turn.
                ChatLineTimer += DeltaSeconds;
                const int32 Turn = FConversationFloor::TurnAt(ChatTimer, ChatLineIntervalSec);
                const bool bLead = FConversationFloor::IsLead(
                    CitizenSeed, Partner->GetSeed(), GetUniqueID(), Partner->GetUniqueID());
                if (Turn != LastChatTurn && FConversationFloor::HoldsFloor(bLead, Turn))
                {
                    LastChatTurn = Turn;
                    ChatLineTimer = 0.0;
                    const int32 LineSeed = CitizenSeed + (BarkCounter++ * 7919);
                    // Both sides derive the SAME topic from their seed pair, so the
                    // exchange actually coheres: even turns raise the topic, odd turns
                    // answer THAT topic — not a random reply. If this side opens AND
                    // already knows the other (a regular), it leads with a warm greeting
                    // instead of opening cold.
                    const ENpcChatTopic Topic = FNpcTopic::Shared(CitizenSeed, Partner->GetSeed());
                    FString Line;
                    if ((Turn % 2) == 0)
                    {
                        Line = (bChatPartnerFamiliar && Turn == 0)
                            ? FNpcConversation::Greeting(Archetype.Voice, LineSeed)
                            : FNpcTopic::Line(Topic, /*bReply=*/false, LineSeed);
                    }
                    else
                    {
                        Line = FNpcTopic::Line(Topic, /*bReply=*/true, LineSeed);
                    }
                    if (!Line.IsEmpty())
                    {
                        SpeakLine(Line);
                        UE_LOG(LogTemp, Verbose, TEXT("[%s→chat %s turn %d] %s"),
                            *Archetype.Name, FNpcTopic::Name(Topic), Turn, *Line);
                    }
                }
            }
        }
    }

    // 4c) A physical-contact reaction overrides the baseline for a beat: the
    //     citizen stops its errand and deals with having just been touched.
    bool bContactFacePlayer = false;
    if (FNpcContact::Reacts(ContactReaction))
    {
        if (FNpcContact::IsPanic(ContactReaction))
        {
            // Knocked down or terrified: route through the flee machinery so the
            // body sprints away from where the player hit it — but only with a real
            // contact point, never toward world origin.
            if (!ContactThreatPos.IsZero())
            {
                bThreatActive = true;
                ThreatPos = ContactThreatPos;
            }

            const bool bDownNow = FNpcContact::IsKnockdown(ContactReaction)
                && ReactionTimer > FNpcContact::Duration(ENpcContactReaction::Knockdown) * 0.45;
            BrainState = bDownNow ? FNpcBrain::EState::Idle  // still sprawled on the ground
                                  : FNpcBrain::EState::Flee; // scramble up and bolt
        }
        else if (BrainState != FNpcBrain::EState::Flee)
        {
            // Excuse / Annoyed / Flinch / Confront: stop, turn, and address the
            // player — but never downgrade an active flee from a real threat (a gun
            // trumps a "hey, watch it").
            BrainState = FNpcBrain::EState::Idle;
            bContactFacePlayer = true;

            // A brave citizen doesn't just glare — it squares up and shoves back. While
            // Confronting, track the (moving) player and, on a cadence, throw a
            // retaliatory shove: an anim hook + a short forward lunge + an angry line.
            // Whether the shove knocks the player back is the montage/combat layer's call.
            if (FNpcContact::IsHostile(ContactReaction) && Crowd)
            {
                const FGTCThreatSnapshot Threat = Crowd->GetPlayerThreat();
                if (Threat.bValid)
                {
                    ContactThreatPos = Threat.Position; // face/track the live player
                    FVector Toward = Threat.Position - GetActorLocation();
                    Toward.Z = 0.0;
                    const double DistToPlayer = Toward.Size();
                    const FVector Dir = Toward.GetSafeNormal();

                    ConfrontShoveTimer -= DeltaSeconds;
                    if (ConfrontShoveTimer <= 0.0)
                    {
                        // Consume the beat whether or not we have a clean direction, so a
                        // degenerate (near-overlapping) frame can't bank up negative time
                        // and fire instantly the moment the player separates.
                        ConfrontShoveTimer = ConfrontShoveIntervalSec;
                        if (!Dir.IsNearlyZero())
                        {
                            OnConfrontShove(Dir);
                            // A committed step-in — but only when there's ground to close;
                            // already in their face, just swing in place so we don't lunge
                            // straight through the player. Small Z so the lunge leaves
                            // Walking-mode braking for a beat (the reaction forces Idle).
                            if (DistToPlayer > ContactRadiusCm)
                            {
                                // The launch travels ~speed*airtime unopposed while briefly
                                // airborne; cap the planar speed to the headroom so the lunge
                                // presses in but never carries past arm's length into the player.
                                constexpr double LungeAirTimeSec = 0.18; // ~2*Z/gravity for the Z=90 hop
                                const double LungeSpeed = FMath::Min(
                                    ConfrontLungeSpeed, (DistToPlayer - ContactRadiusCm) / LungeAirTimeSec);
                                LaunchCharacter(Dir * LungeSpeed + FVector(0.0, 0.0, 90.0),
                                    /*bXYOverride*/ false, /*bZOverride*/ true);
                            }
                            // Re-voice the aggression, honouring the global bark cadence.
                            if (FBarkPool::ShouldBark(TimeSinceBark, BarkCooldownSec))
                            {
                                const FString Line = FContactBark::Line(
                                    Archetype.Voice, ENpcContactReaction::Confront,
                                    CitizenSeed + (BarkCounter++ * 7919));
                                if (!Line.IsEmpty())
                                {
                                    SpeakLine(Line);
                                    TimeSinceBark = 0.0;
                                    UE_LOG(LogTemp, Verbose, TEXT("[%s>confront] %s"),
                                        *Archetype.Name, *Line);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 4d) Calm pass-by: a citizen going about its day clocks the player walking
    //     close past — a glance (anim hook) and the occasional greeting. Self-guards
    //     against fear / contact / chat / gawk so it only fires when truly idle-calm.
    MaybeNoticePlayer(DeltaSeconds, Crowd);

    // 8b) Wave to a friend passing in the street — friendships visible in ambient
    //     life. Cooldown-throttled so the crowd scan is cheap and waves stay sparse.
    MaybeGreetAcquaintance(DeltaSeconds, Crowd);

    // 8c) Glance around while walking somewhere, so citizens look alive en route.
    MaybeGlance(DeltaSeconds);

    // 5) Steer + move.
    DriveLocomotion(DeltaSeconds, Crowd, bThreatActive, ThreatPos);

    // 5b) A stopped citizen won't orient to movement, so when it's watching the
    // player — gawking, cowering at gunpoint, or mid contact-reaction (squaring up,
    // apologising, flinching) — turn to face them explicitly.
    {
        FVector FaceAt = FVector::ZeroVector;
        if (bContactFacePlayer && !ContactThreatPos.IsZero())
        {
            FaceAt = ContactThreatPos;
        }
        else if ((bGawking || bCowering) && BrainState != FNpcBrain::EState::Flee && !ThreatPos.IsZero())
        {
            // Face the watched/feared player — but if a contact this frame forced a
            // flee, let the flee own orientation (don't fight it with stale facing).
            FaceAt = ThreatPos;
        }

        if (!FaceAt.IsZero())
        {
            const FVector ToPlayer = FNpcSteering::Ground(
                FNpcLocomotion::ToPlane(FaceAt) - FNpcLocomotion::ToPlane(GetActorLocation()));
            const FVector Look = FNpcLocomotion::FromPlane(ToPlayer);
            if (!Look.IsNearlyZero())
            {
                const FRotator Target(0.0, Look.Rotation().Yaw, 0.0);
                const FRotator Smoothed =
                    FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, 8.0f);
                SetActorRotation(Smoothed);
            }
        }
    }

    // 6) Memory fades; the city forgets after a minute or so.
    MemoryIntensity = FNpcMemory::Decay(MemoryIntensity, DeltaSeconds);

    // 7) Voice.
    MaybeBark(DeltaSeconds, bThreatActive);
}
