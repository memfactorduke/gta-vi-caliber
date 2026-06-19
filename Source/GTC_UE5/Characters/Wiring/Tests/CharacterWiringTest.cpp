// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CharacterWiring.h"

/**
 * Tests for the Character Forge wiring brain (prefix GTC.Characters.Wiring).
 *
 * Covers the two pure stages: bone-name → FSkeletonProbe inference across rig
 * conventions (UE5 Mannequin, Mixamo, MetaHuman, broken/partial rigs), and the
 * probe + role → FCharacterWiringPlan decision tables. No engine state.
 */

namespace
{
	// A UE5-Mannequin-style humanoid plus MetaHuman facial/eye bones and fingers —
	// the "everything present" rig that should light up the full player surface.
	TArray<FString> FullHumanoidBones()
	{
		return {
			TEXT("root"), TEXT("pelvis"),
			TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"),
			TEXT("neck_01"), TEXT("head"),
			TEXT("clavicle_l"), TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"),
			TEXT("clavicle_r"), TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"),
			TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("ball_l"),
			TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r"),
			TEXT("index_01_l"), TEXT("thumb_01_l"), TEXT("index_01_r"), TEXT("thumb_01_r"),
			TEXT("FACIAL_C_Jaw"), TEXT("FACIAL_L_Eye"), TEXT("FACIAL_R_Eye"),
		};
	}

	// A Mixamo export: namespace-prefixed, "Hips" as the root (no "root" bone),
	// no facial or eye bones.
	TArray<FString> MixamoBones()
	{
		return {
			TEXT("mixamorig:Hips"),
			TEXT("mixamorig:Spine"), TEXT("mixamorig:Spine1"), TEXT("mixamorig:Spine2"),
			TEXT("mixamorig:Neck"), TEXT("mixamorig:Head"),
			TEXT("mixamorig:LeftArm"), TEXT("mixamorig:LeftForeArm"), TEXT("mixamorig:LeftHand"),
			TEXT("mixamorig:LeftHandIndex1"), TEXT("mixamorig:LeftHandThumb1"),
			TEXT("mixamorig:RightArm"), TEXT("mixamorig:RightForeArm"), TEXT("mixamorig:RightHand"),
			TEXT("mixamorig:LeftUpLeg"), TEXT("mixamorig:LeftLeg"),
			TEXT("mixamorig:LeftFoot"), TEXT("mixamorig:LeftToeBase"),
			TEXT("mixamorig:RightUpLeg"), TEXT("mixamorig:RightLeg"),
			TEXT("mixamorig:RightFoot"), TEXT("mixamorig:RightToeBase"),
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCharacterWiringTest,
	"GTC.Characters.Wiring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterWiringTest::RunTest(const FString& Parameters)
{
	// --- Probe: full humanoid recognises every part ---------------------------
	{
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(FullHumanoidBones());
		TestTrue(TEXT("full: root"), P.bHasRoot);
		TestTrue(TEXT("full: pelvis"), P.bHasPelvis);
		TestTrue(TEXT("full: spine"), P.bHasSpine);
		TestTrue(TEXT("full: head"), P.bHasHead);
		TestTrue(TEXT("full: left hand"), P.bHasLeftHand);
		TestTrue(TEXT("full: right hand"), P.bHasRightHand);
		TestTrue(TEXT("full: left foot"), P.bHasLeftFoot);
		TestTrue(TEXT("full: right foot"), P.bHasRightFoot);
		TestTrue(TEXT("full: toes"), P.bHasToes);
		TestTrue(TEXT("full: fingers"), P.bHasFingers);
		TestTrue(TEXT("full: facial"), P.bHasFacialBones);
		TestTrue(TEXT("full: eyes"), P.bHasEyeBones);
	}

	// --- Probe: Mixamo (no root, no facial/eye) -------------------------------
	{
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(MixamoBones());
		TestFalse(TEXT("mixamo: no standalone root bone"), P.bHasRoot);
		TestTrue(TEXT("mixamo: Hips reads as pelvis"), P.bHasPelvis);
		TestTrue(TEXT("mixamo: spine"), P.bHasSpine);
		TestTrue(TEXT("mixamo: both hands"), P.HasBothHands());
		TestTrue(TEXT("mixamo: both feet"), P.bHasLeftFoot && P.bHasRightFoot);
		TestTrue(TEXT("mixamo: ToeBase reads as toes"), P.bHasToes);
		TestTrue(TEXT("mixamo: HandIndex/Thumb read as fingers"), P.bHasFingers);
		TestFalse(TEXT("mixamo: no facial bones"), P.bHasFacialBones);
		TestFalse(TEXT("mixamo: no eye bones"), P.bHasEyeBones);
	}

	// --- Side detection is word-boundary aware --------------------------------
	{
		// "lowerarm_l" must read as LEFT (the _l), and "lowerarm" alone must not
		// trip a side. "hand_r" must read right, not left.
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(
			{ TEXT("lowerarm_l"), TEXT("hand_r"), TEXT("foot_l") });
		TestTrue(TEXT("side: hand_r is a right hand"), P.bHasRightHand);
		TestFalse(TEXT("side: no left hand from lowerarm_l alone"), P.bHasLeftHand);
		TestTrue(TEXT("side: foot_l is a left foot"), P.bHasLeftFoot);
	}

	// --- Plan: full humanoid as Player → everything wired ---------------------
	{
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(FullHumanoidBones());
		const FCharacterWiringPlan Plan = FCharacterWiring::Plan(P, ECharacterRole::Player);
		TestTrue(TEXT("player: usable"), Plan.bUsable);
		TestEqual(TEXT("player: all relevant actions wired"), Plan.WiredCount(), Plan.RelevantCount());
		TestEqual(TEXT("player: no warnings"), Plan.Warnings.Num(), 0);
		TestTrue(TEXT("player: lip-sync wired"), Plan.IsWired(ECharacterAction::FacialLipSync));
		TestTrue(TEXT("player: finger pose wired"), Plan.IsWired(ECharacterAction::FingerPose));
		TestTrue(TEXT("player: weapon aim wired"), Plan.IsWired(ECharacterAction::WeaponAim));
	}

	// --- Plan: Mixamo as Player → locomotion/combat yes, face/eye/finger no ---
	{
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(MixamoBones());
		const FCharacterWiringPlan Plan = FCharacterWiring::Plan(P, ECharacterRole::Player);
		TestTrue(TEXT("mixamo player: usable"), Plan.bUsable);
		TestTrue(TEXT("mixamo player: locomotion wired despite no root"), Plan.IsWired(ECharacterAction::Locomotion));
		TestTrue(TEXT("mixamo player: weapon hold wired"), Plan.IsWired(ECharacterAction::WeaponHold));
		TestTrue(TEXT("mixamo player: reload wired (both hands)"), Plan.IsWired(ECharacterAction::Reload));
		TestTrue(TEXT("mixamo player: finger pose wired"), Plan.IsWired(ECharacterAction::FingerPose));
		TestFalse(TEXT("mixamo player: lip-sync skipped (no facial)"), Plan.IsWired(ECharacterAction::FacialLipSync));
		TestFalse(TEXT("mixamo player: eye look skipped (no eyes)"), Plan.IsWired(ECharacterAction::EyeLook));
		TestTrue(TEXT("mixamo player: has warnings for the skips"), Plan.Warnings.Num() >= 2);
	}

	// --- Plan: role narrowing — pedestrian never wields weapons ---------------
	{
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(FullHumanoidBones());
		const FCharacterWiringPlan Ped = FCharacterWiring::Plan(P, ECharacterRole::Pedestrian);
		// Weapon actions are simply not relevant to a pedestrian, so they are
		// absent from the plan (not "wired"), even on a rig that could hold a gun.
		TestFalse(TEXT("pedestrian: weapon hold not in plan"), Ped.IsWired(ECharacterAction::WeaponHold));
		bool bHasWeaponEntry = false;
		for (const FCharacterAction& A : Ped.Actions)
		{
			if (A.Action == ECharacterAction::WeaponHold) { bHasWeaponEntry = true; }
		}
		TestFalse(TEXT("pedestrian: no weapon entry at all"), bHasWeaponEntry);
		TestTrue(TEXT("pedestrian: still usable + walks"), Ped.bUsable);

		const FCharacterWiringPlan Combat = FCharacterWiring::Plan(P, ECharacterRole::Combatant);
		TestTrue(TEXT("combatant: weapon hold wired"), Combat.IsWired(ECharacterAction::WeaponHold));
	}

	// --- Plan: combatant missing the right hand → hold ok, reload skipped -----
	{
		// Left-only arm: one hand present, the other absent.
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(
			{ TEXT("pelvis"), TEXT("spine_01"), TEXT("head"),
			  TEXT("hand_l"), TEXT("foot_l"), TEXT("foot_r") });
		const FCharacterWiringPlan Plan = FCharacterWiring::Plan(P, ECharacterRole::Combatant);
		TestTrue(TEXT("one-hand: weapon hold wired (one hand suffices)"), Plan.IsWired(ECharacterAction::WeaponHold));
		TestFalse(TEXT("one-hand: reload skipped (needs both hands)"), Plan.IsWired(ECharacterAction::Reload));
	}

	// --- Plan: a stub rig (root only) is unusable -----------------------------
	{
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames({ TEXT("root") });
		const FCharacterWiringPlan Plan = FCharacterWiring::Plan(P, ECharacterRole::Pedestrian);
		TestFalse(TEXT("stub: not usable"), Plan.bUsable);
		TestFalse(TEXT("stub: locomotion not wired"), Plan.IsWired(ECharacterAction::Locomotion));
		TestEqual(TEXT("stub: nothing wired"), Plan.WiredCount(), 0);
	}

	// --- Plan: vehicle occupant usable without feet ---------------------------
	{
		// A seated torso: pelvis + spine + head + facial, but no legs/feet.
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(
			{ TEXT("pelvis"), TEXT("spine_01"), TEXT("head"), TEXT("jaw") });
		const FCharacterWiringPlan Occ = FCharacterWiring::Plan(P, ECharacterRole::VehicleOccupant);
		TestTrue(TEXT("occupant: usable without feet"), Occ.bUsable);
		TestTrue(TEXT("occupant: lip-sync wired"), Occ.IsWired(ECharacterAction::FacialLipSync));

		// The same footless torso cannot be a pedestrian (can't stand/move).
		const FCharacterWiringPlan Ped = FCharacterWiring::Plan(P, ECharacterRole::Pedestrian);
		TestFalse(TEXT("footless pedestrian: unusable"), Ped.bUsable);
	}

	// --- Role token parsing (side-editor input) -------------------------------
	{
		bool bOk = false;
		TestEqual(TEXT("parse: player"), GTCParseCharacterRole(TEXT("Player"), &bOk), ECharacterRole::Player);
		TestTrue(TEXT("parse: player recognised"), bOk);
		TestEqual(TEXT("parse: ped alias"), GTCParseCharacterRole(TEXT("ped")), ECharacterRole::Pedestrian);
		TestEqual(TEXT("parse: cop → combatant"), GTCParseCharacterRole(TEXT("cop")), ECharacterRole::Combatant);
		TestEqual(TEXT("parse: driver → occupant"), GTCParseCharacterRole(TEXT("driver")), ECharacterRole::VehicleOccupant);
		TestEqual(TEXT("parse: unknown → pedestrian fallback"),
			GTCParseCharacterRole(TEXT("wizard"), &bOk), ECharacterRole::Pedestrian);
		TestFalse(TEXT("parse: unknown not recognised"), bOk);
	}

	// --- Creature (quadruped) role + auto-suggested role ----------------------
	{
		// A dog-like rig: pelvis + spine + head + four legs + jaw, NO hands.
		const FSkeletonProbe Quad = FSkeletonProbe::FromBoneNames(
			{ TEXT("pelvis"), TEXT("spine_01"), TEXT("head"),
			  TEXT("thigh_l"), TEXT("foot_l"), TEXT("thigh_r"), TEXT("foot_r"),
			  TEXT("tail_01"), TEXT("jaw") });
		TestFalse(TEXT("quadruped: no hands"), Quad.HasAnyHand());

		TestEqual(TEXT("suggest: handless quadruped → Creature"),
			GTCSuggestRole(Quad), ECharacterRole::Creature);

		const FCharacterWiringPlan Plan = FCharacterWiring::Plan(Quad, ECharacterRole::Creature);
		TestTrue(TEXT("creature: usable"), Plan.bUsable);
		TestTrue(TEXT("creature: locomotion wired"), Plan.IsWired(ECharacterAction::Locomotion));
		TestTrue(TEXT("creature: hit reaction wired"), Plan.IsWired(ECharacterAction::HitReaction));
		TestTrue(TEXT("creature: can vocalise (jaw)"), Plan.IsWired(ECharacterAction::FacialLipSync));
		// No hands → weapon actions are not even relevant to a creature.
		bool bHasWeapon = false;
		for (const FCharacterAction& A : Plan.Actions)
		{
			if (A.Action == ECharacterAction::WeaponHold) { bHasWeapon = true; }
		}
		TestFalse(TEXT("creature: no weapon actions"), bHasWeapon);

		// A humanoid auto-suggests Pedestrian, and the creature token round-trips.
		const FSkeletonProbe Humanoid = FSkeletonProbe::FromBoneNames(FullHumanoidBones());
		TestEqual(TEXT("suggest: humanoid → Pedestrian"),
			GTCSuggestRole(Humanoid), ECharacterRole::Pedestrian);
		TestEqual(TEXT("creature token round-trips"),
			GTCParseCharacterRole(GTCCharacterRoleToken(ECharacterRole::Creature)), ECharacterRole::Creature);
	}

	// --- Action binding table (wired means driven) ----------------------------
	{
		// Exhaustive + self-consistent across every action.
		for (int32 i = 0; i < static_cast<int32>(ECharacterAction::Count); ++i)
		{
			const ECharacterAction Act = static_cast<ECharacterAction>(i);
			const FActionBinding B = GTCActionBinding(Act);
			TestEqual(TEXT("binding: action round-trips"), B.Action, Act);
			// A component-driven action must name the component that drives it.
			if (B.Driver == EActionDriver::Component)
			{
				TestTrue(TEXT("binding: component driver has a tag"),
					FCString::Strlen(B.ComponentTag) > 0);
			}
			// A locomotion-graph action must name its graph node.
			if (B.Driver == EActionDriver::LocomotionGraph)
			{
				TestTrue(TEXT("binding: locomotion driver names a node"),
					FCString::Strlen(B.AnimNode) > 0);
			}
		}

		// Concrete, project-real mappings.
		TestEqual(TEXT("binding: locomotion → Move node"),
			FString(GTCActionBinding(ECharacterAction::Locomotion).AnimNode), FString(TEXT("Move")));
		TestEqual(TEXT("binding: jump → JumpStart node"),
			FString(GTCActionBinding(ECharacterAction::Jump).AnimNode), FString(TEXT("JumpStart")));
		TestEqual(TEXT("binding: weapon hold → Weapon component"),
			FString(GTCActionBinding(ECharacterAction::WeaponHold).ComponentTag), FString(TEXT("Weapon")));
		TestEqual(TEXT("binding: reload → Weapon component"),
			FString(GTCActionBinding(ECharacterAction::Reload).ComponentTag), FString(TEXT("Weapon")));
		TestEqual(TEXT("binding: hit reaction → Health component"),
			FString(GTCActionBinding(ECharacterAction::HitReaction).ComponentTag), FString(TEXT("Health")));
		TestEqual(TEXT("binding: death → Health component"),
			FString(GTCActionBinding(ECharacterAction::Death).ComponentTag), FString(TEXT("Health")));
		TestEqual(TEXT("binding: aim layers on UpperBody"),
			FString(GTCActionBinding(ECharacterAction::WeaponAim).MontageSlot), FString(TEXT("UpperBody")));
		TestTrue(TEXT("binding: lip-sync is a Blueprint seam"),
			GTCActionBinding(ECharacterAction::FacialLipSync).Driver == EActionDriver::BlueprintSeam);
		TestTrue(TEXT("binding: eye-look is a Blueprint seam"),
			GTCActionBinding(ECharacterAction::EyeLook).Driver == EActionDriver::BlueprintSeam);
	}

	// --- Summary string is informative ----------------------------------------
	{
		const FSkeletonProbe P = FSkeletonProbe::FromBoneNames(MixamoBones());
		const FString S = FCharacterWiring::Plan(P, ECharacterRole::Player).Summary();
		TestTrue(TEXT("summary: names the role"), S.Contains(TEXT("Player")));
		TestTrue(TEXT("summary: reports skipped actions"), S.Contains(TEXT("skipped")));
		TestTrue(TEXT("summary: mentions FacialLipSync skip"), S.Contains(TEXT("FacialLipSync")));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
