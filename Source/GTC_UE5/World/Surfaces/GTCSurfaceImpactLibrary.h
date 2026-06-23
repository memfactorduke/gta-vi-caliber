// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GTCSurfaceImpactLibrary.generated.h"

struct FHitResult;
class UPrimitiveComponent;

/**
 * Blueprint entry point into the surface-impact FX system (World/Surfaces/SurfaceImpact*.{h,cpp}).
 *
 * The C++ weapon component (UGTCWeaponComponent) already spawns these bursts itself, but the
 * Blueprint weapons (BP_BaseWeapons and its children) run their own fire trace in the graph, so
 * they need a node to call. Drop "Play Surface Impact" right after the weapon's line trace, feed it
 * the Hit Result, and the round leaves the right burst (+decal) for whatever it struck — splinters
 * off wood, sparks off metal, shards off glass, blood off a creature, and so on.
 */
UCLASS()
class GTC_UE5_API UGTCSurfaceImpactLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Spawn the surface-appropriate impact burst (and a wear decal on hard surfaces) at a hit.
	 * Resolves the surface from the hit's component/actor tags first, then its physical material.
	 * Safe to call for every pellet; a no-op if the hit didn't block.
	 */
	UFUNCTION(BlueprintCallable, Category = "GTC|Weapon|Impact",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Play Surface Impact"))
	static void PlaySurfaceImpact(const UObject* WorldContextObject, const FHitResult& Hit);

	/**
	 * Same, for fire logic that has the pieces but not a full Hit Result. Pass the impact point,
	 * the surface normal (the burst fans along it), and the actor/component that was struck (so
	 * Creature / Surface.* tags resolve). HitComponent may be left empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "GTC|Weapon|Impact",
		meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "HitComponent",
			DisplayName = "Play Surface Impact At"))
	static void PlaySurfaceImpactAt(const UObject* WorldContextObject, FVector ImpactPoint,
		FVector ImpactNormal, AActor* HitActor, UPrimitiveComponent* HitComponent = nullptr);

	/** Debug helper: the resolved surface name for a hit, e.g. "Concrete", "Creature", "Default". */
	UFUNCTION(BlueprintPure, Category = "GTC|Weapon|Impact", meta = (DisplayName = "Resolve Impact Surface"))
	static FString ResolveImpactSurfaceName(const FHitResult& Hit);
};
