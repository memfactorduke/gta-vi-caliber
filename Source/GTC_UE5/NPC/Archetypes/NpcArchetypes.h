// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "../Decision/NpcSchedule.h"

/**
 * The census of who lives in this city — a catalog of absurd citizens, each a
 * bundle of {daily routine, personality, dialogue voice, visual tint, quirk}.
 *
 * Direct PURE-CORE port of the Godot `NpcArchetypes` (RefCounted) at
 * `game/scripts/npc/npc_archetypes.gd`. Pure data + deterministic selection,
 * scene-free, unit-tested headless (Tests/NpcArchetypesTest.cpp, prefix
 * GTC.NPC.Archetypes). Computed precision is `double` to match GDScript; tints
 * are linear float RGB to match the Godot `Color(r, g, b)` literals.
 *
 * Schedule templates are expressed as ordered TArray<FNpcScheduleBlock> (reusing
 * the already-ported FNpcSchedule), preserving the Godot Array author order that
 * NpcSchedule scanning observes (first covering block wins). The citizen census
 * is likewise an ordered TArray so All()/Pick() seeding is stable.
 *
 * DEFERRED — Wave 3 adapter (NOT implemented, NOT tested here): the engine
 * coupling that the Godot comment describes — the spawner reading `Tint` onto
 * the shared procedural HumanoidBody, NpcDialogue reading `Voice`, and any
 * Citizen actor / appearance / DataTable wiring — is an engine-feel adapter that
 * belongs to Wave 3. This file ports only the pure model (the census data and
 * the All/ById/Pick selectors).
 */

/**
 * One citizen archetype. Mirrors the Godot dict
 * {id, name, schedule, personality{discipline}, voice, tint, quirk}.
 * `Tint` is a linear float colour (Godot Color). `Discipline` in [-1, 1] is the
 * only personality field the model carries.
 */
struct GTC_UE5_API FNpcArchetype
{
    FString Id;
    FString Name;
    TArray<FNpcScheduleBlock> Schedule;
    double Discipline = 0.0;
    FString Voice;
    FLinearColor Tint = FLinearColor::Black;
    FString Quirk;

    /** True for the empty/unknown archetype returned by ById on a miss. */
    bool IsEmpty() const
    {
        return Id.IsEmpty() && Name.IsEmpty() && Schedule.Num() == 0;
    }
};

struct GTC_UE5_API FNpcArchetypes
{
    /** Every archetype, in declaration order (stable for tests + seeding). */
    static const TArray<FNpcArchetype>& All();

    /** Look one up by id, or an empty FNpcArchetype (IsEmpty()) if unknown. */
    static FNpcArchetype ById(const FString& Id);

    /**
     * Deterministically pick an archetype from a seed (e.g. an NPC's spawn
     * index), so the same NPC is always the same person across reloads. Wraps
     * via positive modulo. Returns an empty archetype only if the census is
     * empty (never, in practice).
     */
    static FNpcArchetype Pick(int32 SeedValue);
};
