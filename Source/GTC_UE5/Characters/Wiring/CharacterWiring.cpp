// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterWiring.h"

// ---------------------------------------------------------------------------
// Labels
// ---------------------------------------------------------------------------

const TCHAR* GTCCharacterActionName(ECharacterAction Action)
{
	switch (Action)
	{
	case ECharacterAction::Locomotion:    return TEXT("Locomotion");
	case ECharacterAction::Sprint:        return TEXT("Sprint");
	case ECharacterAction::Crouch:        return TEXT("Crouch");
	case ECharacterAction::Jump:          return TEXT("Jump");
	case ECharacterAction::Swim:          return TEXT("Swim");
	case ECharacterAction::WeaponHold:    return TEXT("WeaponHold");
	case ECharacterAction::WeaponAim:     return TEXT("WeaponAim");
	case ECharacterAction::Reload:        return TEXT("Reload");
	case ECharacterAction::MeleeStrike:   return TEXT("MeleeStrike");
	case ECharacterAction::HitReaction:   return TEXT("HitReaction");
	case ECharacterAction::Death:         return TEXT("Death");
	case ECharacterAction::FacialLipSync: return TEXT("FacialLipSync");
	case ECharacterAction::EyeLook:       return TEXT("EyeLook");
	case ECharacterAction::FingerPose:    return TEXT("FingerPose");
	default:                              return TEXT("Unknown");
	}
}

const TCHAR* GTCCharacterRoleName(ECharacterRole Role)
{
	switch (Role)
	{
	case ECharacterRole::Player:          return TEXT("Player");
	case ECharacterRole::Pedestrian:      return TEXT("Pedestrian");
	case ECharacterRole::Combatant:       return TEXT("Combatant");
	case ECharacterRole::VehicleOccupant: return TEXT("VehicleOccupant");
	case ECharacterRole::Creature:        return TEXT("Creature");
	default:                              return TEXT("Unknown");
	}
}

const TCHAR* GTCCharacterRoleToken(ECharacterRole Role)
{
	switch (Role)
	{
	case ECharacterRole::Player:          return TEXT("player");
	case ECharacterRole::Pedestrian:      return TEXT("pedestrian");
	case ECharacterRole::Combatant:       return TEXT("combatant");
	case ECharacterRole::VehicleOccupant: return TEXT("occupant");
	case ECharacterRole::Creature:        return TEXT("creature");
	default:                              return TEXT("pedestrian");
	}
}

ECharacterRole GTCParseCharacterRole(const FString& Token, bool* bOutRecognized)
{
	const FString T = Token.ToLower();
	ECharacterRole Role = ECharacterRole::Pedestrian;
	bool bOk = true;

	if (T == TEXT("player"))
	{
		Role = ECharacterRole::Player;
	}
	else if (T == TEXT("ped") || T == TEXT("pedestrian") || T == TEXT("citizen"))
	{
		Role = ECharacterRole::Pedestrian;
	}
	else if (T == TEXT("combatant") || T == TEXT("cop") || T == TEXT("enforcer"))
	{
		Role = ECharacterRole::Combatant;
	}
	else if (T == TEXT("occupant") || T == TEXT("driver") || T == TEXT("passenger"))
	{
		Role = ECharacterRole::VehicleOccupant;
	}
	else if (T == TEXT("creature") || T == TEXT("animal") || T == TEXT("dog"))
	{
		Role = ECharacterRole::Creature;
	}
	else
	{
		bOk = false;
	}

	if (bOutRecognized)
	{
		*bOutRecognized = bOk;
	}
	return Role;
}

FActionBinding GTCActionBinding(ECharacterAction Action)
{
	FActionBinding B;
	B.Action = Action;

	switch (Action)
	{
	// Locomotion family: states in the locomotion anim graph.
	case ECharacterAction::Locomotion:
		B.Driver = EActionDriver::LocomotionGraph; B.AnimNode = TEXT("Move"); break;
	case ECharacterAction::Sprint:
		B.Driver = EActionDriver::LocomotionGraph; B.AnimNode = TEXT("Move"); break;
	case ECharacterAction::Jump:
		B.Driver = EActionDriver::LocomotionGraph; B.AnimNode = TEXT("JumpStart"); break;

	// Held poses: looping montages on the full-body slot.
	case ECharacterAction::Crouch:
		B.Driver = EActionDriver::Montage; B.MontageSlot = TEXT("DefaultSlot"); break;
	case ECharacterAction::Swim:
		B.Driver = EActionDriver::Montage; B.MontageSlot = TEXT("DefaultSlot"); break;
	case ECharacterAction::MeleeStrike:
		B.Driver = EActionDriver::Montage; B.MontageSlot = TEXT("DefaultSlot"); break;

	// Weapon family: the weapon component owns hold; aim layers additively; reload
	// is an upper-body montage. All gated on the weapon component.
	case ECharacterAction::WeaponHold:
		B.Driver = EActionDriver::Component; B.ComponentTag = TEXT("Weapon"); break;
	case ECharacterAction::WeaponAim:
		B.Driver = EActionDriver::AdditiveUpperBody; B.MontageSlot = TEXT("UpperBody"); B.ComponentTag = TEXT("Weapon"); break;
	case ECharacterAction::Reload:
		B.Driver = EActionDriver::Montage; B.MontageSlot = TEXT("UpperBody"); B.ComponentTag = TEXT("Weapon"); break;

	// Damage family: driven by the health component (flinch montage / ragdoll).
	case ECharacterAction::HitReaction:
		B.Driver = EActionDriver::Montage; B.MontageSlot = TEXT("UpperBody"); B.ComponentTag = TEXT("Health"); break;
	case ECharacterAction::Death:
		B.Driver = EActionDriver::Component; B.ComponentTag = TEXT("Health"); break;

	// Expression: Blueprint seams (OnSpeak lip-sync, look-at) + finger grip pose.
	case ECharacterAction::FacialLipSync:
		B.Driver = EActionDriver::BlueprintSeam; break;
	case ECharacterAction::EyeLook:
		B.Driver = EActionDriver::BlueprintSeam; break;
	case ECharacterAction::FingerPose:
		B.Driver = EActionDriver::AdditiveUpperBody; B.MontageSlot = TEXT("UpperBody"); break;

	default:
		B.Driver = EActionDriver::BlueprintSeam; break;
	}

	return B;
}

// ---------------------------------------------------------------------------
// Bone-name → capability inference
// ---------------------------------------------------------------------------

namespace
{
	// Strip a rig namespace prefix ("mixamorig:Hips" -> "hips") and lower-case.
	FString NormalizeBone(const FString& Raw)
	{
		FString S = Raw;
		int32 Colon = INDEX_NONE;
		if (S.FindLastChar(TEXT(':'), Colon))
		{
			S = S.RightChop(Colon + 1);
		}
		return S.ToLower();
	}

	bool Has(const FString& Bone, const TCHAR* Needle)
	{
		return Bone.Contains(Needle);
	}

	// Side detection covers the common conventions: trailing/embedded "_l"/"_r",
	// ".l"/".r", and the words "left"/"right". Word-boundary aware so "lowerarm"
	// (an arm segment, not a side) does not read as left.
	bool IsLeftSide(const FString& Bone)
	{
		if (Has(Bone, TEXT("left")))
		{
			return true;
		}
		return Bone.EndsWith(TEXT("_l")) || Bone.EndsWith(TEXT(".l"))
			|| Bone.Contains(TEXT("_l_")) || Bone.Contains(TEXT(".l."));
	}

	bool IsRightSide(const FString& Bone)
	{
		if (Has(Bone, TEXT("right")))
		{
			return true;
		}
		return Bone.EndsWith(TEXT("_r")) || Bone.EndsWith(TEXT(".r"))
			|| Bone.Contains(TEXT("_r_")) || Bone.Contains(TEXT(".r."));
	}

	bool IsHandBone(const FString& Bone)
	{
		// "hand" covers UE (hand_l), Mixamo (LeftHand), generic (RightHand).
		// Exclude finger sub-bones that embed "hand" (Mixamo LeftHandIndex1) so a
		// hand flag means the wrist/palm bone, not a finger.
		return Has(Bone, TEXT("hand"))
			&& !Has(Bone, TEXT("index")) && !Has(Bone, TEXT("middle"))
			&& !Has(Bone, TEXT("ring")) && !Has(Bone, TEXT("pinky"))
			&& !Has(Bone, TEXT("thumb"));
	}

	bool IsFootBone(const FString& Bone)
	{
		// "foot" (UE/Mixamo) or "ankle" (some generic rigs). "ball"/"toe" handled
		// separately as toes, not feet.
		return Has(Bone, TEXT("foot")) || Has(Bone, TEXT("ankle"));
	}

	bool IsToeBone(const FString& Bone)
	{
		return Has(Bone, TEXT("toe")) || Has(Bone, TEXT("ball"));
	}

	bool IsFingerBone(const FString& Bone)
	{
		return Has(Bone, TEXT("thumb")) || Has(Bone, TEXT("index"))
			|| Has(Bone, TEXT("middle")) || Has(Bone, TEXT("ring"))
			|| Has(Bone, TEXT("pinky")) || Has(Bone, TEXT("finger"));
	}

	bool IsFacialBone(const FString& Bone)
	{
		// Jaw/teeth/tongue/lip drive lip-sync; MetaHuman prefixes FACIAL_.
		return Has(Bone, TEXT("jaw")) || Has(Bone, TEXT("facial"))
			|| Has(Bone, TEXT("teeth")) || Has(Bone, TEXT("tongue"))
			|| Has(Bone, TEXT("lip"));
	}

	bool IsEyeBone(const FString& Bone)
	{
		// "eye" but not "eyebrow"/"eyelid" facial detail (those are lip-sync-ish,
		// not gaze). Gaze needs an eyeball pivot.
		return Has(Bone, TEXT("eye"))
			&& !Has(Bone, TEXT("eyebrow")) && !Has(Bone, TEXT("eyelid"))
			&& !Has(Bone, TEXT("eyelash"));
	}
}

FSkeletonProbe FSkeletonProbe::FromBoneNames(const TArray<FString>& BoneNames)
{
	FSkeletonProbe Probe;
	Probe.BoneCount = BoneNames.Num();

	for (const FString& Raw : BoneNames)
	{
		const FString Bone = NormalizeBone(Raw);
		if (Bone.IsEmpty())
		{
			continue;
		}

		if (Bone == TEXT("root") || Has(Bone, TEXT("root")))
		{
			Probe.bHasRoot = true;
		}
		if (Has(Bone, TEXT("pelvis")) || Has(Bone, TEXT("hips")) || Bone == TEXT("hip"))
		{
			Probe.bHasPelvis = true;
		}
		if (Has(Bone, TEXT("spine")) || Has(Bone, TEXT("chest")) || Has(Bone, TEXT("torso")))
		{
			Probe.bHasSpine = true;
		}
		if (Has(Bone, TEXT("head")))
		{
			Probe.bHasHead = true;
		}
		if (IsHandBone(Bone))
		{
			if (IsRightSide(Bone)) { Probe.bHasRightHand = true; }
			if (IsLeftSide(Bone))  { Probe.bHasLeftHand = true; }
		}
		if (IsFootBone(Bone))
		{
			if (IsRightSide(Bone)) { Probe.bHasRightFoot = true; }
			if (IsLeftSide(Bone))  { Probe.bHasLeftFoot = true; }
		}
		if (IsToeBone(Bone))    { Probe.bHasToes = true; }
		if (IsFingerBone(Bone)) { Probe.bHasFingers = true; }
		if (IsFacialBone(Bone)) { Probe.bHasFacialBones = true; }
		if (IsEyeBone(Bone))    { Probe.bHasEyeBones = true; }
	}

	return Probe;
}

// ---------------------------------------------------------------------------
// Planner
// ---------------------------------------------------------------------------

TArray<ECharacterAction> FCharacterWiring::RelevantActions(ECharacterRole Role)
{
	switch (Role)
	{
	case ECharacterRole::Player:
		// The full surface — the player can do everything the rig allows.
		return {
			ECharacterAction::Locomotion, ECharacterAction::Sprint,
			ECharacterAction::Crouch, ECharacterAction::Jump, ECharacterAction::Swim,
			ECharacterAction::WeaponHold, ECharacterAction::WeaponAim,
			ECharacterAction::Reload, ECharacterAction::MeleeStrike,
			ECharacterAction::HitReaction, ECharacterAction::Death,
			ECharacterAction::FacialLipSync, ECharacterAction::EyeLook,
			ECharacterAction::FingerPose,
		};

	case ECharacterRole::Combatant:
		// An armed NPC: the pedestrian surface plus weapon handling.
		return {
			ECharacterAction::Locomotion, ECharacterAction::Sprint,
			ECharacterAction::WeaponHold, ECharacterAction::WeaponAim,
			ECharacterAction::Reload, ECharacterAction::MeleeStrike,
			ECharacterAction::HitReaction, ECharacterAction::Death,
			ECharacterAction::FacialLipSync, ECharacterAction::EyeLook,
		};

	case ECharacterRole::Pedestrian:
		// Walks, flees (sprint), reacts, dies, talks — never wields weapons.
		return {
			ECharacterAction::Locomotion, ECharacterAction::Sprint,
			ECharacterAction::HitReaction, ECharacterAction::Death,
			ECharacterAction::FacialLipSync, ECharacterAction::EyeLook,
		};

	case ECharacterRole::VehicleOccupant:
		// Seated: no on-foot locomotion; just expression + a death ragdoll.
		return {
			ECharacterAction::Death, ECharacterAction::FacialLipSync,
			ECharacterAction::EyeLook,
		};

	case ECharacterRole::Creature:
		// Non-humanoid: moves, bolts, reacts, dies, and can vocalise/look — but
		// has no hands, so no weapon handling and no finger poses.
		return {
			ECharacterAction::Locomotion, ECharacterAction::Sprint,
			ECharacterAction::HitReaction, ECharacterAction::Death,
			ECharacterAction::FacialLipSync, ECharacterAction::EyeLook,
		};

	default:
		return {};
	}
}

ECharacterRole GTCSuggestRole(const FSkeletonProbe& Probe)
{
	// No hands but it can stand and move → a non-humanoid creature.
	if (!Probe.HasAnyHand() && Probe.HasAnyFoot() && Probe.bHasSpine)
	{
		return ECharacterRole::Creature;
	}
	// Otherwise treat it as an ambient humanoid; the creator can promote it to
	// Player/Combatant in the side editor.
	return ECharacterRole::Pedestrian;
}

namespace
{
	// Decide one action against the probe. Returns whether the rig supports it
	// and, when not, fills OutReason with the missing requirement.
	bool CanWire(ECharacterAction Action, const FSkeletonProbe& P, FString& OutReason)
	{
		switch (Action)
		{
		case ECharacterAction::Locomotion:
			// A standalone "root" bone enables root motion but is not required to
			// stand and move (Mixamo rigs root at "Hips" with no root bone).
			if (!P.bHasPelvis)   { OutReason = TEXT("no pelvis/hips bone"); return false; }
			if (!P.HasAnyFoot()) { OutReason = TEXT("no foot bones"); return false; }
			return true;

		case ECharacterAction::Sprint:
			// Sprint rides the locomotion graph.
			if (!P.bHasPelvis || !P.HasAnyFoot())
			{
				OutReason = TEXT("requires locomotion (pelvis + foot)");
				return false;
			}
			return true;

		case ECharacterAction::Crouch:
			if (!P.bHasPelvis || !P.HasAnyFoot()) { OutReason = TEXT("no pelvis/foot"); return false; }
			if (!P.bHasSpine) { OutReason = TEXT("no spine to bend"); return false; }
			return true;

		case ECharacterAction::Jump:
			if (!P.bHasPelvis) { OutReason = TEXT("no pelvis to launch from"); return false; }
			return true;

		case ECharacterAction::Swim:
			if (!P.bHasPelvis || !P.bHasSpine) { OutReason = TEXT("no pelvis/spine"); return false; }
			return true;

		case ECharacterAction::WeaponHold:
			if (!P.HasAnyHand()) { OutReason = TEXT("no hand bone to grip with"); return false; }
			return true;

		case ECharacterAction::WeaponAim:
			if (!P.HasAnyHand()) { OutReason = TEXT("no hand bone"); return false; }
			if (!P.bHasSpine)    { OutReason = TEXT("no spine to aim from"); return false; }
			if (!P.bHasHead)     { OutReason = TEXT("no head to sight along"); return false; }
			return true;

		case ECharacterAction::Reload:
			if (!P.HasBothHands()) { OutReason = TEXT("needs both hands"); return false; }
			return true;

		case ECharacterAction::MeleeStrike:
			if (!P.HasAnyHand()) { OutReason = TEXT("no hand/arm to strike with"); return false; }
			return true;

		case ECharacterAction::HitReaction:
			if (!P.bHasSpine) { OutReason = TEXT("no spine to flinch"); return false; }
			return true;

		case ECharacterAction::Death:
			// Ragdoll needs enough body for a believable physics asset.
			if (!P.bHasPelvis || !P.bHasSpine || !P.HasAnyFoot())
			{
				OutReason = TEXT("too few bones for a ragdoll (need pelvis + spine + foot)");
				return false;
			}
			return true;

		case ECharacterAction::FacialLipSync:
			if (!P.bHasFacialBones) { OutReason = TEXT("no jaw/facial bones"); return false; }
			return true;

		case ECharacterAction::EyeLook:
			if (!P.bHasEyeBones) { OutReason = TEXT("no eye bones"); return false; }
			return true;

		case ECharacterAction::FingerPose:
			if (!P.bHasFingers) { OutReason = TEXT("no finger bones"); return false; }
			return true;

		default:
			OutReason = TEXT("unknown action");
			return false;
		}
	}
}

FCharacterWiringPlan FCharacterWiring::Plan(const FSkeletonProbe& Probe, ECharacterRole Role)
{
	FCharacterWiringPlan PlanOut;
	PlanOut.Role = Role;

	for (ECharacterAction Action : RelevantActions(Role))
	{
		FCharacterAction Decision;
		Decision.Action = Action;

		FString Reason;
		Decision.bWired = CanWire(Action, Probe, Reason);
		if (!Decision.bWired)
		{
			Decision.Note = Reason;
			PlanOut.Warnings.Add(FString::Printf(
				TEXT("%s skipped: %s"), GTCCharacterActionName(Action), *Reason));
		}

		PlanOut.Actions.Add(MoveTemp(Decision));
	}

	// "Usable" = the rig can fulfil the role's minimum. On-foot roles must be able
	// to stand and move; a seated occupant only needs a body to ragdoll/emote.
	if (Role == ECharacterRole::VehicleOccupant)
	{
		PlanOut.bUsable = Probe.bHasPelvis && Probe.bHasSpine;
	}
	else
	{
		PlanOut.bUsable = PlanOut.IsWired(ECharacterAction::Locomotion);
	}

	return PlanOut;
}

// ---------------------------------------------------------------------------
// Plan queries
// ---------------------------------------------------------------------------

bool FCharacterWiringPlan::IsWired(ECharacterAction Action) const
{
	for (const FCharacterAction& A : Actions)
	{
		if (A.Action == Action)
		{
			return A.bWired;
		}
	}
	return false;
}

int32 FCharacterWiringPlan::WiredCount() const
{
	int32 N = 0;
	for (const FCharacterAction& A : Actions)
	{
		if (A.bWired)
		{
			++N;
		}
	}
	return N;
}

FString FCharacterWiringPlan::Summary() const
{
	TArray<FString> Skipped;
	for (const FCharacterAction& A : Actions)
	{
		if (!A.bWired)
		{
			Skipped.Add(GTCCharacterActionName(A.Action));
		}
	}

	FString Line = FString::Printf(TEXT("%s: %d/%d actions wired"),
		GTCCharacterRoleName(Role), WiredCount(), Actions.Num());
	if (!bUsable)
	{
		Line += TEXT(" [UNUSABLE]");
	}
	if (Skipped.Num() > 0)
	{
		Line += FString::Printf(TEXT(" — skipped %s"), *FString::Join(Skipped, TEXT(", ")));
	}
	return Line;
}
