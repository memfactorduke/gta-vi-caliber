// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What a citizen says WHILE doing their job — the spoken half of FNpcOccupation.
 * Pairs a work micro-action token with a line that fits that exact bit of the
 * job: the barista calling an order ("Oat-milk doom latte, for... whoever's
 * left!"), the crossing guard blowing the whistle, the conspiracy vendor hawking
 * between glances at the surveillance pigeons. It's what turns "an NPC is at
 * work" into "that's clearly a barista, and I can hear them working".
 *
 * Not every action talks — wiping a counter, framing a shot, scooping after a
 * dog are silent — so a workplace has the texture of occasional announcements
 * over busywork rather than a running monologue.
 *
 * PURE-CORE: scene-free, deterministic FString selection by action token + index
 * (Godot-style posmod wrap, mirroring FIdleActionBark / FBarkPool), no engine
 * coupling. Unit-tested headless (Tests/WorkBarkTest.cpp, prefix
 * GTC.NPC.Dialogue.WorkBark). Keyed by the same FName tokens FNpcOccupation
 * produces; an action with no bank returns "" (silent). GTC-original content.
 */
struct GTC_UE5_API FWorkBark
{
	/**
	 * A line for the given work-action token (e.g. "call_order", "hawk_wares",
	 * "deliver_forecast"), chosen deterministically from Index (posmod-wrapped).
	 * Returns "" for a silent action or an unknown token.
	 */
	static FString Line(FName Action, int32 Index);

	/** Number of lines for a work-action token (0 if the action is silent). */
	static int32 Count(FName Action);
};
