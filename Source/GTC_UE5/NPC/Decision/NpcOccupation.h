// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What a citizen actually DOES at their job — the difference between "stands at
 * the office/bar/street tagged work" and "is visibly a barista pulling shots, a
 * crossing guard waving traffic, a dog-walker untangling leashes". The schedule
 * (FNpcSchedule) already sends each archetype to a workplace with activity
 * "work"; this layer says what the work looks like, per ROLE, so a knot of
 * working pawns reads as a street of people each doing their own specific job.
 *
 * Roles are derived from the archetype id (FNpcArchetypes), so the Doomsday
 * Barista pulls espresso (and foretells doom), the Method Crossing Guard blows
 * the whistle and studies his motivation, the Conspiracy Hot-Dog Vendor scans
 * for surveillance pigeons between orders. Each role owns a bank of work
 * micro-actions; the citizen cycles them while on the clock, handing each token
 * to the anim layer exactly like FNpcIdleAction.
 *
 * PURE-CORE: scene-free, deterministic FName selection by role + seed (Godot
 * posmod wrap, mirroring FNpcIdleAction / FBarkPool), no engine coupling.
 * Unit-tested headless (Tests/NpcOccupationTest.cpp, prefix
 * GTC.NPC.Decision.NpcOccupation). GTC-original content (not a Godot parity
 * port), so the banks are free to grow.
 */
struct GTC_UE5_API FNpcOccupation
{
	/**
	 * The job role token for an archetype id (e.g. "barista", "crossing_guard",
	 * "vendor", "dog_walker"...). Returns NAME_None for an unknown/empty id — such
	 * a citizen has no specific job and falls back to generic idle business.
	 */
	static FName RoleFor(const FString& ArchetypeId);

	/**
	 * A role-specific work micro-action, chosen deterministically from Seed
	 * (posmod-wrapped over the role's bank). An unknown/None role draws from a
	 * generic "on shift" bank; the result is never NAME_None unless even that bank
	 * is empty (it isn't). The token is what the anim layer plays.
	 */
	static FName WorkAction(FName Role, int32 Seed);

	/** Number of distinct work actions for a role (generic-bank count for None). */
	static int32 WorkActionCount(FName Role);
};
