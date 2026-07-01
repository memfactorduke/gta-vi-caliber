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
class APostProcessVolume;
class ULightComponent;
class UMaterialParameterCollection;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

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

	/**
	 * Real seconds for one full 24h cycle. Default 2880s = 48 real minutes, the
	 * same day length GTA V/VI use, so the day no longer races by. 0 freezes the
	 * clock. Tunable live with `gtc.Weather.DayLength <s>`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0.0"))
	float DayLengthSeconds = 2880.0f;

	/** South-facing reference yaw the sun sweeps around through the day. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky")
	float SunBaseYaw = -120.0f;

	/** Peak midday sun brightness in lux. Tunable live with `gtc.Weather.SunLux <lux>`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0.0"))
	float DaySunLux = 50000.0f;

	/**
	 * Forward-shading priority pinned on the sun so it always owns the single
	 * directional slot used by forward shading, translucency, single-layer water
	 * and volumetric fog. Must stay above MoonForwardShadingPriority, otherwise
	 * the engine breaks the tie on brightness and the night-time moon fill wins.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0"))
	int32 SunForwardShadingPriority = 10;

	/** Forward-shading priority for the moon fill; kept below the sun so it never contends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0"))
	int32 MoonForwardShadingPriority = 0;

	/** Night sun floor in lux (kept tiny, not pure black). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Sky", meta = (ClampMin = "0.0"))
	float NightSunLux = 0.0f;

	//~ Exposure knobs ---------------------------------------------------------
	// Without an exposure clamp the auto-exposure pushes midday to a blown-out
	// white. These drive an unbound post-process volume the controller creates
	// (GTC_GlobalPostProcess). The project runs extended luminance range, so the
	// min/max are EV100 stops; bias is a uniform stop offset (negative = darker).

	/**
	 * Fixed exposure (EV100) for full daylight. One auto-exposure value can't serve
	 * both blinding noon and dark night, so the controller pins exposure and ramps
	 * it Night->Day by the daylight factor every tick instead. 11 = a correctly
	 * exposed sunny midday, verified in-engine on Ocean Drive (lower blows out to
	 * white, higher reads like dusk).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Exposure")
	float DayExposureEV = 11.0f;

	/**
	 * Fixed exposure (EV100) for full night — lower than day so the camera opens up
	 * after dusk and the city's streetlights / neon read instead of the world going
	 * pitch black. ~5 gives a moody lit street; raise toward day for a brighter night.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Exposure")
	float NightExposureEV = 5.0f;

	/** Overall brightness offset in stops (negative darkens). Tunable live with `gtc.Weather.ExposureBias <stops>`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Exposure")
	float ExposureBias = 0.0f;

	//~ Night lighting ---------------------------------------------------------
	/** Add a soft cool "moonlight" fill so the city isn't pitch black between lamps at night. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night")
	bool bMoonlight = true;

	/** Peak moonlight brightness (lux) at full night; eased to 0 through the day so it never tints daylight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "0.0"))
	float MoonIntensityLux = 3.0f;

	/** Cool moonlight colour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night")
	FLinearColor MoonColor = FLinearColor(0.55f, 0.68f, 1.0f);

	/** SkyLight ambient floor at night (0..1) so the city keeps a faint moonlit glow instead of going black. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightSkyLightFloor = 0.18f;

	//~ Auto city lights -------------------------------------------------------
	// Dynamic: the controller re-scans the world for every light and switches them
	// on/off by darkness, so it works for any number of lights (including ones
	// added or streamed in later) without hand-wiring or baking each fixture.

	/** Master switch for the automatic darkness-driven city lights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night")
	bool bAutoCityLights = true;

	/** Switch the city lights ON once it is at least this % dark (0..100). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float CityLightsOnDarknessPct = 65.0f;

	/** Switch them OFF once darkness falls back below this % (kept under On for a stable hold band — no dusk flicker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float CityLightsOffDarknessPct = 55.0f;

	/** When lighting up, raise any fixture dimmer than this (cd) to it so placeholder lights still read. 0 = leave fixtures as authored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "0.0"))
	float CityLightMinIntensity = 2000.0f;

	/** Re-scan the world for lights this often (s) so lights added or streamed in later are picked up too. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "0.25"))
	float CityLightsRescanSeconds = 2.0f;

	/**
	 * Show a starfield after dusk. The controller spawns a camera-centred sky dome
	 * with an additive star material whose opacity is driven by the same continuous
	 * day-night clock (StarOpacity = 1 - daylight), so stars fade in at dusk and out
	 * at dawn, and dim under cloud cover. Additive over the dark night sky and
	 * occluded by city geometry, so it never washes out the day.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night")
	bool bShowStarsAtNight = true;

	/** Overall star brightness multiplier. Tunable live with `gtc.Weather.StarBrightness <x>`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "0.0"))
	float StarBrightness = 4.0f;

	/** Radius (uu) of the camera-centred star dome; every piece of city geometry stays inside it and occludes the stars. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night", meta = (ClampMin = "10000.0"))
	float StarDomeRadius = 800000.0f;

	/** Additive unlit star material (reads scalar params StarOpacity / StarBrightness). Defaults to M_GTCStars. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Night")
	TSoftObjectPtr<UMaterialInterface> StarMaterial;

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

	/**
	 * Live surface wetness, 0 (bone dry) .. 1 (soaked). This is the same value the
	 * controller pushes onto the material collection (world_wetness); exposing it is
	 * the weather-reactive seam gameplay reads to react to the rain — ambient traffic
	 * driving cautious, vehicle tyres losing grip — without re-deriving the weather.
	 */
	UFUNCTION(BlueprintPure, Category = "GTC|Weather")
	float GetWetness() const { return (float)Weather.Current().Wetness; }

	/**
	 * Live visibility, 1 (clear) .. 0 (whiteout), derived from the weather's fog. Feeds
	 * the "drive scared when you can't see" caution (FTrafficWeather) and any other
	 * system that should ease off in fog.
	 */
	UFUNCTION(BlueprintPure, Category = "GTC|Weather")
	float GetVisibility() const { return (float)FMath::Clamp(1.0 - Weather.Current().FogDensity, 0.0, 1.0); }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ResolveSkyActors();
	void EnsureSkyRig();
	void EnsurePostProcess();
	void EnsureMoon();
	void EnsureStarDome();
	void ApplyExposure(double Ev);
	void ApplySky(double Hours);
	void ApplyWeather(const FWeatherParams& W, double Hours);
	void UpdateStars(double Hours);
	void RescanCityLights();
	void UpdateCityLights(double Hours, float DeltaSeconds);
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
	UPROPERTY(Transient)
	TObjectPtr<APostProcessVolume> PostProcess;
	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> Moon;

	/** Camera-centred additive star dome (spawned by EnsureStarDome) and its dynamic material. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> StarDome;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> StarDomeMesh;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StarMID;

	/** All managed city lights (weak — the level/streaming owns them; re-scanned periodically). */
	TArray<TWeakObjectPtr<ULightComponent>> CityLights;
	bool bCityLightsLit = false;
	bool bCityLightsInit = false;
	float TimeSinceLightRescan = 0.0f;

	float BaseSkyLightIntensity = 1.0f;
	bool bResolved = false;
	bool bRigCreated = false;

	/** Console-command handles registered for this instance. */
	TArray<IConsoleCommand*> ConsoleCommands;
};
