// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcAttitude.h"

namespace
{
    // A small, platform-stable hash of (seed, lowercased id) so temper is deterministic
    // and reproducible across runs/machines — not relying on FString::GetTypeHash, which
    // is only required to be stable within a process. FNV-1a over the bytes, salted by seed.
    uint32 GtcAttitudeHash(int32 Seed, const FString& Id)
    {
        uint32 H = 2166136261u ^ static_cast<uint32>(Seed);
        const FString Lower = Id.ToLower();
        for (TCHAR C : Lower)
        {
            H = (H ^ static_cast<uint32>(C)) * 16777619u;
        }
        // Final avalanche so adjacent seeds don't bucket together.
        H ^= H >> 15; H *= 2246822519u; H ^= H >> 13;
        return H;
    }

    // Godot posmod(value, n): always non-negative in [0, n).
    int32 GtcAttitudeWrap(int32 Value, int32 N)
    {
        return N <= 0 ? 0 : ((Value % N) + N) % N;
    }

    int32 GtcAttitudeClampTier(int32 Tier)
    {
        return FMath::Clamp(Tier, 0, static_cast<int32>(ENpcVerbalHeat::SquareUp));
    }

    // The baseline verbal heat a temper starts from before a provocation modifies it.
    int32 GtcAttitudeBaseTier(ENpcTemper Temper)
    {
        switch (Temper)
        {
        case ENpcTemper::Meek:      return static_cast<int32>(ENpcVerbalHeat::None);
        case ENpcTemper::Easygoing: return static_cast<int32>(ENpcVerbalHeat::Mutter);
        case ENpcTemper::Surly:     return static_cast<int32>(ENpcVerbalHeat::Insult);
        case ENpcTemper::Hothead:   return static_cast<int32>(ENpcVerbalHeat::Curse);
        }
        return static_cast<int32>(ENpcVerbalHeat::Mutter);
    }
}

ENpcTemper FNpcAttitude::TemperFor(int32 Seed, const FString& ArchetypeId)
{
    // Roll in [0,100). Most people are Easygoing; a spicy minority skews up.
    const int32 Roll = static_cast<int32>(GtcAttitudeHash(Seed, ArchetypeId) % 100u);
    if (Roll < 15) { return ENpcTemper::Meek; }       // 15%
    if (Roll < 70) { return ENpcTemper::Easygoing; }  // 55%
    if (Roll < 92) { return ENpcTemper::Surly; }      // 22%
    return ENpcTemper::Hothead;                        //  8%
}

bool FNpcAttitude::IsRude(ENpcTemper Temper)
{
    return Temper == ENpcTemper::Surly || Temper == ENpcTemper::Hothead;
}

ENpcVerbalHeat FNpcAttitude::ProvocationResponse(
    ENpcTemper Temper, ENpcProvocation Provocation, double Bravery, int32 Seed)
{
    // PeerPassing is gossip ABOUT a passer-by, not a response to being hit: only the
    // rude bother, and only ever under their breath.
    if (Provocation == ENpcProvocation::PeerPassing)
    {
        return IsRude(Temper) ? ENpcVerbalHeat::Mutter : ENpcVerbalHeat::None;
    }

    int32 Tier = GtcAttitudeBaseTier(Temper);

    switch (Provocation)
    {
    case ENpcProvocation::Bumped:   Tier += 1; break; // a barge is personal
    case ENpcProvocation::NearMiss: Tier += 1; break; // nearly run over — adrenaline
    case ENpcProvocation::Honked:   Tier += 0; break; // rude, but not aimed at them
    case ENpcProvocation::Stared:   Tier -= 1; break; // just crowding — low simmer
    default: break;
    }

    // In-character jitter: hotheads occasionally boil over a notch; the meek occasionally
    // bite their tongue. Deterministic from the seed so a given moment is reproducible.
    const int32 Jitter = GtcAttitudeWrap(Seed, 3); // 0,1,2
    if (Temper == ENpcTemper::Hothead && Jitter == 0) { Tier += 1; }
    if (Temper == ENpcTemper::Meek && Jitter == 0)    { Tier -= 1; }

    Tier = GtcAttitudeClampTier(Tier);

    // SquareUp is reserved for the rude who are also brave; everyone else caps at Curse.
    if (Tier >= static_cast<int32>(ENpcVerbalHeat::SquareUp))
    {
        const bool bWillSquareUp = IsRude(Temper) && Bravery > 0.65;
        if (!bWillSquareUp)
        {
            Tier = static_cast<int32>(ENpcVerbalHeat::Curse);
        }
    }

    // The meek never escalate past a muttered insult, whatever the provocation.
    if (Temper == ENpcTemper::Meek)
    {
        Tier = FMath::Min(Tier, static_cast<int32>(ENpcVerbalHeat::Insult));
    }

    return static_cast<ENpcVerbalHeat>(GtcAttitudeClampTier(Tier));
}
