// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "WeatherSystem.h"
#include "GTCWeatherController.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class UMaterialParameterCollection;

/** Editor-facing mirror of the pure-core EWeatherKind (kept UHT-free in the core). */
UENUM(BlueprintType)
enum class EGTCWeatherKind : uint8
{
	Clear,
	PartlyCloudy,
	Overcast,
	Rain,
	Storm,
	Fog
};

/**
 * Runtime adapter that turns FSkyModel + FWeatherSystem into a living sky. It is
 * the UCLASS half of the scene-free-core / actor-adapter pair (the cores carry
 * the testable, provably-smooth math; this actor only resolves the level's sky
 * actors and pushes values onto them each tick). Drop-in replacement for the old
 * BP_GTC_DayNightCycle: place ONE in the persistent level (and delete the BP so
 * two writers don't fight over GTC_Sun).
 *
 * What it drives, resolved by class (preferring a matching actor tag) so it works
 * with the existing GTC_ sky rig with no wiring:
 *  - the directional sun (GTC_Sun): rotation from the continuous clock, brightness
 *    dimmed across dusk and under storms, colour warmed at the golden hour;
 *  - the SkyLight: ambient intensity faded down at night;
 *  - the ExponentialHeightFog: density driven by weather + a small night lift;
 *  - an optional Material Parameter Collection (assign MPC_GTCWeather): scalar
 *    params CloudCoverage / CloudDensity / RainIntensity / Wetness / Wind /
 *    Daylight / SkyDesaturation / SunWarmth / StarOpacity for cloud, rain and
 *    wet-surface materials to read. Absent MPC is harmless — sun/sky/fog still run.
 *
 * Everything moves only through the cores, so by construction the world's light
 * and weather can only ease — never cut — from one state to the next.
 */
UCLASS()
class GTC_UE5_API AGTCWeatherController : public AActor
{
	GENERATED_BODY()

public:
	AGTCWeatherController();

	//~ Time-of-day knobs ------------------------------------------------------
	/** Hour the cycle starts at (0..24). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float StartTimeOfDay = 8.0f;

	/** Real seconds for one full 24h cycle (default 20 min). 0 freezes the clock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0.0"))
	float DayLengthSeconds = 1200.0f;

	/** South-facing reference yaw the sun sweeps around through the day. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky")
	float SunBaseYaw = -120.0f;

	/** Peak midday sun brightness in lux. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0.0"))
	float DaySunLux = 75000.0f;

	/** Night sun floor in lux (kept tiny, not pure black). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0.0"))
	float NightSunLux = 0.0f;

	//~ Weather knobs ----------------------------------------------------------
	/** When true the director rolls new weather on its own; when false weather is held / set by console. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather")
	bool bAutoWeather = true;

	/** Weather to start in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather")
	EGTCWeatherKind StartingWeather = EGTCWeatherKind::PartlyCloudy;

	/** How long a weather change eases over (seconds). Clamped to >= 2s by the core. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather", meta = (ClampMin = "2.0"))
	float WeatherTransitionSeconds = 90.0f;

	/** Shortest a settled spell lasts before the next roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather", meta = (ClampMin = "0.0"))
	float MinDwellSeconds = 240.0f;

	/** Longest a settled spell lasts before the next roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather", meta = (ClampMin = "0.0"))
	float MaxDwellSeconds = 600.0f;

	/** Seed for the deterministic weather director. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather")
	int32 WeatherSeed = 12345;

	/**
	 * If the level has no sun/sky/fog rig, spawn a full one at BeginPlay (movable
	 * sun, SkyAtmosphere, realtime SkyLight, height fog, volumetric cloud) so this
	 * single actor produces a complete dynamic sky on any map. Existing GTC_ sky
	 * actors are always adopted instead of duplicated.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky")
	bool bAutoCreateSkyRig = true;

	/** Fog density at the dry end (Clear). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather", meta = (ClampMin = "0.0"))
	float FogDensityClear = 0.01f;

	/** Fog density at the thick end (Storm/Fog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather", meta = (ClampMin = "0.0"))
	float FogDensityThick = 0.18f;

	/** Optional collection materials read for cloud/rain/wetness (e.g. MPC_GTCWeather). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Weather")
	TSoftObjectPtr<UMaterialParameterCollection> WeatherParameters;

	//~ Runtime control (also exposed via gtc.Weather.* console commands) -------
	/** Manually transition to a weather kind (turns auto off). */
	UFUNCTION(BlueprintCallable, Category = "GTC|Weather")
	void SetWeather(EGTCWeatherKind Kind);

	/** Jump the clock to an absolute hour (0..24). */
	UFUNCTION(BlueprintCallable, Category = "GTC|Sky")
	void SetTimeOfDay(float Hours);

	/** Current clock hour (0..24). */
	UFUNCTION(BlueprintPure, Category = "GTC|Sky")
	float GetTimeOfDay() const { return (float)TimeOfDay; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ResolveSkyActors();
	void EnsureSkyRig();
	void ApplySky(double Hours);
	void ApplyWeather(const FWeatherParams& W, double Hours);
	void RollNextDwell();

	static EWeatherKind ToCore(EGTCWeatherKind Kind);

	/** The pure cores — all the smooth math lives here. */
	double TimeOfDay = 8.0;
	FWeatherSystem Weather;
	FRandomStream Rng;
	double CurrentDwellSeconds = 300.0;

	/** Resolved sky actors (weak — the level owns them). */
	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY(Transient)
	TObjectPtr<ASkyLight> SkyLight;
	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> HeightFog;

	float BaseSkyLightIntensity = 1.0f;
	bool bResolved = false;
	bool bRigCreated = false;

	/** Console-command handles registered for this instance. */
	TArray<IConsoleCommand*> ConsoleCommands;
};
