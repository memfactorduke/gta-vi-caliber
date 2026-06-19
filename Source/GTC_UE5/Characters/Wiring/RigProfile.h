// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Rig identification + canonical bone resolution — the backend intelligence that
 * lets the Character Forge accept ANY skeleton and still bind the shared anim
 * graph, weapon sockets, IK and look-at targets to the right bones.
 *
 * Different DCC pipelines name the same joint differently (UE5 Mannequin
 * `hand_r`, Mixamo `mixamorig:RightHand`, MetaHuman's UE-style body + `FACIAL_*`
 * head). To retarget one locomotion ABP onto a dropped-in rig, and to attach a
 * weapon to "the right hand", the Forge needs the rig's CONVENTION and a map from
 * a canonical slot (RightHand, Jaw, …) to the rig's actual bone name. This is
 * that map — scene-free and headless-tested (Tests/RigProfileTest.cpp, prefix
 * GTC.Characters.Rig), consumed by the UGTCCharacterForge adapter.
 */

/** The naming convention a rig follows (best-effort from its bone names). */
enum class ERigConvention : uint8
{
	Unknown,        // no bones, or nothing recognisable
	UE5Mannequin,   // root/pelvis/spine_01/clavicle_l/hand_r/thigh_l…
	Mixamo,         // mixamorig:Hips/Spine/RightHand/LeftUpLeg…
	MetaHuman,      // UE5-style body + FACIAL_* head bones
	Generic,        // humanoid but an unrecognised naming scheme
};

/** A convention-independent joint the engine wants to find on the rig. */
enum class ECanonicalBone : uint8
{
	Root,
	Pelvis,
	Spine,
	Neck,
	Head,
	LeftHand,
	RightHand,
	LeftFoot,
	RightFoot,
	Jaw,
	LeftEye,
	RightEye,
	Count,
};

/** Stable label for a convention (logs / side editor). */
const TCHAR* GTCRigConventionName(ERigConvention Convention);

/** Stable label for a canonical bone. */
const TCHAR* GTCCanonicalBoneName(ECanonicalBone Bone);

/** Best-effort detection of a rig's naming convention from its bone names. */
ERigConvention GTCDetectRigConvention(const TArray<FString>& BoneNames);

/**
 * Resolve a canonical joint to the rig's ACTUAL bone name (returned verbatim,
 * keeping any namespace prefix so an engine lookup matches), or empty if the rig
 * has no such bone. Matching is convention-agnostic: it recognises the UE,
 * Mixamo, MetaHuman and generic spellings and the usual l/r side tokens.
 */
FString GTCResolveCanonicalBone(ECanonicalBone Bone, const TArray<FString>& BoneNames);
