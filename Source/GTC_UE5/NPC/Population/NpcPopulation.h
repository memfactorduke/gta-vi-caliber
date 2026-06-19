// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Decision/NpcNeeds.h"
#include "../Decision/NpcMind.h"
#include "../Decision/NpcAcquaintance.h"

/**
 * The persistent city census — the layer that turns a churning crowd of
 * disposable extras into a fixed cast of people who keep living while you're not
 * looking.
 *
 * The crowd director (UGTCCrowdSubsystem) streams a handful of AGTCCitizen actors
 * around the player and retires them the moment they fall behind. On its own that
 * means every passer-by is born when you approach and ceases to exist when you
 * turn away: internally consistent for the few seconds it's embodied, but with no
 * continuity — walk past the same diner twice and you meet two different strangers.
 *
 * FNpcPopulation is the backing store that gives those bodies a persistent
 * identity. It holds a roster of FNpcCitizenRecord — one stable person each, with
 * a seed (hence a fixed archetype, look, and personality) and a carried inner
 * state (drives + current intent). The roster advances on the master clock as
 * cheap arithmetic whether or not anyone is on screen: needs decay, the schedule
 * and drives pick an activity, and doing that activity (off-screen, by fiat) tops
 * the served drive back up. When the director needs a body it embodies a roster
 * member — restoring its carried drives rather than rolling fresh ones — and
 * writes the live state back when the body retires. The record keeps living.
 *
 * Pure state + math, scene-free, unit-tested headless
 * (Tests/NpcPopulationTest.cpp, prefix GTC.NPC.Population). It leans on the
 * already-tested FNpcNeeds / FNpcMind / FNpcSchedule / FNpcArchetypes models and
 * adds no engine coupling — the AGTCCitizen adapter is the only thing that turns a
 * record into a pawn. Computed precision is `double` to match the rest of the
 * pure core.
 */

/**
 * One persistent citizen: a stable identity plus the slice of inner state that
 * must survive across embodiment. Identity (archetype, traits, decay rates) is a
 * pure function of `Seed`, so the same person is always the same person; only the
 * live drives and current intent change over a day.
 */
struct GTC_UE5_API FNpcCitizenRecord
{
    /** Stable index in the roster — this person's lifelong handle. */
    int32 StableId = INDEX_NONE;

    /** Identity seed: picks the archetype and seeds the trait/decay derivation. */
    int32 Seed = 0;

    /** Personality: how stubbornly the schedule resists a needy drive ([-1, 1]). */
    double Discipline = 0.0;

    /** Steady vs skittish ([0, 1]) — carried so the embodied actor matches. */
    double Bravery = 0.5;

    /** Incurious vs nosy ([0, 1]). */
    double Curiosity = 0.5;

    /** Per-citizen locomotion speeds (cm/s), varied off the base. */
    double WalkSpeed = 150.0;
    double RunSpeed = 420.0;

    /** Deterministic face/head choice in [0, FNpcPopulation::NumHeadVariants). The
     *  adapter maps this to a head skeletal mesh (modular or MetaHuman) so the same
     *  person always wears the same face — the seam that turns the crowd from two
     *  tinted mannequins into distinct human faces. */
    int32 HeadVariant = 0;

    /** Deterministic hair choice in [0, FNpcPopulation::NumHairVariants) — a second
     *  modular variation axis (on top of head + skin tone) so the crowd reads as
     *  individuals. The adapter maps it to a hair skeletal mesh. */
    int32 HairVariant = 0;

    /** Deterministic outfit/clothing choice in [0, FNpcPopulation::NumOutfitVariants)
     *  — the third modular axis. The adapter maps it to an outfit skeletal mesh,
     *  tinted by the archetype. */
    int32 OutfitVariant = 0;

    /** Per-drive hourly decay rates (name -> satisfaction lost per in-game hour). */
    TMap<FString, double> DecayRates;

    /** Live drives — the part that decays and recovers as the day goes on. */
    FNpcNeeds Needs;

    /** The activity/place the record is currently living out (off-screen or via its body). */
    FNpcIntent CurrentIntent;

    /** Who this person has come to recognise — their social ties, carried across
     *  embodiment so a regular stays a regular after the body despawns and respawns.
     *  Empty for a fresh FromSeed person; filled as they meet others (see
     *  FNpcAcquaintance). The embodied actor restores this on spawn and stores it
     *  back on retire. */
    FNpcAcquaintance Acquaintances;

    /** True while a live AGTCCitizen owns this record — the actor drives it; the
     *  off-screen Advance leaves it alone to avoid double-simulating. */
    bool bEmbodied = false;

    /** Where the body was standing when it last retired. Lets a re-embodied citizen
     *  reappear roughly where you last saw it (continuity) instead of on a fresh
     *  spawn ring — valid only for short round-trips, gated on range at spawn time. */
    FVector LastPosition = FVector::ZeroVector;
    bool bHasLastPosition = false;

    /**
     * Derive a record's full identity from a seed, reproducing exactly the draw
     * sequence the AGTCCitizen adapter uses (Bravery, Curiosity, WalkSpeed,
     * RunSpeed, decay rates, then starting drives) so an embodied record and a
     * fresh-seeded actor are the same person to the bit. `StableId` is stored
     * verbatim and does not affect the derivation.
     */
    static FNpcCitizenRecord FromSeed(int32 StableId, int32 Seed);
};

struct GTC_UE5_API FNpcPopulation
{
    /** The cast, indexed by StableId (Roster[i].StableId == i). */
    TArray<FNpcCitizenRecord> Roster;

    /** Number of people in the census. */
    int32 Num() const { return Roster.Num(); }

    /** Build a census of `Count` people, each FromSeed(i, BaseSeed + i). Replaces
     *  any existing roster. */
    void Seed(int32 Count, int32 BaseSeed = 0);

    /**
     * Advance every NON-embodied record by `ElapsedHours` at clock `Hour`: decay
     * its drives, let FNpcMind pick the activity its routine + drives call for,
     * and credit the served drive for doing that activity off-screen (so an
     * unembodied citizen lives a believable day instead of starving). Embodied
     * records are skipped — their actor simulates them for real.
     */
    void Advance(double Hour, double ElapsedHours);

    /** Read a record by stable id, or null if out of range. */
    const FNpcCitizenRecord* Find(int32 StableId) const;

    /**
     * Claim the next un-embodied record for a body, marking it embodied and
     * returning its StableId, or INDEX_NONE when the whole cast is already on
     * screen (roster smaller than the live headcount — size it larger).
     */
    int32 AcquireFreeRecord();

    /**
     * Hand a record's live state back when its body retires: copy the actor's
     * drives + intent into the record and clear the embodied flag so off-screen
     * Advance resumes. When bStoreLastPosition, also remember LastPos (the body's
     * world location) for continuity on re-embodiment. No-op for an unknown id.
     */
    void ReleaseRecord(
        int32 StableId,
        const FNpcNeeds& Needs,
        const FNpcIntent& Intent,
        const FVector& LastPos = FVector::ZeroVector,
        bool bStoreLastPosition = false);

    /** Store a body's social memory back into its record, so the person's
     *  acquaintances survive despawn/respawn. No-op for an unknown id. Separate
     *  from ReleaseRecord so the retire path stays a drop-in (no signature churn). */
    void StoreAcquaintance(int32 StableId, const FNpcAcquaintance& Acq);

    // --- Shared need tuning (single source of truth for adapter + off-screen sim) ---

    /** Base hourly decay rate for a drive (before per-citizen variance). 0 = no decay. */
    static double BaseDecayRate(const FString& Need);

    /**
     * Which drive an activity tops up and the full satisfaction it grants on
     * completion. Returns false for activities that serve no drive (e.g. "work",
     * "commute"). The embodied actor applies OutAmount instantly on arrival; the
     * off-screen sim meters it out over the dwell.
     */
    static bool ActivitySatisfies(const FString& Activity, FString& OutNeed, double& OutAmount);

    /** In-game hours of doing an activity to fully satisfy its drive off-screen —
     *  the rate the off-screen credit is metered at. */
    static constexpr double ActivityDwellHours = 1.0;

    /** Number of distinct face/head variants a citizen's HeadVariant selects among.
     *  The adapter's head-mesh table is indexed modulo this. */
    static constexpr int32 NumHeadVariants = 8;

    /** Number of distinct hair variants a citizen's HairVariant selects among. */
    static constexpr int32 NumHairVariants = 8;

    /** Number of distinct outfit variants a citizen's OutfitVariant selects among. */
    static constexpr int32 NumOutfitVariants = 8;
};
