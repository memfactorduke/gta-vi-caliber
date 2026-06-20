// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "../Steering/PedestrianTraffic.h"
#include "../Population/NpcPopulation.h"
#include "GTCCrowdSubsystem.generated.h"

class AGTCCitizen;

/**
 * The player's read on the world's most relevant threat (themselves, usually).
 * Citizens turn this into reactions through FNpcReaction. Speed is cm/s in UE
 * world units; the citizen converts to the Godot metre scale at the boundary.
 */
USTRUCT(BlueprintType)
struct FGTCThreatSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Crowd")
    bool bValid = false;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Crowd")
    FVector Position = FVector::ZeroVector;

    /** Threat actor's planar speed, cm/s. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Crowd")
    double Speed = 0.0;

    /** Threat actor's planar velocity, cm/s — lets a citizen tell whether the
     *  player is closing on it (a bump coming) or just passing by. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|Crowd")
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "GTC|Crowd")
    bool bArmed = false;
};

/**
 * UGTCCrowdSubsystem — the city's director. It does the three things a crowd of
 * AGTCCitizen pawns can't each do for themselves:
 *
 *  1. Keeps the master clock. One hour-of-day for the whole city, advancing at a
 *     configurable day length, so every citizen's schedule is reading the same
 *     time. (A lone, un-streamed citizen falls back to its own clock.)
 *
 *  2. Streams the population around the player. Spawns citizens on a ring just
 *     out of view and retires them once they fall far behind, holding a roughly
 *     constant live headcount — the trick that makes a city feel densely peopled
 *     without simulating a whole metropolis. New citizens are projected onto the
 *     NavMesh when one exists so they spawn on walkable ground. Distant citizens
 *     are ticked less often (a distance LOD), so a big crowd stays affordable.
 *
 *  3. Answers the neighbourhood queries each citizen needs but can't cheaply run
 *     alone: who's near me (separation), is anyone nearby panicking (contagion),
 *     what cars threaten this spot, and where/how dangerous is the player.
 *
 * It OWNS the live-citizen registry and the clock; it does not own the citizens'
 * decisions (those stay in the pure-core models the citizen drives). Ticking via
 * FTickableGameObject — UWorldSubsystem has no native tick.
 */
UCLASS()
class GTC_UE5_API UGTCCrowdSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // --- Tuning -----------------------------------------------------------------

    /** Real seconds in one full in-game day. Drives the master clock rate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Time")
    double SecondsPerDay = 1200.0; // a 20-minute day, GTA-ish compression.

    /** Hour the city starts at (0..24). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Time")
    double StartHour = 8.0;

    /** Whether the director streams a crowd at all (off => only hand-placed citizens). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    bool bStreamPopulation = true;

    /** Embody a fixed, persistent cast (a census that lives off-screen) instead of
     *  minting a fresh disposable citizen on every spawn. Off => legacy behaviour
     *  (each spawn is a new seed, no continuity). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    bool bPersistentPopulation = true;

    /** Size of the persistent census. Should comfortably exceed TargetPopulation so
     *  the director never runs out of un-embodied people to stream in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    int32 PersistentPopulationSize = 240;

    /** Base seed for the census, so the same city is the same cast across runs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    int32 PopulationSeed = 0;

    /** Seconds between off-screen census advances (drives decay slowly; no need to
     *  re-simulate the whole roster every frame). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    double PopulationAdvanceIntervalSec = 0.5;

    /** Class to spawn. Defaults to AGTCCitizen; a BP subclass (with a mesh) may override. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    TSubclassOf<AGTCCitizen> CitizenClass;

    /** Target live headcount around the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    int32 TargetPopulation = 40;

    /** Spawn ring: citizens appear between these radii (cm) from the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    double SpawnRadiusMin = 2500.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    double SpawnRadiusMax = 6000.0;

    /** Beyond this (cm) from the player, a citizen is retired. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    double DespawnRadius = 8000.0;

    /** Max citizens to spawn per streaming pass (amortises spikes). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    int32 MaxSpawnsPerPass = 4;

    /** Seconds between streaming passes (spawning need not run every frame). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|Population")
    double StreamIntervalSec = 0.5;

    /** Distance (cm) past which citizens tick on the coarse LOD interval. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|LOD")
    double NearTickRadius = 4000.0;

    /** Tick interval (s) for far citizens; near ones tick every frame (0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|LOD")
    double FarTickInterval = 0.2;

    /** Within this distance (cm) a citizen is promoted to its high-detail (hero /
     *  MetaHuman) face; beyond it, demoted back to the cheap crowd head. Kept tight
     *  so only the handful of faces the player can actually read pay the rigged cost. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|LOD")
    double FacePromoteRadius = 1500.0;

    // --- UWorldSubsystem --------------------------------------------------------

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    // --- FTickableGameObject ----------------------------------------------------

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }
    virtual bool IsTickableInEditor() const override { return false; }
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

    // --- Clock ------------------------------------------------------------------

    /** Current hour-of-day in [0, 24). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Crowd")
    double GetHourOfDay() const { return HourOfDay; }

    /** In-game hours that pass per real second (derived from SecondsPerDay). */
    double GetHoursPerSecond() const;

    /** Override the clock (debug / cutscene / save-restore). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Crowd")
    void SetHourOfDay(double Hour);

    // --- Threat feed (pushed by the weapon / wanted layer) ----------------------

    /** The weapon layer pushes whether the player is currently brandishing. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Crowd")
    void SetPlayerArmed(bool bArmed) { bPlayerArmed = bArmed; }

    /** The player threat snapshot citizens react to. */
    FGTCThreatSnapshot GetPlayerThreat() const;

    // --- Citizen registry (called by AGTCCitizen) -------------------------------

    void RegisterCitizen(AGTCCitizen* Citizen);
    void UnregisterCitizen(AGTCCitizen* Citizen);

    /** A retiring body hands its lived state back to the census so its roster
     *  member keeps its drives/intent and resumes off-screen. LastPos is the body's
     *  world location, remembered for continuity if you double back. */
    void ReleaseCitizenRecord(
        int32 StableId, const FNpcNeeds& Needs, const FNpcIntent& Intent, const FVector& LastPos);

    /** Store a retiring body's social memory into its census record, so the
     *  person's acquaintances persist across despawn/respawn. No-op off-census. */
    void StoreCitizenAcquaintance(int32 StableId, const FNpcAcquaintance& Acq);

    /**
     * Push the current weather severity (0 clear .. 4 storm) to the crowd. On a
     * CHANGE this broadcasts ReactToWeather to every live citizen (so the city
     * reacts together when the sky turns); unchanged values are a no-op, so the
     * weather controller can call it every tick. Newly spawned citizens pick up
     * the current value on register. */
    void SetWeatherSeverity(int32 Severity);

    /** The weather severity last broadcast (0 clear .. 4 storm). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Crowd")
    int32 GetWeatherSeverity() const { return WeatherSeverity; }

    /** Live citizen count (for HUD/debug). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Crowd")
    int32 GetLiveCount() const { return Citizens.Num(); }

    /** Size of the persistent census (0 when not running persistent population). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Crowd")
    int32 GetCensusSize() const { return Population.Num(); }

    /** Read-only access to the persistent census (debug / save). */
    const FNpcPopulation& GetPopulation() const { return Population; }

    // --- Neighbourhood queries (called by AGTCCitizen each tick) ----------------

    /** Up to MaxNeighbors citizen locations within Radius of From (excluding Exclude). */
    void GatherNeighbors(
        const FVector& From, double Radius, const AGTCCitizen* Exclude, TArray<FVector>& OutLocations) const;

    /** Cars within Radius that the pedestrian-traffic reflex should consider.
     *  Forwards to UGTCTrafficSubsystem::QueryCars — the ambient traffic that now
     *  drives the real road graph — so citizens dodge actual cars (empty only when
     *  no traffic subsystem is present). */
    void GatherNearbyCars(const FVector& From, double Radius, TArray<FPedestrianTraffic::FCar>& OutCars) const;

    /** Nearest panicking (fleeing) citizen within Radius of From, for contagion. */
    bool NearestPanicSource(
        const FVector& From, double Radius, const AGTCCitizen* Exclude, FVector& OutLocation) const;

    /** A nearby citizen that's free to start a conversation (loitering, calm, not
     *  already chatting), or null. Used to pair up two idle people into a chat. */
    AGTCCitizen* FindChatPartner(const FVector& From, double Radius, AGTCCitizen* Self) const;

    /** The nearest other live citizen within Radius that `Self` RECOGNISES (a known
     *  acquaintance, via Self->Recognizes), or null. Lets friends wave/greet as they
     *  pass without stopping. No chat-availability filter — a passing greeting is
     *  lighter than a conversation. */
    AGTCCitizen* FindNearestAcquaintance(const FVector& From, double Radius, AGTCCitizen* Self) const;

    /**
     * Ripple a witnessed assault through the crowd: every citizen near AtLocation
     * (within FNpcWitness range) picks up a distance-attenuated memory of the player's
     * mayhem, so bystanders — not just the victim — recognise, recoil from, and flee
     * the culprit. BaseIntensity (0..1) is how brutal the act was; Victim is excluded
     * (they already took their own memory hit at the moment of contact). Called by a
     * citizen when the player's contact escalates to real violence.
     */
    void BroadcastMayhem(const FVector& AtLocation, double BaseIntensity, const AGTCCitizen* Victim);

    /** Max neighbours returned to any one citizen's separation query (cost bound). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Crowd|LOD")
    int32 MaxNeighbors = 8;

private:
    /** Spawn/retire citizens to hold TargetPopulation around the player. */
    void StreamPopulation(float DeltaTime);

    /** Find a spawn point on the ring, projected to NavMesh when available. */
    bool PickSpawnPoint(const FVector& Around, FVector& OutPoint) const;

    /** Apply the distance LOD tick interval to every live citizen. */
    void ApplyTickLod(const FVector& PlayerPos);

    /** The player pawn, or null. */
    APawn* GetPlayerPawn() const;

    /** Advance the off-screen census on the master clock. */
    void AdvancePopulation(float DeltaTime);

    double HourOfDay = 8.0;
    int32 WeatherSeverity = 0; // last severity broadcast (0 clear .. 4 storm)
    bool bPlayerArmed = false;
    double StreamAccum = 0.0;
    double PopulationAccum = 0.0;
    int32 SpawnCounter = 0;

    /** The persistent census — the cast that lives whether or not it's on screen. */
    FNpcPopulation Population;

    /** Live citizens (weak: a citizen destroyed elsewhere just drops out). */
    UPROPERTY()
    TArray<TWeakObjectPtr<AGTCCitizen>> Citizens;
};
