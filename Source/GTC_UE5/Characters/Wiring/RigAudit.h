// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FSkeletonProbe;
enum class ECharacterRole : uint8;

/**
 * Rig quality audit — grades an inserted skeleton for AAA suitability, beyond the
 * yes/no capability probe. The Forge can wire an action as soon as the gating
 * bones exist, but a GTA-VI-caliber result needs more: enough spine joints to
 * lean and aim, twist bones so elbows/wrists don't candy-wrap, left/right
 * symmetry, toe bones for foot IK, fingers for grips. This produces a 0-100
 * score plus actionable issues so a creator knows what to fix before shipping a
 * character — not just whether it animates at all.
 *
 * Pure data in / pure report out, headless-tested (Tests/RigAuditTest.cpp,
 * prefix GTC.Characters.Audit). Consumed by the side editor (GTC.Character.Audit).
 */
struct GTC_UE5_API FRigAudit
{
	/** Overall quality, 0 (unusable) .. 100 (clean AAA humanoid). */
	int32 Score = 0;
	/** Problems worth fixing, most severe first. */
	TArray<FString> Issues;
	/** Informational observations (not problems). */
	TArray<FString> Notes;
	/** True when the rig clears the quality bar with no blocking issue. */
	bool bProductionReady = false;

	/** One-line digest, e.g. "Rig score 72/100 — 2 issue(s): ...". */
	FString Summary() const;
};

/**
 * Audit a rig from its capability probe and raw bone names. Symmetric checks and
 * joint-depth counts read the bone names; gating checks read the probe.
 */
FRigAudit GTCAuditRig(const FSkeletonProbe& Probe, const TArray<FString>& BoneNames);

/**
 * Ship-readiness verdict for a rig in a specific role — the "can I add this to
 * the game right now?" gate. Composes the wiring plan (does it meet the role's
 * minimum?), the quality audit (is it good enough?), and role-specific must-haves
 * (an unarmed cop is broken) into a single shippable/blockers answer.
 */
struct GTC_UE5_API FCharacterReadiness
{
	bool bShippable = false;
	ECharacterRole Role{};       // the role this verdict is for
	int32 QualityScore = 0;      // from the rig audit (0..100)
	int32 ActionsWired = 0;
	int32 ActionsRelevant = 0;
	/** Must-fix problems for this role; empty when shippable. */
	TArray<FString> Blockers;

	/** One-line digest, e.g. "Combatant: NOT shippable — cannot hold a weapon". */
	FString Summary() const;
};

/** Assess whether a rig is ready to ship in Role (plan + audit + role rules). */
FCharacterReadiness GTCAssessReadiness(
	const FSkeletonProbe& Probe, const TArray<FString>& BoneNames, ECharacterRole Role);
