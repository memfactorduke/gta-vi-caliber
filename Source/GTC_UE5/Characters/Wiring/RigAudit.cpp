// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigAudit.h"
#include "CharacterWiring.h"

namespace
{
	FString Norm(const FString& Raw)
	{
		FString S = Raw;
		int32 Colon = INDEX_NONE;
		if (S.FindLastChar(TEXT(':'), Colon))
		{
			S = S.RightChop(Colon + 1);
		}
		return S.ToLower();
	}

	// Count bones whose normalized name contains Needle.
	int32 CountContaining(const TArray<FString>& Bones, const TCHAR* Needle)
	{
		int32 N = 0;
		for (const FString& B : Bones)
		{
			if (Norm(B).Contains(Needle))
			{
				++N;
			}
		}
		return N;
	}
}

FString FRigAudit::Summary() const
{
	return FString::Printf(TEXT("Rig score %d/100%s — %d issue(s)%s"),
		Score,
		bProductionReady ? TEXT(" [production-ready]") : TEXT(""),
		Issues.Num(),
		Issues.Num() > 0 ? *FString::Printf(TEXT(": %s"), *FString::Join(Issues, TEXT("; "))) : TEXT(""));
}

FRigAudit GTCAuditRig(const FSkeletonProbe& Probe, const TArray<FString>& BoneNames)
{
	FRigAudit A;
	int32 Score = 100;

	// --- Blocking structure ---------------------------------------------------
	if (!Probe.bHasPelvis)
	{
		A.Issues.Add(TEXT("no pelvis/hips — cannot anchor the body"));
		Score -= 40;
	}
	if (!Probe.HasAnyFoot())
	{
		A.Issues.Add(TEXT("no foot bones — cannot stand or walk"));
		Score -= 25;
	}
	if (!Probe.bHasSpine)
	{
		A.Issues.Add(TEXT("no spine — no lean, aim, or hit reactions"));
		Score -= 20;
	}
	if (!Probe.bHasHead)
	{
		A.Issues.Add(TEXT("no head bone — no look-at or aim sighting"));
		Score -= 10;
	}

	// --- Joint depth / deform quality ----------------------------------------
	const int32 SpineCount = CountContaining(BoneNames, TEXT("spine"));
	if (Probe.bHasSpine && SpineCount < 3)
	{
		A.Issues.Add(FString::Printf(
			TEXT("shallow spine (%d joint(s)) — limited lean/aim arc"), SpineCount));
		Score -= 8;
	}

	const bool bHasTwist =
		CountContaining(BoneNames, TEXT("twist")) > 0 ||
		CountContaining(BoneNames, TEXT("roll")) > 0;
	if (!bHasTwist && Probe.HasAnyHand())
	{
		A.Issues.Add(TEXT("no twist/roll bones — forearm/upper-arm will candy-wrap"));
		Score -= 8;
	}

	// --- Symmetry -------------------------------------------------------------
	if (Probe.bHasLeftHand != Probe.bHasRightHand)
	{
		A.Issues.Add(TEXT("asymmetric hands — only one arm is rigged"));
		Score -= 10;
	}
	if (Probe.bHasLeftFoot != Probe.bHasRightFoot)
	{
		A.Issues.Add(TEXT("asymmetric feet — only one leg is rigged"));
		Score -= 10;
	}

	// --- Polish details (smaller deductions) ----------------------------------
	if (Probe.HasAnyFoot() && !Probe.bHasToes)
	{
		A.Issues.Add(TEXT("no toe/ball bones — foot roll & plant IK limited"));
		Score -= 5;
	}
	if (Probe.HasAnyHand() && !Probe.bHasFingers)
	{
		A.Notes.Add(TEXT("no finger bones — grips use a static hand pose"));
		Score -= 4;
	}
	if (!Probe.bHasFacialBones)
	{
		A.Notes.Add(TEXT("no facial bones — lip-sync falls back to jaw-only/none"));
		Score -= 3;
	}
	if (!Probe.bHasEyeBones)
	{
		A.Notes.Add(TEXT("no eye bones — gaze/look-at disabled"));
		Score -= 2;
	}

	// --- Bone-budget sanity ---------------------------------------------------
	if (Probe.BoneCount > 0 && Probe.BoneCount < 15)
	{
		A.Issues.Add(FString::Printf(
			TEXT("very low bone count (%d) — likely a placeholder/low-detail rig"), Probe.BoneCount));
		Score -= 6;
	}
	else if (Probe.BoneCount >= 200)
	{
		A.Notes.Add(FString::Printf(
			TEXT("high bone count (%d) — rich (MetaHuman-class) rig"), Probe.BoneCount));
	}

	A.Score = FMath::Clamp(Score, 0, 100);
	// Production-ready: clears the bar and carries no blocking structural issue.
	A.bProductionReady = A.Score >= 75
		&& Probe.bHasPelvis && Probe.HasAnyFoot() && Probe.bHasSpine && Probe.bHasHead;
	return A;
}

FString FCharacterReadiness::Summary() const
{
	FString S = FString::Printf(TEXT("%s: %s — quality %d/100, %d/%d actions"),
		GTCCharacterRoleName(Role),
		bShippable ? TEXT("SHIPPABLE") : TEXT("NOT shippable"),
		QualityScore, ActionsWired, ActionsRelevant);
	if (Blockers.Num() > 0)
	{
		S += FString::Printf(TEXT(" — %s"), *FString::Join(Blockers, TEXT("; ")));
	}
	return S;
}

FCharacterReadiness GTCAssessReadiness(
	const FSkeletonProbe& Probe, const TArray<FString>& BoneNames, ECharacterRole Role)
{
	FCharacterReadiness R;
	R.Role = Role;

	const FCharacterWiringPlan Plan = FCharacterWiring::Plan(Probe, Role);
	const FRigAudit Audit = GTCAuditRig(Probe, BoneNames);
	R.QualityScore = Audit.Score;
	R.ActionsWired = Plan.WiredCount();
	R.ActionsRelevant = Plan.RelevantCount();

	// Can it even fulfil the role's minimum?
	if (!Plan.bUsable)
	{
		R.Blockers.Add(FString::Printf(
			TEXT("rig cannot fulfil the %s minimum"), GTCCharacterRoleName(Role)));
	}

	// Role-specific must-haves: an armed role that can't hold a weapon is broken.
	if (Role == ECharacterRole::Combatant || Role == ECharacterRole::Player)
	{
		if (!Plan.IsWired(ECharacterAction::WeaponHold))
		{
			R.Blockers.Add(TEXT("cannot hold a weapon (no hand bone)"));
		}
	}

	// Quality floor — even a wired rig below this reads as broken in motion.
	if (R.QualityScore < 60)
	{
		R.Blockers.Add(FString::Printf(TEXT("rig quality too low (%d/100)"), R.QualityScore));
	}

	R.bShippable = R.Blockers.Num() == 0;
	return R;
}
