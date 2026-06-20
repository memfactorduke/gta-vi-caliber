// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCWeatherController.h"
#include "SkyModel.h"
#include "NightLights.h"
#include "../../NPC/Agent/GTCCrowdSubsystem.h"

#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/Light.h"
#include "Components/LightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// Neutral noon white and the deep golden-hour colour the sun warms toward.
	const FLinearColor NoonWhite(1.0f, 0.99f, 0.96f);
	const FLinearColor SunsetOrange(1.0f, 0.42f, 0.16f);

	template <typename T>
	T* FindByClassPreferTag(UWorld* World, FName Tag)
	{
		if (!World)
		{
			return nullptr;
		}
		T* First = nullptr;
		for (TActorIterator<T> It(World); It; ++It)
		{
			T* A = *It;
			if (A->ActorHasTag(Tag))
			{
				return A; // an explicitly tagged actor always wins
			}
			if (!First)
			{
				First = A;
			}
		}
		return First; // otherwise the (typically only) actor of this class
	}
}

AGTCWeatherController::AGTCWeatherController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// The procedural star material the dome uses by default (authored as a project
	// asset; additive unlit, reads scalar params StarOpacity / StarBrightness).
	StarMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Environment/Sky/M_GTCStars.M_GTCStars")));
}

EWeatherKind AGTCWeatherController::ToCore(EGTCWeatherKind Kind)
{
	// The two enums are declared in the same order, so the index maps directly.
	return static_cast<EWeatherKind>(static_cast<uint8>(Kind));
}

void AGTCWeatherController::BeginPlay()
{
	Super::BeginPlay();

	TimeOfDay = FSkyModel::WrapHours(StartTimeOfDay);
	Rng.Initialize(WeatherSeed);
	Weather.InitializeTo(ToCore(StartingWeather));
	RollNextDwell();

	ResolveSkyActors();

	// Register runtime console controls (one set; unregistered in EndPlay).
	TWeakObjectPtr<AGTCWeatherController> WeakThis(this);
	IConsoleManager& CM = IConsoleManager::Get();

	if (!CM.FindConsoleObject(TEXT("gtc.Weather.Set")))
	{
		ConsoleCommands.Add(CM.RegisterConsoleCommand(
			TEXT("gtc.Weather.Set"),
			TEXT("Set weather (turns auto off): 0 Clear, 1 PartlyCloudy, 2 Overcast, 3 Rain, 4 Storm, 5 Fog"),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[WeakThis](const TArray<FString>& Args, UWorld*)
				{
					if (AGTCWeatherController* C = WeakThis.Get())
					{
						if (Args.Num() > 0)
						{
							C->SetWeather((EGTCWeatherKind)FMath::Clamp(FCString::Atoi(*Args[0]), 0, 5));
						}
					}
				}),
			ECVF_Default));

		ConsoleCommands.Add(CM.RegisterConsoleCommand(
			TEXT("gtc.Weather.Auto"),
			TEXT("Enable (1) or disable (0) automatic weather changes"),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[WeakThis](const TArray<FString>& Args, UWorld*)
				{
					if (AGTCWeatherController* C = WeakThis.Get())
					{
						C->bAutoWeather = (Args.Num() == 0) || (FCString::Atoi(*Args[0]) != 0);
					}
				}),
			ECVF_Default));

		ConsoleCommands.Add(CM.RegisterConsoleCommand(
			TEXT("gtc.Weather.Time"),
			TEXT("Set time of day in hours (0..24)"),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[WeakThis](const TArray<FString>& Args, UWorld*)
				{
					if (AGTCWeatherController* C = WeakThis.Get())
					{
						if (Args.Num() > 0)
						{
							C->SetTimeOfDay(FCString::Atof(*Args[0]));
						}
					}
				}),
			ECVF_Default));

		ConsoleCommands.Add(CM.RegisterConsoleCommand(
			TEXT("gtc.Weather.DayLength"),
			TEXT("Set real seconds per full 24h cycle (0 freezes time)"),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[WeakThis](const TArray<FString>& Args, UWorld*)
				{
					if (AGTCWeatherController* C = WeakThis.Get())
					{
						if (Args.Num() > 0)
						{
							C->DayLengthSeconds = FMath::Max(0.0f, (float)FCString::Atof(*Args[0]));
						}
					}
				}),
			ECVF_Default));

		ConsoleCommands.Add(CM.RegisterConsoleCommand(
			TEXT("gtc.Weather.SunLux"),
			TEXT("Set peak midday sun brightness in lux (default 50000)"),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[WeakThis](const TArray<FString>& Args, UWorld*)
				{
					if (AGTCWeatherController* C = WeakThis.Get())
					{
						if (Args.Num() > 0)
						{
							C->DaySunLux = FMath::Max(0.0f, (float)FCString::Atof(*Args[0]));
						}
					}
				}),
			ECVF_Default));

		ConsoleCommands.Add(CM.RegisterConsoleCommand(
			TEXT("gtc.Weather.ExposureBias"),
			TEXT("Set overall brightness offset in stops (negative darkens, default 0)"),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[WeakThis](const TArray<FString>& Args, UWorld*)
				{
					if (AGTCWeatherController* C = WeakThis.Get())
					{
						if (Args.Num() > 0)
						{
							// ApplySky re-applies exposure (incl. this bias) every tick.
							C->ExposureBias = (float)FCString::Atof(*Args[0]);
						}
					}
				}),
			ECVF_Default));

		ConsoleCommands.Add(CM.RegisterConsoleCommand(
			TEXT("gtc.Weather.StarBrightness"),
			TEXT("Set night star brightness multiplier (0 hides stars, default 4)"),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
				[WeakThis](const TArray<FString>& Args, UWorld*)
				{
					if (AGTCWeatherController* C = WeakThis.Get())
					{
						if (Args.Num() > 0)
						{
							// UpdateStars re-pushes this onto the dome material every tick.
							C->StarBrightness = FMath::Max(0.0f, (float)FCString::Atof(*Args[0]));
						}
					}
				}),
			ECVF_Default));
	}

	// One immediate apply so the world looks right before the first tick.
	ApplySky(TimeOfDay);
	ApplyWeather(Weather.Current(), TimeOfDay);
	UpdateStars(TimeOfDay);
}

void AGTCWeatherController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	IConsoleManager& CM = IConsoleManager::Get();
	for (IConsoleCommand* Cmd : ConsoleCommands)
	{
		if (Cmd)
		{
			CM.UnregisterConsoleObject(Cmd);
		}
	}
	ConsoleCommands.Reset();

	Super::EndPlay(EndPlayReason);
}

void AGTCWeatherController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Advance the continuous clock and the weather blend.
	TimeOfDay = FSkyModel::AdvanceHours(TimeOfDay, DeltaSeconds, DayLengthSeconds);
	const bool bRollNew = Weather.Tick(DeltaSeconds, CurrentDwellSeconds);
	if (bAutoWeather && bRollNew)
	{
		const EWeatherKind Next = FWeatherSystem::ChooseNextKind(Weather.CurrentKind(), Rng.FRand());
		Weather.BeginTransition(Next, WeatherTransitionSeconds);
		RollNextDwell();
	}

	if (!bResolved)
	{
		ResolveSkyActors(); // streaming may bring the sky actors in after BeginPlay
	}

	ApplySky(TimeOfDay);
	ApplyWeather(Weather.Current(), TimeOfDay);
	UpdateStars(TimeOfDay);
	if (bAutoCityLights)
	{
		UpdateCityLights(TimeOfDay, DeltaSeconds);
	}

	// Tell the crowd what the sky is doing, so citizens react (shelter/hurry/grumble)
	// when it turns. The crowd no-ops unless the severity actually changed, so this
	// every-tick push only broadcasts on a real transition.
	if (UWorld* World = GetWorld())
	{
		if (UGTCCrowdSubsystem* Crowd = World->GetSubsystem<UGTCCrowdSubsystem>())
		{
			Crowd->SetWeatherSeverity(FWeatherSystem::SeverityIndex(Weather.CurrentKind()));
		}
	}
}

void AGTCWeatherController::ResolveSkyActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!Sun)
	{
		Sun = FindByClassPreferTag<ADirectionalLight>(World, TEXT("GTC_Sun"));
		if (Sun)
		{
			// An adopted level sun is often Static/Stationary, which makes UE reject
			// the per-tick rotation ("...must be Movable if you'd like to move it") and
			// freezes the cycle. A real SetMobility (which re-registers the component)
			// is the only thing that fixes it — setting the property alone does not.
			if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
			{
				if (C->Mobility != EComponentMobility::Movable)
				{
					C->SetMobility(EComponentMobility::Movable);
				}
				C->bAtmosphereSunLight = true; // keep the atmosphere lit by our sun
			}
		}
	}
	if (!SkyLight)
	{
		SkyLight = FindByClassPreferTag<ASkyLight>(World, TEXT("GTC_SkyLight"));
		if (SkyLight)
		{
			if (USkyLightComponent* SC = SkyLight->GetLightComponent())
			{
				// Adopted sky lights must be Movable + real-time so the ambient tracks
				// the moving sun (same re-registration requirement as the sun).
				if (SC->Mobility != EComponentMobility::Movable)
				{
					SC->SetMobility(EComponentMobility::Movable);
				}
				SC->bRealTimeCapture = true;
				BaseSkyLightIntensity = SC->Intensity; // remember the authored ambient level
			}
		}
	}
	if (!HeightFog)
	{
		HeightFog = FindByClassPreferTag<AExponentialHeightFog>(World, TEXT("GTC_HeightFog"));
	}

	// Nothing to drive? Build the whole rig ourselves so this one actor is a
	// complete dynamic sky on any map (the level lost its rig in the engine
	// downgrade — see ResolveSkyActors callers).
	if (bAutoCreateSkyRig && !bRigCreated && (!Sun || !SkyLight || !HeightFog))
	{
		EnsureSkyRig();
	}

	// Make sure an exposure clamp exists so midday doesn't blow out. Adopt only a
	// tagged GTC_GlobalPostProcess (never a designer's own untagged volume), else
	// spawn our own.
	EnsurePostProcess();

	// Soft moonlight fill so night isn't pitch black between the city's lamps.
	EnsureMoon();

	// Star dome for the night sky (always, independent of whether we built the rig).
	EnsureStarDome();

	bResolved = (Sun != nullptr) && (SkyLight != nullptr) && (HeightFog != nullptr);
}

void AGTCWeatherController::EnsureSkyRig()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	bRigCreated = true; // create at most one rig, even if some spawns fail

	// Movable directional sun, flagged as the atmosphere's sun light.
	if (!Sun)
	{
		const FTransform Xform(FRotator(-45.0, SunBaseYaw, 0.0));
		ADirectionalLight* L = World->SpawnActorDeferred<ADirectionalLight>(
			ADirectionalLight::StaticClass(), Xform, this);
		if (L)
		{
			if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(L->GetLightComponent()))
			{
				C->SetMobility(EComponentMobility::Movable);
				C->bAtmosphereSunLight = true;
				C->SetIntensity(DaySunLux);
				C->SetDynamicShadowDistanceMovableLight(20000.0f);
			}
			L->Tags.Add(TEXT("GTC_Sun"));
			L->FinishSpawning(Xform);
			Sun = L;
		}
	}

	// SkyAtmosphere (auto-follows the sun light; nothing we drive per-frame).
	{
		AActor* AtmoActor = World->SpawnActor<AActor>(AActor::StaticClass());
		if (AtmoActor)
		{
			AtmoActor->Tags.Add(TEXT("GTC_SkyAtmosphere"));
			USkyAtmosphereComponent* Atmo = NewObject<USkyAtmosphereComponent>(AtmoActor, TEXT("GTC_SkyAtmosphereComp"));
			AtmoActor->SetRootComponent(Atmo);
			Atmo->RegisterComponent();
		}
	}

	// Realtime-capture SkyLight for ambient that tracks the changing sky.
	if (!SkyLight)
	{
		ASkyLight* S = World->SpawnActorDeferred<ASkyLight>(ASkyLight::StaticClass(), FTransform::Identity, this);
		if (S)
		{
			if (USkyLightComponent* SC = S->GetLightComponent())
			{
				SC->SetMobility(EComponentMobility::Movable);
				SC->bRealTimeCapture = true;
				SC->bLowerHemisphereIsBlack = false;
				BaseSkyLightIntensity = SC->Intensity;
			}
			S->Tags.Add(TEXT("GTC_SkyLight"));
			S->FinishSpawning(FTransform::Identity);
			SkyLight = S;
		}
	}

	// Exponential height fog (we drive its density from the weather).
	if (!HeightFog)
	{
		AExponentialHeightFog* F = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass());
		if (F)
		{
			if (UExponentialHeightFogComponent* FC = F->FindComponentByClass<UExponentialHeightFogComponent>())
			{
				FC->SetMobility(EComponentMobility::Movable);
				FC->SetFogDensity(FogDensityClear);
			}
			F->Tags.Add(TEXT("GTC_HeightFog"));
			HeightFog = F;
		}
	}

	// Volumetric cloud (driven via the material/MPC, not per-frame here).
	{
		AActor* CloudActor = World->SpawnActor<AActor>(AActor::StaticClass());
		if (CloudActor)
		{
			CloudActor->Tags.Add(TEXT("GTC_VolumetricCloud"));
			UVolumetricCloudComponent* Cloud = NewObject<UVolumetricCloudComponent>(CloudActor, TEXT("GTC_VolumetricCloudComp"));
			CloudActor->SetRootComponent(Cloud);
			Cloud->RegisterComponent();
		}
	}
}

void AGTCWeatherController::EnsurePostProcess()
{
	UWorld* World = GetWorld();
	if (!World || PostProcess)
	{
		return;
	}

	// Adopt an explicitly tagged volume if the level ships one — but never grab a
	// designer's own untagged post-process volume (that would override their grade).
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("GTC_GlobalPostProcess")))
		{
			PostProcess = *It;
			break;
		}
	}

	if (!PostProcess && bAutoCreateSkyRig)
	{
		if (APostProcessVolume* PP = World->SpawnActor<APostProcessVolume>())
		{
			PP->bUnbound = true;       // applies everywhere, no trigger box
			PP->BlendWeight = 1.0f;
			PP->Priority = 1.0f;       // win over the level's default unbound volume
			PP->Tags.Add(TEXT("GTC_GlobalPostProcess"));
			PostProcess = PP;
		}
	}

	ApplyExposure(DayExposureEV);
}

void AGTCWeatherController::EnsureMoon()
{
	UWorld* World = GetWorld();
	if (!World || !bMoonlight || Moon)
	{
		return;
	}

	// Adopt a tagged moon if one exists, otherwise spawn a movable cool-blue
	// directional fill. It is NOT an atmosphere sun light (that stays the real sun),
	// just a soft ambient key so surfaces are readable at night; ApplySky fades its
	// intensity with the daylight so it never tints the day.
	Moon = FindByClassPreferTag<ADirectionalLight>(World, TEXT("GTC_Moon"));
	if (Moon == Sun)
	{
		Moon = nullptr; // never repurpose the sun as the moon
	}
	if (!Moon)
	{
		const FTransform Xform(FRotator(-50.0, SunBaseYaw + 180.0, 0.0));
		ADirectionalLight* L = World->SpawnActorDeferred<ADirectionalLight>(
			ADirectionalLight::StaticClass(), Xform, this);
		if (L)
		{
			if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(L->GetLightComponent()))
			{
				C->SetMobility(EComponentMobility::Movable);
				C->bAtmosphereSunLight = false;
				C->SetIntensity(0.0f);
				C->SetLightColor(MoonColor);
				C->SetCastShadows(false); // a soft fill, not a second shadow-caster
			}
			L->Tags.Add(TEXT("GTC_Moon"));
			L->FinishSpawning(Xform);
			Moon = L;
		}
	}
}

void AGTCWeatherController::EnsureStarDome()
{
	UWorld* World = GetWorld();
	if (!World || !bShowStarsAtNight || StarDome)
	{
		return;
	}

	// Adopt an explicitly tagged dome if the level already ships one; otherwise spawn
	// a huge inside-out sphere centred on the camera. (We can't use FindByClassPreferTag
	// here — its "first actor of class" fallback would grab an arbitrary AActor.)
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("GTC_StarDome")))
		{
			StarDome = *It;
			StarDomeMesh = StarDome->FindComponentByClass<UStaticMeshComponent>();
			break;
		}
	}

	if (!StarDome)
	{
		AActor* Dome = World->SpawnActor<AActor>(AActor::StaticClass());
		if (!Dome)
		{
			return;
		}
		Dome->Tags.Add(TEXT("GTC_StarDome"));

		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Dome, TEXT("GTC_StarDomeMesh"));
		Dome->SetRootComponent(Mesh);
		if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
		{
			Mesh->SetStaticMesh(Sphere);
		}
		// A sky dome: no collision/shadow/decals/nav, and it must not occlude itself.
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->bCastDynamicShadow = false;
		Mesh->bReceivesDecals = false;
		Mesh->SetCanEverAffectNavigation(false);
		// The engine sphere is 100uu across (radius 50uu); scale up to the dome radius.
		const float Scale = FMath::Max(StarDomeRadius, 10000.0f) / 50.0f;
		Mesh->SetWorldScale3D(FVector(Scale));
		Mesh->RegisterComponent();

		StarDomeMesh = Mesh;
		StarDome = Dome;
	}

	if (StarDomeMesh && !StarMID)
	{
		if (UMaterialInterface* Mat = StarMaterial.LoadSynchronous())
		{
			StarMID = UMaterialInstanceDynamic::Create(Mat, this);
			StarMID->SetScalarParameterValue(TEXT("StarBrightness"), StarBrightness);
			StarDomeMesh->SetMaterial(0, StarMID);
		}
	}
}

void AGTCWeatherController::UpdateStars(double Hours)
{
	// Lazily create the dome if BeginPlay's attempt failed (e.g. the material wasn't
	// loaded yet); self-guards once everything is in place.
	if (!StarDome || !StarMID)
	{
		EnsureStarDome();
	}
	if (!StarDome)
	{
		return;
	}

	// Keep the dome centred on the view so the stars sit at infinity (no parallax as
	// the player moves) and every piece of city geometry stays inside it and occludes
	// the additive starlight rather than letting it bleed over rooftops.
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			StarDome->SetActorLocation(CamLoc);
		}
	}

	if (StarMID)
	{
		// Stars follow the same continuous clock as everything else (1 - daylight), so
		// they ease in at dusk and out at dawn, and are dimmed under cloud cover.
		const double Clouds = FMath::Clamp((double)Weather.Current().CloudCoverage, 0.0, 1.0);
		const double Opacity = FSkyModel::StarOpacity(Hours) * (1.0 - 0.85 * Clouds);
		StarMID->SetScalarParameterValue(TEXT("StarOpacity"), (float)Opacity);
		StarMID->SetScalarParameterValue(TEXT("StarBrightness"), StarBrightness);
	}
}

void AGTCWeatherController::ApplyExposure(double Ev)
{
	if (!PostProcess)
	{
		return;
	}

	// Project runs r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange, so the
	// min/max are EV100 stops. Pinning min == max fixes the exposure to Ev (no auto
	// metering, which otherwise blows the day to white); ApplySky moves Ev with the
	// time of day so day stays clean and night opens up enough to see the lights.
	FPostProcessSettings& S = PostProcess->Settings;
	S.bOverride_AutoExposureMinBrightness = true;
	S.AutoExposureMinBrightness = (float)Ev;
	S.bOverride_AutoExposureMaxBrightness = true;
	S.AutoExposureMaxBrightness = (float)Ev;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = ExposureBias;
}

void AGTCWeatherController::ApplySky(double Hours)
{
	const FWeatherParams& W = Weather.Current();
	const double Desat = FMath::Clamp(W.SkyDesaturation, 0.0, 1.0);

	// Ramp the fixed exposure with the time of day: bright-day EV at noon eases down
	// to the lower night EV after dusk so streetlights/neon read instead of black.
	// DaylightFactor is C1-smooth, so the exposure glide is too — no popping.
	if (PostProcess)
	{
		const double Ev = FMath::Lerp((double)NightExposureEV, (double)DayExposureEV, FSkyModel::DaylightFactor(Hours));
		ApplyExposure(Ev);
	}

	if (Sun)
	{
		const double Pitch = FSkyModel::SunPitchDegrees(Hours);
		const double Yaw = FSkyModel::SunYawDegrees(Hours, SunBaseYaw);
		Sun->SetActorRotation(FRotator(Pitch, Yaw, 0.0));

		if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			const double Lux = FSkyModel::SunBrightnessLux(Hours, DaySunLux, NightSunLux) * W.SunLightScale;
			C->SetIntensity((float)Lux);

			// Warm toward the horizon, then grey out a little under heavy weather.
			const double Warmth = FSkyModel::SunWarmth(Hours);
			FLinearColor Colour = FMath::Lerp(NoonWhite, SunsetOrange, (float)Warmth);
			const float Grey = Colour.GetLuminance();
			Colour = FMath::Lerp(Colour, FLinearColor(Grey, Grey, Grey), (float)(0.5 * Desat));
			C->SetLightColor(Colour);
		}
	}

	if (SkyLight)
	{
		if (USkyLightComponent* SC = SkyLight->GetLightComponent())
		{
			// Higher night floor keeps a faint moonlit ambient instead of pure black.
			const double Factor = FSkyModel::SkyLightFactor(Hours, NightSkyLightFloor);
			SC->SetIntensity((float)(BaseSkyLightIntensity * Factor * (1.0 - 0.35 * Desat)));
		}
	}

	// Moonlight fill: brightest at full night, faded to nothing by day so it never
	// tints daylight. Points opposite the sun and casts no shadow (soft key only).
	if (Moon)
	{
		if (UDirectionalLightComponent* MC = Cast<UDirectionalLightComponent>(Moon->GetLightComponent()))
		{
			const double NightAmount = 1.0 - FSkyModel::DaylightFactor(Hours);
			MC->SetIntensity((float)(MoonIntensityLux * NightAmount));
			MC->SetLightColor(MoonColor);
		}
	}
}

void AGTCWeatherController::ApplyWeather(const FWeatherParams& W, double Hours)
{
	if (HeightFog)
	{
		if (UExponentialHeightFogComponent* FC = HeightFog->FindComponentByClass<UExponentialHeightFogComponent>())
		{
			const double NightLift = (1.0 - FSkyModel::DaylightFactor(Hours)) * 0.004; // a touch more haze at night
			const double Density = FMath::Lerp((double)FogDensityClear, (double)FogDensityThick, W.FogDensity) + NightLift;
			FC->SetFogDensity((float)Density);
		}
	}

	// Push the full weather vector into the material collection so cloud, rain and
	// wet-surface materials can react. Harmless no-op if no MPC is assigned.
	if (UMaterialParameterCollection* MPC = WeatherParameters.LoadSynchronous())
	{
		auto Set = [&](const TCHAR* Name, double Value)
		{
			UKismetMaterialLibrary::SetScalarParameterValue(this, MPC, FName(Name), (float)Value);
		};
		Set(TEXT("CloudCoverage"), W.CloudCoverage);
		Set(TEXT("CloudDensity"), W.CloudDensity);
		Set(TEXT("RainIntensity"), W.RainIntensity);
		Set(TEXT("Wetness"), W.Wetness);
		Set(TEXT("Wind"), W.Wind);
		Set(TEXT("SkyDesaturation"), W.SkyDesaturation);
		Set(TEXT("Daylight"), FSkyModel::DaylightFactor(Hours));
		Set(TEXT("SunWarmth"), FSkyModel::SunWarmth(Hours));
		Set(TEXT("StarOpacity"), FSkyModel::StarOpacity(Hours));
		// Share of the city that should be lit at this darkness (FNightLights), so
		// emissive windows / neon materials reading this param switch on at dusk and
		// off at dawn with hysteresis — the whole city lights up, not just lamps.
		Set(TEXT("world_night_amount"), FNightLights::LitFraction(FSkyModel::DaylightFactor(Hours)));
	}
}

void AGTCWeatherController::RollNextDwell()
{
	const double Lo = FMath::Min(MinDwellSeconds, MaxDwellSeconds);
	const double Hi = FMath::Max(MinDwellSeconds, MaxDwellSeconds);
	CurrentDwellSeconds = FMath::Lerp(Lo, Hi, (double)Rng.FRand());
}

void AGTCWeatherController::RescanCityLights()
{
	CityLights.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Every light actor in the world (point/spot/rect/etc.), minus the sky rig's
	// own directional sun/moon. Re-scanning each cycle means newly placed or
	// streamed-in lights are managed automatically — no count limit, no wiring.
	for (TActorIterator<ALight> It(World); It; ++It)
	{
		ALight* L = *It;
		if (!L || L == Sun || L == Moon)
		{
			continue;
		}
		if (L->ActorHasTag(TEXT("GTC_Sun")) || L->ActorHasTag(TEXT("GTC_Moon")))
		{
			continue;
		}
		ULightComponent* C = L->GetLightComponent();
		if (!C || C->IsA(UDirectionalLightComponent::StaticClass()))
		{
			continue; // city lights are local lights, never the directional sun/moon
		}
		CityLights.Add(C);
	}
}

void AGTCWeatherController::UpdateCityLights(double Hours, float DeltaSeconds)
{
	// Periodically re-scan so lights added/streamed in after BeginPlay are picked up,
	// and force the current on/off state onto the freshly gathered set.
	TimeSinceLightRescan += DeltaSeconds;
	bool bRescanned = false;
	if (!bCityLightsInit || TimeSinceLightRescan >= CityLightsRescanSeconds)
	{
		RescanCityLights();
		TimeSinceLightRescan = 0.0f;
		bRescanned = true;
	}

	// Darkness % -> daylight thresholds (On below a lower daylight, Off above a higher
	// one => a hysteresis hold band so lights don't chatter through dusk).
	const double Daylight = FSkyModel::DaylightFactor(Hours);
	const double OnDaylight = 1.0 - FMath::Clamp(CityLightsOnDarknessPct, 0.0f, 100.0f) / 100.0;
	const double OffDaylight = 1.0 - FMath::Clamp(CityLightsOffDarknessPct, 0.0f, 100.0f) / 100.0;
	const bool bWantLit = FNightLights::ShouldBeLit(bCityLightsLit, Daylight, OnDaylight, OffDaylight);

	if (bWantLit == bCityLightsLit && !bRescanned && bCityLightsInit)
	{
		return; // nothing changed and no new lights to bring into line
	}

	for (const TWeakObjectPtr<ULightComponent>& Weak : CityLights)
	{
		ULightComponent* C = Weak.Get();
		if (!C)
		{
			continue;
		}
		C->SetVisibility(bWantLit);
		// Lift placeholder/too-dim fixtures so they actually read once switched on.
		if (bWantLit && CityLightMinIntensity > 0.0f && C->Intensity < CityLightMinIntensity)
		{
			C->SetIntensity(CityLightMinIntensity);
		}
	}

	bCityLightsLit = bWantLit;
	bCityLightsInit = true;
}

void AGTCWeatherController::SetWeather(EGTCWeatherKind Kind)
{
	bAutoWeather = false;
	Weather.BeginTransition(ToCore(Kind), WeatherTransitionSeconds);
}

void AGTCWeatherController::SetTimeOfDay(float Hours)
{
	TimeOfDay = FSkyModel::WrapHours(Hours);
	ApplySky(TimeOfDay);
	UpdateStars(TimeOfDay);
}
