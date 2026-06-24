// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurfaceImpactFX.h"

#include "Components/DecalComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/** How long a wear decal (bullet hole / scorch) lingers before fading, in seconds. */
	constexpr float GDecalLifeSeconds = 12.0f;

	// Module-lifetime asset caches. TStrongObjectPtr keeps the loaded asset from being GC'd
	// between shots; keyed by the registry's /Game path string. A null value is cached too, so a
	// missing/typo'd path is loaded (and warned) once, not on every impact.
	TMap<FString, TStrongObjectPtr<UParticleSystem>> GParticleCache;
	TMap<FString, TStrongObjectPtr<UMaterialInterface>> GDecalCache;

	UParticleSystem* LoadParticleCached(const TCHAR* Path)
	{
		if (Path == nullptr)
		{
			return nullptr;
		}
		const FString Key(Path);
		if (const TStrongObjectPtr<UParticleSystem>* Found = GParticleCache.Find(Key))
		{
			return Found->Get();
		}
		UParticleSystem* System = LoadObject<UParticleSystem>(nullptr, Path);
		GParticleCache.Add(Key, TStrongObjectPtr<UParticleSystem>(System));
		return System;
	}

	UMaterialInterface* LoadDecalCached(const TCHAR* Path)
	{
		if (Path == nullptr)
		{
			return nullptr;
		}
		const FString Key(Path);
		if (const TStrongObjectPtr<UMaterialInterface>* Found = GDecalCache.Find(Key))
		{
			return Found->Get();
		}
		UMaterialInterface* Decal = LoadObject<UMaterialInterface>(nullptr, Path);
		GDecalCache.Add(Key, TStrongObjectPtr<UMaterialInterface>(Decal));
		return Decal;
	}
} // namespace

EGTCSurface FGTCImpactFX::ResolveSurface(const FHitResult& Hit)
{
	// 1a. The hit primitive's own tags win first — a destructible sub-part (a glass pane on a
	//     concrete building) can override the actor it belongs to.
	if (const UPrimitiveComponent* Component = Hit.GetComponent())
	{
		for (const FName& Tag : Component->ComponentTags)
		{
			const EGTCSurface Surface = FGTCSurfaceImpact::SurfaceFromTag(Tag);
			if (Surface != EGTCSurface::Default)
			{
				return Surface;
			}
		}
	}

	// 1b. Then the actor's tags — every NPC pawn carries "Creature"; tagged props/buildings carry
	//     "Surface.*".
	if (const AActor* Actor = Hit.GetActor())
	{
		for (const FName& Tag : Actor->Tags)
		{
			const EGTCSurface Surface = FGTCSurfaceImpact::SurfaceFromTag(Tag);
			if (Surface != EGTCSurface::Default)
			{
				return Surface;
			}
		}
	}

	// 2. Untagged geometry: read the physical material the trace returned (needs
	//    bReturnPhysicalMaterial). The SurfaceTypeN index maps to our category.
	if (Hit.PhysMaterial.IsValid())
	{
		const uint8 SurfaceIndex = static_cast<uint8>(Hit.PhysMaterial->SurfaceType.GetValue());
		return FGTCSurfaceImpact::PhysicalSurfaceToSurface(SurfaceIndex);
	}

	// 3. Nothing tagged, no physical material -> generic puff.
	return EGTCSurface::Default;
}

void FGTCImpactFX::PlayImpact(UWorld* World, const FHitResult& Hit)
{
	if (World == nullptr)
	{
		return;
	}

	const EGTCSurface Surface = ResolveSurface(Hit);
	const FGTCImpactEffect Effect = FGTCSurfaceImpact::ImpactEffectFor(Surface);

	// Orient the burst to the surface normal so sparks/dust/splinters fan off the surface rather
	// than firing into it. A degenerate normal (rare) just yields a forward-facing burst.
	const FVector Location = Hit.ImpactPoint;
	const FRotator Rotation = Hit.ImpactNormal.Rotation();

	if (UParticleSystem* System = LoadParticleCached(Effect.ParticlePath))
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World, System, Location, Rotation, FVector(Effect.Scale), /*bAutoDestroy=*/true);
	}

	// Wear decal only on static geometry: the registry sets DecalSizeCm == 0 (and DecalPath null)
	// for a moving body, so blood never stamps a decal on a walking NPC.
	if (Effect.DecalPath != nullptr && Effect.DecalSizeCm > 0.0)
	{
		if (UMaterialInterface* Decal = LoadDecalCached(Effect.DecalPath))
		{
			UDecalComponent* Stamped = UGameplayStatics::SpawnDecalAtLocation(
				World, Decal, FVector(Effect.DecalSizeCm), Location, Rotation, GDecalLifeSeconds);
			if (Stamped != nullptr)
			{
				Stamped->SetFadeScreenSize(0.0f); // keep distant bullet holes visible
			}
		}
	}
}
