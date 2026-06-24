// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCSurfaceImpactLibrary.h"

#include "SurfaceImpact.h"
#include "SurfaceImpactFX.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

namespace
{
	UWorld* WorldFrom(const UObject* WorldContextObject)
	{
		if (WorldContextObject == nullptr || GEngine == nullptr)
		{
			return nullptr;
		}
		return GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	}
}

void UGTCSurfaceImpactLibrary::PlaySurfaceImpact(const UObject* WorldContextObject, const FHitResult& Hit)
{
	FGTCImpactFX::PlayImpact(WorldFrom(WorldContextObject), Hit);
}

void UGTCSurfaceImpactLibrary::PlaySurfaceImpactAt(const UObject* WorldContextObject, FVector ImpactPoint,
	FVector ImpactNormal, AActor* HitActor, UPrimitiveComponent* HitComponent)
{
	// Rebuild a minimal hit the resolver understands: tags come off the actor/component, and the
	// burst orients along the normal. A zero normal just yields a forward-facing burst.
	FHitResult Hit;
	Hit.ImpactPoint = ImpactPoint;
	Hit.Location = ImpactPoint;
	Hit.ImpactNormal = ImpactNormal.IsNearlyZero() ? FVector::UpVector : ImpactNormal.GetSafeNormal();
	Hit.Normal = Hit.ImpactNormal;
	if (HitActor != nullptr)
	{
		Hit.HitObjectHandle = FActorInstanceHandle(HitActor);
	}
	Hit.Component = HitComponent;
	FGTCImpactFX::PlayImpact(WorldFrom(WorldContextObject), Hit);
}

FString UGTCSurfaceImpactLibrary::ResolveImpactSurfaceName(const FHitResult& Hit)
{
	const EGTCSurface Surface = FGTCImpactFX::ResolveSurface(Hit);
	return FString(FGTCSurfaceImpact::SurfaceName(Surface));
}
