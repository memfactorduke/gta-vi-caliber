// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GtcMissionDefinition.generated.h"

/**
 * The Mission Editor's authoring data model — one fully-reflected, serializable
 * description of a piece of game content (a main story mission, a side quest, or an
 * activity / odd-job). This is the "openable" file the in-game god-mode editor reads
 * and writes; GtcMissionJson turns it to/from JSON, GtcMissionEditorSubsystem owns a
 * bank of them, and (later phases) playback translates one into the existing pure
 * Missions/Core + Coordinators runtime so the authored mission actually runs.
 *
 * --- Why this layer is reflected when Missions/Core is pure C++ ---
 * The Core (Mission/MissionChain/MissionObjectiveTypes) and Activities (SideJob/
 * StreetRace) models are deliberately non-reflected plain C++ value types — they can't
 * be UPROPERTYs and aren't designed for a details panel. The editor's data layer is the
 * ONE place we add reflection so a designer gets details-panel editing, Blueprint
 * access and trivial iteration; at playback the editor *translates* these reflected
 * twins into the pure runtime types. The reflected enums below (EGtcObjectiveKind, ...)
 * are likewise the editable mirrors of the Core enums.
 *
 * Conventions (matching the rest of GTC_UE5): F-prefixed USTRUCT, E-prefixed UENUM,
 * GTC_UE5_API export, generated header last, EditAnywhere+BlueprintReadWrite on every
 * authoring field, world-space cm for all FVector/radii (no FloatingOrigin remap).
 */

/** Top-level authoring category. Decides which runtime path playback uses. */
UENUM(BlueprintType)
enum class EGtcMissionContentType : uint8
{
    MainMission UMETA(DisplayName = "Main Mission"),
    SideQuest   UMETA(DisplayName = "Side Quest"),
    Activity    UMETA(DisplayName = "Activity / Odd-Job"),
};

/** For ContentType==Activity, which pure activity model the payload drives. */
UENUM(BlueprintType)
enum class EGtcActivityKind : uint8
{
    None,
    SideJob    UMETA(DisplayName = "Side Job"),
    StreetRace UMETA(DisplayName = "Street Race"),
};

/**
 * Reflected mirror of MissionObjectiveTypes::EKind (which is a pure-C++ enum and so
 * can't be stored as a UPROPERTY). What the player is asked to DO for one objective.
 */
UENUM(BlueprintType)
enum class EGtcObjectiveKind : uint8
{
    Reach     UMETA(DisplayName = "Reach Location"),
    Collect   UMETA(DisplayName = "Collect Items"),
    Eliminate UMETA(DisplayName = "Eliminate Targets"),
    Escort    UMETA(DisplayName = "Escort / Protect"),
    Survive   UMETA(DisplayName = "Survive"),
    Defend    UMETA(DisplayName = "Defend"),
    Interact  UMETA(DisplayName = "Interact"),
    Custom    UMETA(DisplayName = "Custom / Scripted"),
};

/** How an objective is detected as satisfied at runtime (the trigger wiring). */
UENUM(BlueprintType)
enum class EGtcTriggerType : uint8
{
    EnterRadius UMETA(DisplayName = "Enter Radius"),
    Interact    UMETA(DisplayName = "Interact With"),
    KillCount   UMETA(DisplayName = "Kill Count"),
    HoldRadius  UMETA(DisplayName = "Hold Radius"),
    Timer       UMETA(DisplayName = "Timer Elapsed"),
    Manual      UMETA(DisplayName = "Manual / Scripted"),
};

/**
 * What the player earns. Mapped onto UMissionReward tunables (ObjectiveReward /
 * MissionBonus) and UProgressionTracker (Xp) at payout; Money is the headline lump,
 * Respect/Unlocks are GTA-flavour extras. Unlocks beyond the hardcoded progression
 * gate table are advisory until that table is data-driven (validation warns).
 */
USTRUCT(BlueprintType)
struct GTC_UE5_API FGtcRewardDefinition
{
    GENERATED_BODY()

    /** Headline cash for completing the whole mission (granted on completion). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Reward")
    int32 Money = 0;

    /** Per-objective cash drip (overrides UMissionReward::ObjectiveReward default). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Reward")
    int32 ObjectiveReward = 250;

    /** Completion bonus (overrides UMissionReward::MissionBonus default). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Reward")
    int32 MissionBonus = 1000;

    /** Progression XP awarded on completion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Reward")
    int32 Xp = 0;

    /** Street respect / reputation, GTA-style. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Reward")
    int32 Respect = 0;

    /** Feature / item ids unlocked on completion (e.g. a weapon, a safehouse). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Reward")
    TArray<FString> Unlocks;
};

/**
 * One step of a mission. Id is unique within the mission and (when bHasWaypoint) keys
 * the controller's waypoint map — case-sensitive. Kind-specific params are gated by
 * Kind (Radius for Reach/Hold, TargetCount for Collect/Eliminate, etc.). DriverKind is
 * the lower-level objective-driver selector ("reach"/"hold"); an unknown value silently
 * degrades to reach in the runtime, so validation checks it against the known set.
 */
USTRUCT(BlueprintType)
struct GTC_UE5_API FGtcObjectiveDefinition
{
    GENERATED_BODY()

    /** Unique within the mission; also the waypoint-map key. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    FString Id;

    /** HUD label shown to the player ("Reach the docks"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    EGtcObjectiveKind Kind = EGtcObjectiveKind::Reach;

    /** World-space target (cm). Meaningful when bHasWaypoint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    FVector Waypoint = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    bool bHasWaypoint = true;

    /** Reach/Escort/Hold acceptance radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    double Radius = 600.0;

    /** Collect/Eliminate target count (floored at 1 by validation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    int32 TargetCount = 1;

    /** Survive/Hold duration (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    double Duration = 3.0;

    /** Defend/Escort fail threshold (e.g. escortee health floor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    double Threshold = 0.0;

    /** Objective-driver selector: "reach" or "hold". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    FString DriverKind = TEXT("reach");

    /** Optional objectives don't block mission completion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Objective")
    bool bOptional = false;
};

/**
 * How an objective becomes satisfied — the "go here / interact / kill N / hold / wait"
 * wiring the runtime previously left to scripted code. ObjectiveId links it to the
 * objective it completes; params are used per Type.
 */
USTRUCT(BlueprintType)
struct GTC_UE5_API FGtcTriggerDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Trigger")
    FString ObjectiveId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Trigger")
    EGtcTriggerType Type = EGtcTriggerType::EnterRadius;

    /** World-space (cm) for EnterRadius/HoldRadius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Trigger")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Trigger")
    double Radius = 300.0;

    /** Required count for KillCount. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Trigger")
    int32 Count = 1;

    /** Seconds for Timer / HoldRadius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Trigger")
    double Time = 0.0;

    /** Optional link to a placed world marker. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Trigger")
    FGuid TargetMarkerId;
};

/**
 * Activity payload — a courier/delivery-style Side Job. Reflected twin of SideJob::FJob;
 * translated via SideJob::MakeJob at playback. Kind is the integer SideJob::EKind value
 * (kept as int here to avoid coupling the editor model to the pure enum's layout).
 */
USTRUCT(BlueprintType)
struct GTC_UE5_API FGtcSideJobDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|SideJob")
    int32 Kind = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|SideJob")
    FVector Pickup = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|SideJob")
    FVector Dropoff = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|SideJob")
    int32 BaseReward = 0;

    /** Par time for an on-time bonus (seconds; 0 = untimed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|SideJob")
    double ParTime = 0.0;
};

/**
 * Activity payload — a checkpoint Street Race. Reflected twin of the StreetRace model;
 * translated via the StreetRace(Checkpoints, Laps) ctor at playback.
 */
USTRUCT(BlueprintType)
struct GTC_UE5_API FGtcStreetRaceDefinition
{
    GENERATED_BODY()

    /** Ordered checkpoint gates (world cm). Order is the race line. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Race")
    TArray<FVector> Checkpoints;

    /** Lap count (floored at 1 by validation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Race")
    int32 Laps = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission|Race")
    int32 BaseReward = 0;
};

/**
 * One authored mission/quest/activity — the editor's top-level document. MissionId is a
 * stable GUID (used for prerequisite links and de-dup); Id is the human key reused as
 * the runtime chain id. bValid is a runtime sentinel: a definition that failed to parse
 * from JSON has bValid==false and is never serialized back out.
 */
USTRUCT(BlueprintType)
struct GTC_UE5_API FGtcMissionDefinition
{
    GENERATED_BODY()

    /** Stable unique id for links and de-dup (assigned at CreateMission). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FGuid MissionId;

    /** Human key, e.g. "mission_001"; becomes the runtime chain / controller id. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FString Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    EGtcMissionContentType ContentType = EGtcMissionContentType::MainMission;

    /** Quest-giver id (NPC/contact), free-form for now. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FString Giver;

    /** District / region tag for the map and gating. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FString District;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    TArray<FGtcObjectiveDefinition> Objectives;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    TArray<FGtcTriggerDefinition> Triggers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FGtcRewardDefinition Reward;

    /** Whole-mission time limit (seconds); 0 = untimed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    double TimeLimit = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    EGtcActivityKind ActivityKind = EGtcActivityKind::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FGtcSideJobDefinition SideJob;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FGtcStreetRaceDefinition Race;

    /** Missions (by MissionId) that must be complete before this unlocks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    TArray<FGuid> Prerequisites;

    /** Show a world start-marker for this mission (the GTA "walk up to a blip to start" loop). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    bool bAvailableInWorld = false;

    /** Where the start marker sits (world cm). Meaningful when bAvailableInWorld. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    FVector StartLocation = FVector::ZeroVector;

    /** Player gets within this radius (cm) of the start marker to trigger the mission. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    double StartRadius = 400.0;

    /** Replayable activities/side quests can be started again after completion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Mission")
    bool bRepeatable = false;

    /** Runtime sentinel — false means "failed to load"; never serialized true-by-omission. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "GTC|Mission")
    bool bValid = true;
};
