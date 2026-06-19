// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigProfile.h"

const TCHAR* GTCRigConventionName(ERigConvention Convention)
{
	switch (Convention)
	{
	case ERigConvention::UE5Mannequin: return TEXT("UE5 Mannequin");
	case ERigConvention::Mixamo:       return TEXT("Mixamo");
	case ERigConvention::MetaHuman:    return TEXT("MetaHuman");
	case ERigConvention::Generic:      return TEXT("Generic humanoid");
	default:                           return TEXT("Unknown");
	}
}

const TCHAR* GTCCanonicalBoneName(ECanonicalBone Bone)
{
	switch (Bone)
	{
	case ECanonicalBone::Root:      return TEXT("Root");
	case ECanonicalBone::Pelvis:    return TEXT("Pelvis");
	case ECanonicalBone::Spine:     return TEXT("Spine");
	case ECanonicalBone::Neck:      return TEXT("Neck");
	case ECanonicalBone::Head:      return TEXT("Head");
	case ECanonicalBone::LeftHand:  return TEXT("LeftHand");
	case ECanonicalBone::RightHand: return TEXT("RightHand");
	case ECanonicalBone::LeftFoot:  return TEXT("LeftFoot");
	case ECanonicalBone::RightFoot: return TEXT("RightFoot");
	case ECanonicalBone::Jaw:       return TEXT("Jaw");
	case ECanonicalBone::LeftEye:   return TEXT("LeftEye");
	case ECanonicalBone::RightEye:  return TEXT("RightEye");
	default:                        return TEXT("?");
	}
}

namespace
{
	FString Normalize(const FString& Raw)
	{
		FString S = Raw;
		int32 Colon = INDEX_NONE;
		if (S.FindLastChar(TEXT(':'), Colon))
		{
			S = S.RightChop(Colon + 1);
		}
		return S.ToLower();
	}

	bool C(const FString& Hay, const TCHAR* Needle) { return Hay.Contains(Needle); }

	bool IsLeft(const FString& B)
	{
		return C(B, TEXT("left")) || B.EndsWith(TEXT("_l")) || B.EndsWith(TEXT(".l"))
			|| B.Contains(TEXT("_l_")) || B.Contains(TEXT(".l."));
	}
	bool IsRight(const FString& B)
	{
		return C(B, TEXT("right")) || B.EndsWith(TEXT("_r")) || B.EndsWith(TEXT(".r"))
			|| B.Contains(TEXT("_r_")) || B.Contains(TEXT(".r."));
	}

	bool IsHand(const FString& B)
	{
		return C(B, TEXT("hand")) && !C(B, TEXT("index")) && !C(B, TEXT("middle"))
			&& !C(B, TEXT("ring")) && !C(B, TEXT("pinky")) && !C(B, TEXT("thumb"));
	}
	bool IsFoot(const FString& B) { return C(B, TEXT("foot")) || C(B, TEXT("ankle")); }
	bool IsEye(const FString& B)
	{
		return C(B, TEXT("eye")) && !C(B, TEXT("eyebrow")) && !C(B, TEXT("eyelid")) && !C(B, TEXT("eyelash"));
	}

	// Does a normalized bone match a canonical slot (side handled by the caller)?
	bool MatchesCanonical(ECanonicalBone Bone, const FString& N)
	{
		switch (Bone)
		{
		case ECanonicalBone::Root:      return N == TEXT("root");
		case ECanonicalBone::Pelvis:    return C(N, TEXT("pelvis")) || C(N, TEXT("hips")) || N == TEXT("hip");
		case ECanonicalBone::Spine:     return C(N, TEXT("spine")) || C(N, TEXT("chest")) || C(N, TEXT("torso"));
		case ECanonicalBone::Neck:      return C(N, TEXT("neck"));
		case ECanonicalBone::Head:      return C(N, TEXT("head"));
		case ECanonicalBone::LeftHand:  return IsHand(N) && IsLeft(N);
		case ECanonicalBone::RightHand: return IsHand(N) && IsRight(N);
		case ECanonicalBone::LeftFoot:  return IsFoot(N) && IsLeft(N);
		case ECanonicalBone::RightFoot: return IsFoot(N) && IsRight(N);
		case ECanonicalBone::Jaw:       return C(N, TEXT("jaw"));
		case ECanonicalBone::LeftEye:   return IsEye(N) && IsLeft(N);
		case ECanonicalBone::RightEye:  return IsEye(N) && IsRight(N);
		default:                        return false;
		}
	}
}

ERigConvention GTCDetectRigConvention(const TArray<FString>& BoneNames)
{
	if (BoneNames.Num() == 0)
	{
		return ERigConvention::Unknown;
	}

	bool bMixamo = false;
	bool bFacial = false;
	bool bUEMarkers = false;
	bool bHasPelvis = false;
	bool bHasHead = false;
	bool bHasHand = false;

	for (const FString& Raw : BoneNames)
	{
		// MetaHuman facial bones keep their uppercase FACIAL_ prefix verbatim.
		if (Raw.StartsWith(TEXT("FACIAL_")) || Raw.Contains(TEXT("FACIAL_")))
		{
			bFacial = true;
		}

		const FString N = Normalize(Raw);
		if (Raw.StartsWith(TEXT("mixamorig")) || N.StartsWith(TEXT("mixamorig")))
		{
			bMixamo = true;
		}
		// UE5 Mannequin tells: clavicle/thigh/calf + numbered spine, distinct from
		// Mixamo's LeftUpLeg/Spine1.
		if (C(N, TEXT("clavicle")) || C(N, TEXT("thigh")) || C(N, TEXT("calf")) || C(N, TEXT("spine_0")))
		{
			bUEMarkers = true;
		}
		if (MatchesCanonical(ECanonicalBone::Pelvis, N)) { bHasPelvis = true; }
		if (MatchesCanonical(ECanonicalBone::Head, N))   { bHasHead = true; }
		if (IsHand(N))                                   { bHasHand = true; }
	}

	if (bMixamo)
	{
		return ERigConvention::Mixamo;
	}
	if (bFacial)
	{
		// FACIAL_* head bones are the MetaHuman signature (on a UE-style body).
		return ERigConvention::MetaHuman;
	}
	if (bUEMarkers)
	{
		return ERigConvention::UE5Mannequin;
	}
	if (bHasPelvis && bHasHead && bHasHand)
	{
		return ERigConvention::Generic;
	}
	return ERigConvention::Unknown;
}

FString GTCResolveCanonicalBone(ECanonicalBone Bone, const TArray<FString>& BoneNames)
{
	for (const FString& Raw : BoneNames)
	{
		if (MatchesCanonical(Bone, Normalize(Raw)))
		{
			return Raw; // verbatim, so an engine bone lookup matches
		}
	}
	return FString();
}
