// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What a working citizen says TO THE PLAYER walking past — the job reaching out.
 * A passer-by used to get the same generic "watch yourself" from everyone; this
 * makes the encounter read through the NPC's occupation: the hot-dog vendor
 * tries to sell (cash only, the pigeons are listening), the crossing guard
 * scolds you toward the crosswalk, the unlicensed life coach pitches you on
 * yourself for a small fee. It's how the player feels like they're moving
 * through a city of people with jobs, not set dressing.
 *
 * Keyed by the FNpcOccupation role token, so only the talky trades hail; a role
 * with no bank (the mime, or a non-job citizen) returns "" and the caller falls
 * back to the generic pass-by line. Lines are player-directed (second person),
 * voice-agnostic, characterful.
 *
 * PURE-CORE: scene-free, deterministic FString selection by role + index
 * (Godot-style posmod wrap, mirroring FWorkBark / FContactBark), no engine
 * coupling. Unit-tested headless (Tests/NpcHailTest.cpp, prefix
 * GTC.NPC.Dialogue.NpcHail). GTC-original content.
 */
struct GTC_UE5_API FNpcHail
{
	/**
	 * A player-directed hail for the given job role (FNpcOccupation::RoleFor),
	 * chosen deterministically from Index (posmod-wrapped). Returns "" for a role
	 * with no hail bank (e.g. the mime) or an unknown/None role.
	 */
	static FString Line(FName Role, int32 Index);

	/** Number of hail lines for a role (0 if the role doesn't hail). */
	static int32 Count(FName Role);
};
