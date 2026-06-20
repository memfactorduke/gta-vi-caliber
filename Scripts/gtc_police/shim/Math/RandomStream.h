// Host-clang shim of FRandomStream for the police pure-core verifier. Only
// Initialize(int32) + FRand() are faked, with a deterministic LCG. The exact
// sequence does NOT match UE's FRandomStream (so absolute spawn positions differ
// from an in-engine run), but determinism holds — same seed yields the same
// stream — which is all the structural invariants the verifier checks rely on.
#pragma once

#include "../CoreMinimal.h"

class FRandomStream
{
public:
    FRandomStream() : Seed(0) {}
    explicit FRandomStream(int32 InSeed) : Seed(static_cast<uint32_t>(InSeed)) {}

    void Initialize(int32 InSeed) { Seed = static_cast<uint32_t>(InSeed); }

    /** Uniform double in [0, 1). Deterministic per seed (Numerical Recipes LCG). */
    float FRand() const
    {
        Seed = 1664525u * Seed + 1013904223u;
        return static_cast<float>(Seed >> 8) / static_cast<float>(1u << 24);
    }

private:
    mutable uint32_t Seed;
};
