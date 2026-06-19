// Copyright (c) 2026 GTC contributors

#include "NpcArchetypes.h"

// Shared daily-routine templates. Authored most-specific-first; midnight-wrapping
// blocks allowed (see FNpcSchedule). These mirror the the reference const Arrays
// NINE_TO_FIVE / NIGHT_OWL / STREET_HUSTLE / SLOW_LIFE; block author order is
// preserved by the ordered TArray (first covering block wins on scan).
//
// Builders are function-local statics (unity-build hygiene: no shared file-scope
// symbols). The census is assembled once on first All() and cached.

namespace
{
    // Classic 9-to-5: commute, desk, gym, home, sleep.
    TArray<FNpcScheduleBlock> NpcArchetypes_NineToFive()
    {
        return {
            {7.0, 9.0, TEXT("commute"), TEXT("street")},
            {9.0, 12.5, TEXT("work"), TEXT("office")},
            {12.5, 13.5, TEXT("eat"), TEXT("diner")},
            {13.5, 17.5, TEXT("work"), TEXT("office")},
            {17.5, 19.0, TEXT("goof_off"), TEXT("gym")},
            {19.0, 22.5, TEXT("socialize"), TEXT("bar")},
            {22.5, 7.0, TEXT("sleep"), TEXT("home")},
        };
    }

    // Night owl: sleeps through the day, works/plays the night.
    TArray<FNpcScheduleBlock> NpcArchetypes_NightOwl()
    {
        return {
            {4.0, 13.0, TEXT("sleep"), TEXT("home")},
            {13.0, 18.0, TEXT("goof_off"), TEXT("park")},
            {18.0, 22.0, TEXT("eat"), TEXT("diner")},
            {22.0, 4.0, TEXT("work"), TEXT("bar")},
        };
    }

    // Gig worker: forever in motion, no fixed desk.
    TArray<FNpcScheduleBlock> NpcArchetypes_StreetHustle()
    {
        return {
            {8.0, 11.0, TEXT("work"), TEXT("street")},
            {11.0, 12.0, TEXT("eat"), TEXT("street")},
            {12.0, 19.0, TEXT("work"), TEXT("street")},
            {19.0, 23.0, TEXT("socialize"), TEXT("bar")},
            {23.0, 8.0, TEXT("sleep"), TEXT("home")},
        };
    }

    // Retiree: unhurried, park-bench tempo, early to bed.
    TArray<FNpcScheduleBlock> NpcArchetypes_SlowLife()
    {
        return {
            {6.0, 9.0, TEXT("goof_off"), TEXT("park")},
            {9.0, 12.0, TEXT("loiter"), TEXT("street")},
            {12.0, 13.0, TEXT("eat"), TEXT("diner")},
            {13.0, 18.0, TEXT("loiter"), TEXT("park")},
            {18.0, 20.0, TEXT("socialize"), TEXT("bar")},
            {20.0, 6.0, TEXT("sleep"), TEXT("home")},
        };
    }

    TArray<FNpcArchetype> NpcArchetypes_BuildCitizens()
    {
        const TArray<FNpcScheduleBlock> NineToFive = NpcArchetypes_NineToFive();
        const TArray<FNpcScheduleBlock> NightOwl = NpcArchetypes_NightOwl();
        const TArray<FNpcScheduleBlock> StreetHustle = NpcArchetypes_StreetHustle();
        const TArray<FNpcScheduleBlock> SlowLife = NpcArchetypes_SlowLife();

        TArray<FNpcArchetype> Citizens;

        auto Add = [&Citizens](const TCHAR* Id, const TCHAR* Name, const TArray<FNpcScheduleBlock>& Schedule,
                               double Discipline, const TCHAR* Voice, const FLinearColor& Tint, const TCHAR* Quirk)
        {
            FNpcArchetype A;
            A.Id = Id;
            A.Name = Name;
            A.Schedule = Schedule;
            A.Discipline = Discipline;
            A.Voice = Voice;
            A.Tint = Tint;
            A.Quirk = Quirk;
            Citizens.Add(MoveTemp(A));
        };

        Add(TEXT("doomsday_barista"), TEXT("Doomsday Barista"), NineToFive, 0.3, TEXT("doomsday"),
            FLinearColor(0.30f, 0.22f, 0.18f), TEXT("reads the coming apocalypse in your latte foam"));
        Add(TEXT("pigeon_influencer"), TEXT("Pigeon Influencer"), StreetHustle, -0.6, TEXT("influencer"),
            FLinearColor(0.85f, 0.45f, 0.70f), TEXT("livestreams pigeons to an audience of four"));
        Add(TEXT("method_crossing_guard"), TEXT("Method Crossing Guard"), NineToFive, 0.9, TEXT("method_actor"),
            FLinearColor(0.90f, 0.78f, 0.15f), TEXT("has been preparing for this crosswalk role for six years"));
        Add(TEXT("conspiracy_vendor"), TEXT("Conspiracy Hot-Dog Vendor"), StreetHustle, -0.2, TEXT("conspiracy"),
            FLinearColor(0.65f, 0.20f, 0.18f), TEXT("is pretty sure the pigeons are surveillance drones"));
        Add(TEXT("aggressively_calm_yogi"), TEXT("Aggressively Calm Yoga Instructor"), SlowLife, 0.5, TEXT("yogi"),
            FLinearColor(0.55f, 0.72f, 0.62f), TEXT("will find your center whether you consent or not"));
        Add(TEXT("retired_stunt_double"), TEXT("Retired Stunt Double"), SlowLife, 0.0, TEXT("stunt_double"),
            FLinearColor(0.28f, 0.30f, 0.34f), TEXT("instinctively rolls away from mild inconveniences"));
        Add(TEXT("off_duty_mime"), TEXT("Off-Duty Mime (Still On)"), NightOwl, -0.4, TEXT("mime"),
            FLinearColor(0.92f, 0.92f, 0.92f), TEXT("is trapped in an invisible box and also fine with it"));
        Add(TEXT("overcaffeinated_intern"), TEXT("Over-Caffeinated Intern"), NineToFive, -0.8, TEXT("intern"),
            FLinearColor(0.35f, 0.55f, 0.85f), TEXT("has replaced blood with cold brew and a dream"));
        Add(TEXT("philosophical_dog_walker"), TEXT("Philosophical Dog-Walker"), SlowLife, 0.2, TEXT("philosopher"),
            FLinearColor(0.45f, 0.40f, 0.55f), TEXT("walks seven dogs and one unexamined life"));
        Add(TEXT("feral_food_critic"), TEXT("Feral Food Critic"), NightOwl, -0.3, TEXT("food_critic"),
            FLinearColor(0.70f, 0.55f, 0.25f), TEXT("rates everything, including this sidewalk, out of five stars"));
        Add(TEXT("unlicensed_life_coach"), TEXT("Unlicensed Life Coach"), NineToFive, -0.1, TEXT("life_coach"),
            FLinearColor(0.80f, 0.60f, 0.30f), TEXT("believes in you aggressively and for a small fee"));
        Add(TEXT("weather_anchor_nobody_hired"), TEXT("Self-Appointed Weather Anchor"), StreetHustle, 0.4, TEXT("weather"),
            FLinearColor(0.25f, 0.45f, 0.60f), TEXT("delivers a live forecast to anyone within earshot"));

        return Citizens;
    }
}

const TArray<FNpcArchetype>& FNpcArchetypes::All()
{
    static const TArray<FNpcArchetype> Citizens = NpcArchetypes_BuildCitizens();
    return Citizens;
}

FNpcArchetype FNpcArchetypes::ById(const FString& Id)
{
    for (const FNpcArchetype& C : All())
    {
        if (C.Id == Id)
        {
            return C;
        }
    }
    return FNpcArchetype();
}

FNpcArchetype FNpcArchetypes::Pick(int32 SeedValue)
{
    const TArray<FNpcArchetype>& Citizens = All();
    const int32 N = Citizens.Num();
    if (N == 0)
    {
        return FNpcArchetype();
    }
    // the reference posmod(seed, n): always non-negative in [0, n).
    const int32 Index = ((SeedValue % N) + N) % N;
    return Citizens[Index];
}
