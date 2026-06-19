// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A little slack in everyone's day. Archetypes share a few routine templates
 * (nine-to-five, night owl, ...), so without this every nine-to-fiver commutes at
 * exactly 7:00, breaks for lunch at exactly 12:30, and heads home on the same
 * frame — a city moving in lockstep. This gives each citizen a fixed, small,
 * deterministic shift to their whole clock, so one is out the door at 6:42 and
 * another at 7:21, lunches stagger, and the streets ebb and flow instead of
 * pulsing. Same seed -> same shift, so a person keeps their rhythm across reloads.
 *
 * Applied only to the hour fed to the schedule/mind decision (not the world
 * clock), so weather, lighting, and the shared day stay global while each
 * person's ROUTINE rides its own offset.
 *
 * PURE-CORE: scene-free, deterministic, no engine coupling. Unit-tested headless
 * (Tests/NpcScheduleJitterTest.cpp, prefix GTC.NPC.Decision.NpcScheduleJitter).
 * GTC-original.
 */
struct GTC_UE5_API FNpcScheduleJitter
{
	/** Default half-width of the per-citizen shift, in hours (~45 min each way). */
	static constexpr double DefaultMaxHours = 0.75;

	/**
	 * This citizen's fixed clock offset, in hours, deterministic from `Seed` and
	 * spread across [-MaxHours, +MaxHours]. Same seed always yields the same shift.
	 */
	static double HourOffset(int32 Seed, double MaxHours = DefaultMaxHours);

	/**
	 * `Hour` shifted by this citizen's offset and wrapped into [0,24) — the hour to
	 * feed the schedule/mind decision so the person's routine runs on its own clock.
	 */
	static double Apply(double Hour, int32 Seed, double MaxHours = DefaultMaxHours);
};
