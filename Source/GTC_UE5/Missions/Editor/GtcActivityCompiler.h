// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Activities/SideJob.h"
#include "../Activities/StreetRace.h"

struct FGtcSideJobDefinition;
struct FGtcStreetRaceDefinition;

/**
 * Translates the editor's authored activity payloads (side jobs, street races) into the
 * existing pure activity models (SideJob / StreetRace) and exposes their reward maths.
 * Pure and headless — the same bridge role GtcMissionCompiler plays for narrative missions.
 *
 * Activities don't use UMissionController; they run their own pure model (a SideJob tracker
 * advanced by pickup/dropoff proximity, a StreetRace advanced by checkpoint overlaps). The
 * live per-frame driving + payout-into-wallet is the god-mode/PIE phase; this layer makes an
 * authored activity instantiable and correctly priced, and is proven by running the real
 * models to completion in tests.
 */
namespace GtcActivityCompiler
{
    /** Authored side job -> the pure SideJob::FJob the runtime tracker runs. */
    GTC_UE5_API SideJob::FJob CompileSideJob(const FGtcSideJobDefinition& Def);

    /** Total cash for completing the authored side job over Distance (m) in TimeTaken (s). */
    GTC_UE5_API int32 SideJobPayout(const FGtcSideJobDefinition& Def, double Distance, double TimeTaken);

    /** Authored street race -> a ready-to-run StreetRace instance (checkpoints + laps). */
    GTC_UE5_API StreetRace CompileRace(const FGtcStreetRaceDefinition& Def);

    /** Cash for finishing the authored race in the given 1-based placement (0 = DNF). */
    GTC_UE5_API int32 RaceReward(const FGtcStreetRaceDefinition& Def, int32 Placement);
}
