// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "../Wiring/CharacterWiring.h"
#include "CharacterRecipe.h"
#include "GTCCharacterForge.generated.h"

class USkeleton;
class USkeletalMesh;
class ACharacter;

/**
 * UGTCCharacterForge — the UObject adapter for the "drop a skeleton in, get a
 * wired character out" pipeline. The thin engine layer over the headless
 * Characters/Wiring brain (matching the module's pure-core + adapter split, e.g.
 * UGtcMissionEditorSubsystem over GtcMissionBank).
 *
 * Responsibilities:
 *   - bridge a real rig into the brain: read a USkeleton's reference skeleton
 *     into bone names → FSkeletonProbe → FCharacterWiring::Plan;
 *   - apply a plan to a live pawn: set the body mesh + locomotion anim class and
 *     attach the gameplay components the plan calls for (today: the weapon
 *     component when WeaponHold is wired);
 *   - back the console "side editor" (GTC.Character.Inspect / .Apply), which
 *     prints exactly what the rig can do and what it is missing.
 *
 * A UWorldSubsystem (like the mission editor) so it has a world to spawn/find
 * pawns in. Everything is null-guarded and content-free, so the editor-closed
 * headless build stays green. Spawning a fresh AGTCCitizen/AGTCPlayerCharacter
 * straight from the crowd flow is the next wave; this wave wires an existing or
 * caller-provided pawn.
 */
UCLASS()
class GTC_UE5_API UGTCCharacterForge : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// --- Bridge: rig → wiring plan -------------------------------------------

	/** Read a skeleton's reference-skeleton bone names (empty if null). */
	static TArray<FString> BoneNamesOf(const USkeleton* Skeleton);

	/** Probe a skeleton's capabilities from its bone names. */
	static FSkeletonProbe ProbeSkeleton(const USkeleton* Skeleton);

	/** Probe the skeleton a skeletal mesh is built on. */
	static FSkeletonProbe ProbeMesh(const USkeletalMesh* Mesh);

	/** Plan how a mesh would wire up for a role (empty/unusable plan if null).
	 *  Plain C++ (not a UFUNCTION): the pure-core plan/probe structs are
	 *  deliberately unreflected, so the console editor drives this directly. */
	FCharacterWiringPlan PlanForMesh(const USkeletalMesh* Mesh, ECharacterRole Role) const;

	// --- Apply: wire a live pawn ---------------------------------------------

	/**
	 * Wire Target from Body for Role: assign the body skeletal mesh (keeping the
	 * pawn's existing locomotion anim class) and attach the components the plan
	 * calls for (a UGTCWeaponComponent when WeaponHold is wired, unless the pawn
	 * already has one). Returns the plan that was applied; a no-op plan if Target
	 * or Body is null. Bone-gated cosmetic/anim hookups (lip-sync, IK) remain
	 * Blueprint-side seams keyed off the returned plan.
	 */
	FCharacterWiringPlan ApplyToCharacter(ACharacter* Target, USkeletalMesh* Body, ECharacterRole Role) const;

	/**
	 * Spawn a FRESH, fully-wired pawn from a dropped-in skeleton — the "add it to
	 * the game right away" path. Deferred-spawns an AGTCCitizen (the crowd's
	 * configured class if any, so the shared appearance set + AI controller come
	 * along), stamps a deterministic identity from Seed, finishes spawning, then
	 * overrides the body mesh from Body and wires the role's actions via
	 * ApplyToCharacter. Returns the live pawn (null if the world or Body is null);
	 * fills OutPlan with what got wired.
	 */
	ACharacter* SpawnForged(USkeletalMesh* Body, ECharacterRole Role, const FTransform& Where,
		int32 Seed, FCharacterWiringPlan& OutPlan) const;

	/**
	 * The in-memory authored cast — built up via GTC.Character.Add, persisted with
	 * SaveBank, restored with LoadBank, and dropped into the world with SpawnAll.
	 * A plain member (not a UPROPERTY): FCharacterRecipe is pure-core, not a
	 * USTRUCT, and holds no UObject references so GC tracking is unnecessary.
	 */
	TArray<FCharacterRecipe> AuthoredRoster;
};
