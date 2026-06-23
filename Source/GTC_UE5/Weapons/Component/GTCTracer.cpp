// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTracer.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

AGTCTracer::AGTCTracer()
{
	PrimaryActorTick.bCanEverTick = false;

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	RootComponent = Beam;
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	Beam->SetReceivesDecals(false);
	// The engine cylinder is 100 cm tall along +Z, radius 50 cm — a unit the tracer rescales.
	if (UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		Beam->SetStaticMesh(Cylinder);
	}
	if (UMaterialInterface* TracerMat =
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Surfaces/FX/M_GTC_Tracer.M_GTC_Tracer")))
	{
		Beam->SetMaterial(0, TracerMat);
	}
}

void AGTCTracer::Init(const FVector& Start, const FVector& End, double ThicknessCm, double LifeSeconds)
{
	const FVector Delta = End - Start;
	const double Length = Delta.Size();
	if (Length <= 1.0)
	{
		Destroy();
		return;
	}

	SetActorLocation((Start + End) * 0.5);
	// Point the cylinder's +Z axis down the shot, then scale: X/Y set the diameter (engine cylinder
	// is 100 cm across, so thickness/100), Z stretches the 100 cm length to the shot distance.
	SetActorRotation(FRotationMatrix::MakeFromZ(Delta / Length).Rotator());
	const double Diameter = ThicknessCm / 100.0;
	Beam->SetWorldScale3D(FVector(Diameter, Diameter, Length / 100.0));

	SetLifeSpan(static_cast<float>(LifeSeconds));
}

void AGTCTracer::Spawn(UWorld* World, const FVector& Start, const FVector& End,
	double ThicknessCm, double LifeSeconds)
{
	if (World == nullptr || (End - Start).IsNearlyZero())
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AGTCTracer* Tracer = World->SpawnActor<AGTCTracer>(
			AGTCTracer::StaticClass(), FTransform::Identity, Params))
	{
		Tracer->Init(Start, End, ThicknessCm, LifeSeconds);
	}
}
