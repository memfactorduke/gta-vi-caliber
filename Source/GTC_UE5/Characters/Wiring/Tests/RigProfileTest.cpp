// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RigProfile.h"

/**
 * Tests for rig-convention detection + canonical bone resolution (prefix
 * GTC.Characters.Rig). The Forge relies on these to retarget the shared anim
 * graph and bind sockets across UE5 Mannequin / Mixamo / MetaHuman / generic rigs.
 */

namespace
{
	TArray<FString> UE5Bones()
	{
		return {
			TEXT("root"), TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"),
			TEXT("neck_01"), TEXT("head"),
			TEXT("clavicle_l"), TEXT("hand_l"), TEXT("clavicle_r"), TEXT("hand_r"),
			TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"),
			TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"),
		};
	}

	TArray<FString> MetaHumanBones()
	{
		TArray<FString> B = UE5Bones();
		B.Append({ TEXT("FACIAL_C_Jaw"), TEXT("FACIAL_L_Eye"), TEXT("FACIAL_R_Eye") });
		return B;
	}

	TArray<FString> MixamoBones()
	{
		return {
			TEXT("mixamorig:Hips"), TEXT("mixamorig:Spine"), TEXT("mixamorig:Head"),
			TEXT("mixamorig:LeftHand"), TEXT("mixamorig:RightHand"),
			TEXT("mixamorig:LeftFoot"), TEXT("mixamorig:RightFoot"),
			TEXT("mixamorig:LeftUpLeg"), TEXT("mixamorig:RightUpLeg"),
		};
	}

	TArray<FString> GenericBones()
	{
		return { TEXT("hips"), TEXT("spine"), TEXT("head"),
			TEXT("LeftHand"), TEXT("RightHand"), TEXT("LeftFoot"), TEXT("RightFoot") };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRigProfileTest,
	"GTC.Characters.Rig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRigProfileTest::RunTest(const FString& Parameters)
{
	// --- Convention detection -------------------------------------------------
	TestEqual(TEXT("UE5 mannequin detected"),
		GTCDetectRigConvention(UE5Bones()), ERigConvention::UE5Mannequin);
	TestEqual(TEXT("MetaHuman detected (FACIAL_ bones)"),
		GTCDetectRigConvention(MetaHumanBones()), ERigConvention::MetaHuman);
	TestEqual(TEXT("Mixamo detected"),
		GTCDetectRigConvention(MixamoBones()), ERigConvention::Mixamo);
	TestEqual(TEXT("generic humanoid detected"),
		GTCDetectRigConvention(GenericBones()), ERigConvention::Generic);
	TestEqual(TEXT("empty rig is unknown"),
		GTCDetectRigConvention({}), ERigConvention::Unknown);

	// --- Canonical resolution: UE5 --------------------------------------------
	{
		const TArray<FString> B = UE5Bones();
		TestEqual(TEXT("UE5 pelvis"), GTCResolveCanonicalBone(ECanonicalBone::Pelvis, B), FString(TEXT("pelvis")));
		TestEqual(TEXT("UE5 right hand"), GTCResolveCanonicalBone(ECanonicalBone::RightHand, B), FString(TEXT("hand_r")));
		TestEqual(TEXT("UE5 left foot"), GTCResolveCanonicalBone(ECanonicalBone::LeftFoot, B), FString(TEXT("foot_l")));
		TestEqual(TEXT("UE5 has no jaw"), GTCResolveCanonicalBone(ECanonicalBone::Jaw, B), FString());
	}

	// --- Canonical resolution: MetaHuman keeps the verbatim FACIAL_ name ------
	{
		const TArray<FString> B = MetaHumanBones();
		TestEqual(TEXT("MetaHuman jaw verbatim"),
			GTCResolveCanonicalBone(ECanonicalBone::Jaw, B), FString(TEXT("FACIAL_C_Jaw")));
		TestEqual(TEXT("MetaHuman right eye verbatim"),
			GTCResolveCanonicalBone(ECanonicalBone::RightEye, B), FString(TEXT("FACIAL_R_Eye")));
	}

	// --- Canonical resolution: Mixamo keeps the namespace prefix --------------
	{
		const TArray<FString> B = MixamoBones();
		TestEqual(TEXT("Mixamo pelvis is Hips"),
			GTCResolveCanonicalBone(ECanonicalBone::Pelvis, B), FString(TEXT("mixamorig:Hips")));
		TestEqual(TEXT("Mixamo right hand verbatim"),
			GTCResolveCanonicalBone(ECanonicalBone::RightHand, B), FString(TEXT("mixamorig:RightHand")));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
