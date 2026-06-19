// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RigAudit.h"
#include "../CharacterWiring.h"

/**
 * Tests for the rig quality audit (prefix GTC.Characters.Audit). A clean AAA
 * humanoid scores high and is production-ready; broken / asymmetric / sparse rigs
 * lose points with actionable issues.
 */

namespace
{
	// A clean humanoid: deep spine, twist bones, toes, fingers, facial + eyes.
	TArray<FString> CleanHumanoid()
	{
		return {
			TEXT("root"), TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"),
			TEXT("neck_01"), TEXT("head"),
			TEXT("clavicle_l"), TEXT("upperarm_l"), TEXT("upperarm_twist_01_l"),
			TEXT("lowerarm_l"), TEXT("lowerarm_twist_01_l"), TEXT("hand_l"),
			TEXT("clavicle_r"), TEXT("upperarm_r"), TEXT("upperarm_twist_01_r"),
			TEXT("lowerarm_r"), TEXT("lowerarm_twist_01_r"), TEXT("hand_r"),
			TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("ball_l"),
			TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r"),
			TEXT("index_01_l"), TEXT("thumb_01_l"), TEXT("index_01_r"), TEXT("thumb_01_r"),
			TEXT("FACIAL_C_Jaw"), TEXT("FACIAL_L_Eye"), TEXT("FACIAL_R_Eye"),
		};
	}

	bool AnyIssueContains(const FRigAudit& A, const TCHAR* Needle)
	{
		for (const FString& I : A.Issues)
		{
			if (I.Contains(Needle)) { return true; }
		}
		return false;
	}

	FRigAudit AuditOf(const TArray<FString>& Bones)
	{
		return GTCAuditRig(FSkeletonProbe::FromBoneNames(Bones), Bones);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRigAuditTest,
	"GTC.Characters.Audit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRigAuditTest::RunTest(const FString& Parameters)
{
	// --- Clean AAA humanoid scores high + production-ready ---------------------
	const FRigAudit Clean = AuditOf(CleanHumanoid());
	TestTrue(TEXT("clean: high score"), Clean.Score >= 90);
	TestTrue(TEXT("clean: production-ready"), Clean.bProductionReady);
	TestEqual(TEXT("clean: no issues"), Clean.Issues.Num(), 0);

	// --- A stub rig (root only) is unusable -----------------------------------
	{
		const FRigAudit Stub = AuditOf({ TEXT("root") });
		TestTrue(TEXT("stub: very low score"), Stub.Score <= 20);
		TestFalse(TEXT("stub: not production-ready"), Stub.bProductionReady);
		TestTrue(TEXT("stub: flags missing pelvis"), AnyIssueContains(Stub, TEXT("pelvis")));
	}

	// --- Missing twist bones is flagged ---------------------------------------
	{
		TArray<FString> NoTwist = CleanHumanoid();
		NoTwist.RemoveAll([](const FString& B) { return B.Contains(TEXT("twist")); });
		const FRigAudit A = AuditOf(NoTwist);
		TestTrue(TEXT("no-twist: flagged"), AnyIssueContains(A, TEXT("twist")));
		TestTrue(TEXT("no-twist: lower than clean"), A.Score < Clean.Score);
	}

	// --- Asymmetry is flagged + scores below clean ----------------------------
	{
		// A right-armed humanoid missing the entire left arm.
		const FRigAudit A = AuditOf({
			TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("head"),
			TEXT("upperarm_r"), TEXT("lowerarm_twist_01_r"), TEXT("hand_r"),
			TEXT("thigh_l"), TEXT("foot_l"), TEXT("thigh_r"), TEXT("foot_r") });
		TestTrue(TEXT("asym: flags asymmetric hands"), AnyIssueContains(A, TEXT("asymmetric")));
		TestTrue(TEXT("asym: below clean"), A.Score < Clean.Score);
	}

	// --- Summary mentions the score -------------------------------------------
	TestTrue(TEXT("summary names the score"), Clean.Summary().Contains(TEXT("/100")));

	// --- Ship-readiness gate --------------------------------------------------
	{
		const TArray<FString> Bones = CleanHumanoid();
		const FSkeletonProbe Probe = FSkeletonProbe::FromBoneNames(Bones);

		const FCharacterReadiness AsPlayer = GTCAssessReadiness(Probe, Bones, ECharacterRole::Player);
		TestTrue(TEXT("clean humanoid ships as Player"), AsPlayer.bShippable);
		TestEqual(TEXT("player: no blockers"), AsPlayer.Blockers.Num(), 0);

		const FCharacterReadiness AsCombatant = GTCAssessReadiness(Probe, Bones, ECharacterRole::Combatant);
		TestTrue(TEXT("clean humanoid ships as Combatant"), AsCombatant.bShippable);

		// A handless humanoid: fine as a pedestrian, broken as a combatant.
		const TArray<FString> NoHands = {
			TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("head"),
			TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("ball_l"),
			TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r"),
			TEXT("FACIAL_C_Jaw"), TEXT("FACIAL_L_Eye"), TEXT("FACIAL_R_Eye") };
		const FSkeletonProbe NoHandProbe = FSkeletonProbe::FromBoneNames(NoHands);

		const FCharacterReadiness PedOk = GTCAssessReadiness(NoHandProbe, NoHands, ECharacterRole::Pedestrian);
		TestTrue(TEXT("handless rig ships as Pedestrian"), PedOk.bShippable);

		const FCharacterReadiness CombatBad = GTCAssessReadiness(NoHandProbe, NoHands, ECharacterRole::Combatant);
		TestFalse(TEXT("handless rig does NOT ship as Combatant"), CombatBad.bShippable);
		bool bWeaponBlocker = false;
		for (const FString& B : CombatBad.Blockers)
		{
			if (B.Contains(TEXT("weapon"))) { bWeaponBlocker = true; }
		}
		TestTrue(TEXT("combatant blocker names the weapon problem"), bWeaponBlocker);

		// A stub rig ships as nothing.
		const TArray<FString> Stub = { TEXT("root") };
		const FCharacterReadiness StubR = GTCAssessReadiness(
			FSkeletonProbe::FromBoneNames(Stub), Stub, ECharacterRole::Pedestrian);
		TestFalse(TEXT("stub not shippable"), StubR.bShippable);
		TestTrue(TEXT("stub has blockers"), StubR.Blockers.Num() > 0);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
