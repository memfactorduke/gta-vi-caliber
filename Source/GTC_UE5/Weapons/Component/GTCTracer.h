// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCTracer.generated.h"

class UStaticMeshComponent;

/**
 * A single tracer streak — a thin, faint, unlit-additive line stretched from the muzzle to the
 * impact point, that self-destroys after a few hundredths of a second. Deliberately subtle: it
 * reads as a hint of a round in flight, not a laser beam. Spawned by UGTCWeaponComponent on a
 * fraction of shots (see TracerEveryNthShot). The look comes from /Game/Surfaces/FX/M_GTC_Tracer.
 */
UCLASS()
class GTC_UE5_API AGTCTracer : public AActor
{
	GENERATED_BODY()

public:
	AGTCTracer();

	/** Orient + scale the beam between two world points and start its (short) life countdown. */
	void Init(const FVector& Start, const FVector& End, double ThicknessCm, double LifeSeconds);

	/** Spawn-and-init helper: a no-op if World is null or the two points coincide. */
	static void Spawn(UWorld* World, const FVector& Start, const FVector& End,
		double ThicknessCm, double LifeSeconds);

private:
	UPROPERTY()
	UStaticMeshComponent* Beam = nullptr;
};
