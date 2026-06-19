// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCAppearanceSet.h"

#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"

namespace
{
    // Positive modulo so a (possibly negative) seed-derived index wraps into the
    // pool cleanly. Caller guarantees Size > 0.
    int32 WrapIndex(int32 Index, int32 Size)
    {
        return ((Index % Size) + Size) % Size;
    }
}

USkeletalMesh* UGTCAppearanceSet::ResolveBody(int32 Index) const
{
    if (BodyMeshes.Num() == 0)
    {
        return nullptr;
    }
    return BodyMeshes[WrapIndex(Index, BodyMeshes.Num())].LoadSynchronous();
}

USkeletalMesh* UGTCAppearanceSet::ResolveHead(int32 HeadVariant) const
{
    if (HeadMeshes.Num() == 0)
    {
        return nullptr;
    }
    return HeadMeshes[WrapIndex(HeadVariant, HeadMeshes.Num())].LoadSynchronous();
}

USkeletalMesh* UGTCAppearanceSet::ResolveHeroHead(int32 HeadVariant) const
{
    if (HeroHeadMeshes.Num() == 0)
    {
        return nullptr;
    }
    return HeroHeadMeshes[WrapIndex(HeadVariant, HeroHeadMeshes.Num())].LoadSynchronous();
}

USkeletalMesh* UGTCAppearanceSet::ResolveHair(int32 HairVariant) const
{
    if (HairMeshes.Num() == 0)
    {
        return nullptr;
    }
    return HairMeshes[WrapIndex(HairVariant, HairMeshes.Num())].LoadSynchronous();
}

USkeletalMesh* UGTCAppearanceSet::ResolveOutfit(int32 OutfitVariant) const
{
    if (OutfitMeshes.Num() == 0)
    {
        return nullptr;
    }
    return OutfitMeshes[WrapIndex(OutfitVariant, OutfitMeshes.Num())].LoadSynchronous();
}

UClass* UGTCAppearanceSet::ResolveBodyAnimClass() const
{
    return BodyAnimClass.LoadSynchronous();
}

FLinearColor UGTCAppearanceSet::ResolveSkinTone(int32 Index, const FLinearColor& Fallback) const
{
    if (SkinTones.Num() == 0)
    {
        return Fallback;
    }
    return SkinTones[WrapIndex(Index, SkinTones.Num())];
}
