// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Character Forge — the pure-core "drop a skeleton in, get a fully-wired
 * character out" brain.
 *
 * The product goal: a creator inserts ONE skeleton (a USkeletalMesh / USkeleton)
 * and the backend automatically connects every gameplay action the rig can
 * support — locomotion, sprint, crouch, weapon hold/aim, hit reactions, ragdoll
 * death, facial lip-sync, eye look, finger pose — then drops a playable pawn into
 * the world. No hand-wiring per character.
 *
 * This file is the scene-free, headless-tested decision layer that makes the
 * "automatically connect" call. It answers two questions with NO engine state:
 *
 *   1. What can this skeleton physically do? (FSkeletonProbe — derived from the
 *      rig's bone names, recognising UE5 Mannequin, Mixamo, MetaHuman, and
 *      generic naming conventions.)
 *   2. Given an intended role, which actions should be wired, and which must be
 *      skipped because the rig lacks the bones for them? (FCharacterWiring::Plan
 *      → FCharacterWiringPlan.)
 *
 * The UCLASS adapter (a later wave: an editor subsystem that reads USkeleton
 * bone names, runs Plan(), then attaches the matching gameplay components and
 * spawns the pawn) is deliberately NOT here — this layer stays pure so the
 * bone→capability heuristics and the role wiring tables are unit-tested headless
 * (Tests/CharacterWiringTest.cpp, prefix GTC.Characters.Wiring). Mirrors the
 * pure-core + adapter split used across the module (see NpcArchetypes,
 * Missions/Editor compilers).
 */

/** Who this character is for — narrows which actions are even relevant. */
enum class ECharacterRole : uint8
{
	/** Player avatar: the full action surface (locomotion, combat, expression). */
	Player,
	/** Ambient citizen: walks, flees, reacts, talks — but never wields weapons. */
	Pedestrian,
	/** Armed NPC (cop, enforcer): the pedestrian surface plus weapon handling. */
	Combatant,
	/** Sits in a vehicle: no on-foot locomotion; death + expression only. */
	VehicleOccupant,
	/** Non-humanoid (dog, animal): moves + reacts, but has no hands for weapons. */
	Creature,
};

/**
 * One wireable gameplay capability. Each maps (in the adapter wave) to a real
 * runtime system: Locomotion→ABP_Unarmed/CharacterMovement, WeaponHold→
 * GTCWeaponComponent, HitReaction→Player/Health, FacialLipSync→the OnSpeak
 * appearance hook, etc. Keep `Count` last.
 */
enum class ECharacterAction : uint8
{
	Locomotion,     // idle / walk / run on foot
	Sprint,         // sustained fast run (player) or flee (NPC)
	Crouch,         // crouched stance + movement
	Jump,           // launch off the ground
	Swim,           // in-water locomotion
	WeaponHold,     // grip a weapon in hand
	WeaponAim,      // aim-down / point a held weapon
	Reload,         // two-handed reload
	MeleeStrike,    // unarmed / melee swing
	HitReaction,    // flinch / stagger when damaged
	Death,          // ragdoll on death
	FacialLipSync,  // mouth movement driven by speech
	EyeLook,        // eye aim toward a target
	FingerPose,     // articulated finger / grip poses
	Count,
};

/** Stable, human-readable label for an action (used in plan summaries + logs). */
const TCHAR* GTCCharacterActionName(ECharacterAction Action);

/** Stable, human-readable label for a role. */
const TCHAR* GTCCharacterRoleName(ECharacterRole Role);

/**
 * Parse a role token typed in the side editor ("player", "ped"/"pedestrian",
 * "combatant"/"cop", "occupant"/"driver"). Case-insensitive; unrecognised input
 * falls back to Pedestrian and clears bOutRecognized (nullptr-safe).
 */
ECharacterRole GTCParseCharacterRole(const FString& Token, bool* bOutRecognized = nullptr);

/** Canonical lowercase token for a role that GTCParseCharacterRole round-trips
 *  (player/pedestrian/combatant/occupant). Use for serialization, not display. */
const TCHAR* GTCCharacterRoleToken(ECharacterRole Role);

/** How a wired action is actually driven at runtime — tells the adapter what to
 *  hook up so "wired" means "driven", not just "allowed". */
enum class EActionDriver : uint8
{
	LocomotionGraph,    // a state in the locomotion anim graph (Move/JumpStart/…)
	Montage,            // a one-shot/looping montage on a named slot
	AdditiveUpperBody,  // an additive pose layered on the upper body
	Component,          // owned + ticked by a gameplay UActorComponent
	BlueprintSeam,      // a BlueprintImplementableEvent hook (OnSpeak, look-at)
};

/**
 * The concrete driver an action binds to. Strings are the REAL names used by the
 * pawns/ABPs in this project (slots "DefaultSlot"/"UpperBody"; locomotion nodes
 * "Move"/"JumpStart"/"Air"/"Land"; component tags "Weapon"→UGTCWeaponComponent,
 * "Health"→UPlayerHealthComponent). Empty where not applicable. Pure data so the
 * mapping is unit-tested; the adapter consumes it to attach components and the
 * ABP layer reads the slot/node names.
 */
struct GTC_UE5_API FActionBinding
{
	ECharacterAction Action = ECharacterAction::Locomotion;
	EActionDriver Driver = EActionDriver::Montage;
	const TCHAR* MontageSlot = TEXT("");
	const TCHAR* AnimNode = TEXT("");
	const TCHAR* ComponentTag = TEXT("");
};

/** The driver binding for an action (stable, exhaustive over ECharacterAction). */
FActionBinding GTCActionBinding(ECharacterAction Action);

/**
 * Suggest a sensible default role for a rig so the side editor can auto-pick:
 * a rig with no hands but feet + spine reads as a Creature; an upright humanoid
 * defaults to Pedestrian; an unusable rig falls back to Pedestrian too. The
 * creator can always override. Declared here (used after FSkeletonProbe below).
 */
struct FSkeletonProbe;
ECharacterRole GTCSuggestRole(const FSkeletonProbe& Probe);

/**
 * What an inserted skeleton physically has, inferred from its bone names.
 *
 * Built by the adapter from a USkeleton's reference skeleton (engine side) but
 * computed here, headlessly, by FromBoneNames so the cross-convention matching
 * (UE5 Mannequin `hand_r`, Mixamo `mixamorig:RightHand`, MetaHuman `FACIAL_*`,
 * generic `RightHand`) is unit-tested without an engine.
 */
struct GTC_UE5_API FSkeletonProbe
{
	bool bHasRoot = false;
	bool bHasPelvis = false;
	bool bHasSpine = false;
	bool bHasHead = false;
	bool bHasLeftHand = false;
	bool bHasRightHand = false;
	bool bHasLeftFoot = false;
	bool bHasRightFoot = false;
	bool bHasFingers = false;
	bool bHasFacialBones = false;
	bool bHasEyeBones = false;
	bool bHasToes = false;
	int32 BoneCount = 0;

	bool HasAnyHand() const { return bHasLeftHand || bHasRightHand; }
	bool HasBothHands() const { return bHasLeftHand && bHasRightHand; }
	bool HasAnyFoot() const { return bHasLeftFoot || bHasRightFoot; }

	/**
	 * Infer the probe from a rig's bone names. Case-insensitive; tolerates
	 * namespace prefixes (`mixamorig:`, `root:`) and the usual l/r side tokens
	 * (`_l`, `_r`, `left`, `right`, `.L`, `.R`). Unknown bones are ignored.
	 */
	static FSkeletonProbe FromBoneNames(const TArray<FString>& BoneNames);
};

/** The wiring decision for a single action. */
struct GTC_UE5_API FCharacterAction
{
	ECharacterAction Action = ECharacterAction::Locomotion;
	/** True if the rig supports it and it was connected for this role. */
	bool bWired = false;
	/** When skipped, the reason (missing bones); empty when wired. */
	FString Note;
};

/**
 * The full result of "drop this skeleton in as this role": the per-action
 * decisions, the warnings worth surfacing in the editor, and whether the
 * character is usable at all.
 */
struct GTC_UE5_API FCharacterWiringPlan
{
	ECharacterRole Role = ECharacterRole::Pedestrian;
	/** One entry per action relevant to the role, in declaration order. */
	TArray<FCharacterAction> Actions;
	/** Human-readable notes for relevant actions that had to be skipped. */
	TArray<FString> Warnings;
	/** True if the rig can fulfil the role's minimum (e.g. it can stand/move). */
	bool bUsable = false;

	bool IsWired(ECharacterAction Action) const;
	int32 WiredCount() const;
	int32 RelevantCount() const { return Actions.Num(); }

	/** One-line digest for the side-editor console, e.g.
	 *  "Player: 11/14 actions wired — skipped FacialLipSync, EyeLook, FingerPose". */
	FString Summary() const;
};

/** The planner: pure functions mapping a probe + role to a wiring plan. */
struct GTC_UE5_API FCharacterWiring
{
	/** The actions that matter for a role, in declaration order. */
	static TArray<ECharacterAction> RelevantActions(ECharacterRole Role);

	/** Decide which relevant actions the rig can support, with skip reasons. */
	static FCharacterWiringPlan Plan(const FSkeletonProbe& Probe, ECharacterRole Role);
};
