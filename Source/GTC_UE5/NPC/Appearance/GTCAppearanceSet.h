// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GTCAppearanceSet.generated.h"

class USkeletalMesh;
class UAnimInstance;

/**
 * A data-authored wardrobe for the crowd: the pools of body meshes, head/face
 * meshes, skin tones, and the locomotion Anim Blueprint that AGTCCitizen draws
 * from. This is the artist-facing replacement for the hardcoded mannequin + head
 * paths baked into GTCCitizen.cpp — point a citizen (or the crowd director's
 * CitizenClass default) at one of these assets and new faces/bodies are added by
 * editing the asset, never by recompiling.
 *
 * Selection is deterministic: a citizen's seed-derived indices (its HeadVariant
 * and a body/skin roll) pick entries here, wrapping modulo the pool size, so the
 * same person always looks the same. Everything is soft-referenced and every
 * accessor tolerates an empty pool (returns null / a fallback), so a half-authored
 * set never crashes and the headless build stays green.
 */
UCLASS(BlueprintType)
class GTC_UE5_API UGTCAppearanceSet : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Body skeletal meshes the crowd draws from (was Manny/Quinn, hardcoded). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TArray<TSoftObjectPtr<USkeletalMesh>> BodyMeshes;

    /** Head/face skeletal meshes, indexed by a citizen's HeadVariant (wraps). Each
     *  must be skinned to the body skeleton so it follows the leader pose. This is
     *  the cheap crowd tier — used for distant / ambient citizens. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TArray<TSoftObjectPtr<USkeletalMesh>> HeadMeshes;

    /** High-detail (e.g. MetaHuman) head/face meshes, same HeadVariant indexing.
     *  Swapped in when a citizen is promoted (player close / becomes a contact) and
     *  swapped back out when it returns to the crowd. Leave empty to keep one tier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TArray<TSoftObjectPtr<USkeletalMesh>> HeroHeadMeshes;

    /** Hair skeletal meshes, indexed by a citizen's HairVariant (wraps). Skinned to
     *  the head/body skeleton so it rides the leader pose. A second variation axis. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TArray<TSoftObjectPtr<USkeletalMesh>> HairMeshes;

    /** Outfit/clothing skeletal meshes, indexed by OutfitVariant (wraps). Skinned to
     *  the body skeleton (leader-pose follower); tinted by the archetype. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TArray<TSoftObjectPtr<USkeletalMesh>> OutfitMeshes;

    /** Locomotion Anim Blueprint applied to the body mesh (blends idle/walk/run). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TSoftClassPtr<UAnimInstance> BodyAnimClass;

    /** Skin tones, chosen per-citizen by seed and pushed to the head/body material's
     *  "SkinTone" parameter so a crowd isn't one complexion. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    TArray<FLinearColor> SkinTones;

    /** Load + return the body mesh at Index (wrapped), or null if the pool is empty. */
    USkeletalMesh* ResolveBody(int32 Index) const;

    /** Load + return the crowd head mesh for HeadVariant (wrapped), or null if empty. */
    USkeletalMesh* ResolveHead(int32 HeadVariant) const;

    /** Load + return the high-detail head for HeadVariant (wrapped), or null if the
     *  hero pool is empty (caller then keeps the crowd head). */
    USkeletalMesh* ResolveHeroHead(int32 HeadVariant) const;

    /** Load + return the hair mesh for HairVariant (wrapped), or null if empty. */
    USkeletalMesh* ResolveHair(int32 HairVariant) const;

    /** Load + return the outfit mesh for OutfitVariant (wrapped), or null if empty. */
    USkeletalMesh* ResolveOutfit(int32 OutfitVariant) const;

    /** Load + return the body Anim Blueprint class, or null if unset. */
    UClass* ResolveBodyAnimClass() const;

    /** Skin tone at Index (wrapped), or Fallback if the pool is empty. */
    FLinearColor ResolveSkinTone(int32 Index, const FLinearColor& Fallback) const;
};
