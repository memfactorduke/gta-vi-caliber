// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCWeatherController.h"
#include "SkyModel.h"
#include "../../NPC/Agent/GTCCrowdSubsystem.h"

#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
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
	}

	// One immediate apply so the world looks right before the first tick.
	ApplySky(TimeOfDay);
	ApplyWeather(Weather.Current(), TimeOfDay);
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

void AGTCWeatherController::ApplySky(double Hours)
{
	const FWeatherParams& W = Weather.Current();
	const double Desat = FMath::Clamp(W.SkyDesaturation, 0.0, 1.0);

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
			const double Factor = FSkyModel::SkyLightFactor(Hours);
			SC->SetIntensity((float)(BaseSkyLightIntensity * Factor * (1.0 - 0.35 * Desat)));
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
	}
}

void AGTCWeatherController::RollNextDwell()
{
	const double Lo = FMath::Min(MinDwellSeconds, MaxDwellSeconds);
	const double Hi = FMath::Max(MinDwellSeconds, MaxDwellSeconds);
	CurrentDwellSeconds = FMath::Lerp(Lo, Hi, (double)Rng.FRand());
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
}
