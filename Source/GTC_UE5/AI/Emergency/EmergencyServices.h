// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure civilian-emergency dispatch model — ambulances, fire trucks, and
 * paramedics responding to incidents (a wreck, a fire, a downed pedestrian).
 *
 * Direct port of the Godot `EmergencyServices` (RefCounted) at
 * `game/scripts/ai/emergency_services.gd`. Distinct from the police pursuit
 * layers: this routes the *help* services toward an incident and runs the
 * paramedic-on-scene timer. The one police-shaped role is PoliceBackup for a
 * Shooting — coordination, not a duplicate of the pursuit math.
 *
 * Static dispatch helpers (no scene/RNG/node state, deterministic, unit-tested
 * in Tests/EmergencyServicesTest.cpp, prefix GTC.AI.Emergency) plus one small
 * stateful response timer (the same Begin/Tick/HasArrived/Progress/Treating/
 * Cancel/Reset surface as the Godot original). Spatial math is on the XZ plane
 * (Y ignored), matching the Godot source.
 *
 * PURE-CORE boundary: the helpers take positions/speed/responders as plain
 * inputs. Actual responder spawning, world positioning, navmesh/physics
 * distance, and per-frame tick wiring are the DEFERRED Wave-3 adapter — NOT
 * implemented here and NOT covered by the parity tests. These helpers stay pure
 * scalar/vector math, computed precision being `double` to match GDScript.
 */

/**
 * Which service rolls out — an ambulance for the hurt, a fire truck for fire,
 * police backup to secure a violent scene. Godot ordinal order preserved.
 */
enum class EEmergencyService : uint8
{
    Ambulance,
    FireTruck,
    PoliceBackup,
};

/**
 * What happened. Wreck = a vehicle collision, Fire = a structure/vehicle
 * ablaze, Injury = a downed/hurt pedestrian, Shooting = a violent crime scene.
 * Godot ordinal order preserved.
 */
enum class EEmergencyIncident : uint8
{
    Wreck,
    Fire,
    Injury,
    Shooting,
};

/**
 * Response-timer phases: Idle before Begin(), EnRoute driving to the scene,
 * then Treating once on-scene and working the victim. Godot ordinal order.
 */
enum class EEmergencyPhase : uint8
{
    Idle,
    EnRoute,
    Treating,
};

struct GTC_UE5_API FEmergencyServices
{
    /**
     * At/above this many wanted stars a scene is "hot": unarmed medical and fire
     * crews won't approach until the heat drops. Armed police backup still rolls.
     */
    static constexpr int32 HotSceneStars = 3;

    /**
     * Severity (0 unhurt .. 1 fatal) at/above which revival fails outright —
     * past saving by the time the medic arrives.
     */
    static constexpr double UnrevivableSeverity = 1.0;

    // --- Static dispatch helpers (stateless) --------------------------------

    /**
     * The primary service for a raw incident ordinal. Fire -> FireTruck,
     * Injury/Wreck -> Ambulance, Shooting -> PoliceBackup. An unknown/out-of-range
     * incident defaults to an Ambulance (help-first: someone is probably hurt).
     * Mirrors the Godot `service_for(int)` so `ServiceForInt(999)` -> Ambulance
     * is expressible.
     */
    static EEmergencyService ServiceForInt(int32 IncidentRaw);

    /** Typed convenience overload of ServiceForInt for a known incident. */
    static EEmergencyService ServiceFor(EEmergencyIncident Incident);

    /**
     * Travel time (seconds) from a dispatch point to the incident at a given
     * response speed, on the XZ plane (Y ignored). Guarded: zero/negative speed
     * returns +infinity (can't get there), so callers treat it as "no responder
     * available". Tests assert the guard via !FMath::IsFinite.
     */
    static double EtaSeconds(const FVector& Dispatch, const FVector& Incident, double Speed);

    /** A candidate responder for NearestResponderIndex. */
    struct FResponder
    {
        FVector Pos;
        bool bHasPos = true;
        EEmergencyService Service = EEmergencyService::Ambulance;
    };

    /**
     * Index of the responder closest to the incident, by squared XZ distance
     * (strict <, first wins ties). Entries with bHasPos == false are skipped. An
     * empty list / no usable entry returns INDEX_NONE.
     */
    static int32 NearestResponderIndex(const FVector& Incident, const TArray<FResponder>& Responders);

    /**
     * Whether help should actually be dispatched now.
     * - Shooting always gets PoliceBackup — armed crews go in regardless of heat.
     * - Unarmed medical/fire crews refuse a hot scene (wanted >= HotSceneStars)
     *   *that the player caused*. A hot scene the player didn't cause still gets
     *   help. Below the hot threshold, any known incident is dispatched.
     */
    static bool ShouldDispatch(EEmergencyIncident Incident, bool bPlayerCaused, int32 WantedStars);

    /**
     * Paramedic revival odds (0 .. 1) by victim injury severity (0 unhurt .. 1
     * fatal). Falls linearly as the victim is more hurt; at/above
     * UnrevivableSeverity it is 0.
     */
    static double ReviveChance(double Severity);

    // --- Stateful response timer --------------------------------------------

    /** ResponseDelay = seconds of siren-run before the unit reaches the scene. */
    explicit FEmergencyServices(double ResponseDelay = 6.0);

    /** Dispatch the unit: start the run to the scene. No-op unless Idle. */
    void Begin();

    /**
     * Advance the run. No-op before Begin() or once on-scene. Crossing the delay
     * flips EnRoute -> Treating exactly once.
     */
    void Tick(double Delta);

    /** True once the unit has reached the scene (i.e. is Treating). */
    bool HasArrived() const;

    /**
     * True while a paramedic is on-scene working the victim. Identical to
     * HasArrived() by design — the oracle tests both separately, so both are kept.
     */
    bool Treating() const;

    /**
     * Run progress 0 .. 1 toward arrival. 1.0 once on-scene; 1.0 too when the
     * delay is zero (instant response).
     */
    double Progress() const;

    /** Stand the unit down mid-run. Back to Idle. */
    void Cancel();

    /** Full reset to a fresh, un-dispatched unit (alias of Cancel for symmetry). */
    void Reset();

    /** The public run state. */
    EEmergencyPhase GetPhase() const;

private:
    EEmergencyPhase Phase = EEmergencyPhase::Idle;
    double ResponseDelay = 0.0;
    double Elapsed = 0.0;
};
